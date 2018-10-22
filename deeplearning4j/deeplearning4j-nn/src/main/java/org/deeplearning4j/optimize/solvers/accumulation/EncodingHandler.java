/*******************************************************************************
 * Copyright (c) 2015-2018 Skymind, Inc.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License, Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 ******************************************************************************/

package org.deeplearning4j.optimize.solvers.accumulation;

import com.google.common.util.concurrent.AtomicDouble;
import lombok.NonNull;
import lombok.extern.slf4j.Slf4j;
import org.nd4j.linalg.api.buffer.DataBuffer;
import org.nd4j.linalg.api.ndarray.INDArray;
import org.nd4j.linalg.compression.NDArrayCompressor;
import org.nd4j.linalg.exception.ND4JIllegalStateException;
import org.nd4j.linalg.factory.Nd4j;
import org.nd4j.linalg.ops.transforms.Transforms;

import java.text.DecimalFormat;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicLong;

/**
 * This MessageHandler implementation is suited for debugging mostly, but still can be used in production environment if you really want that.
 * Basic idea: updates are encoded before sharing.
 *
 * This handler is used as basement for distributed handler though.
 *
 * PLEASE NOTE: This handler does NOT provide any network connectivity. *
 * @author raver119@gmail.com
 */
@Slf4j
public class EncodingHandler implements MessageHandler {
    protected transient GradientsAccumulator accumulator;
    protected double threshold, minThreshold, thresholdStep, stepTrigger;
    protected int shakeFrequency;
    protected int stepDelay;
    protected Double boundary = null;
    protected boolean encodingDebugMode;
    protected NDArrayCompressor compressor;
    protected AtomicInteger atomicBoundary = new AtomicInteger(-1);

    protected ThreadLocal<AtomicLong> iterations = new ThreadLocal<>();
    protected ThreadLocal<AtomicLong> lastStep = new ThreadLocal<>();
    protected ThreadLocal<AtomicDouble> currentThreshold = new ThreadLocal<>();
    protected ThreadLocal<AtomicBoolean> bitmapMode = new ThreadLocal<>();

    /**
     * This method builds new EncodingHandler instance with initial threshold of 1e-3
     *
     */
    public EncodingHandler() {
        this(1e-3, false);
    }

    /**
     * This method builds new EncodingHandler instance
     *
     * @param threshold Initial encoding threshold
     */
    public EncodingHandler(double threshold, boolean encodingDebugMode) {
        this(threshold, null, encodingDebugMode);
    }

    /**
     * This method builds new EncodingHandler instance
     *
     * @param threshold Initial encoding threshold
     */
    public EncodingHandler(double threshold, Double boundary, boolean encodingDebugMode) {
        this(threshold, threshold, 0.0, 0, 0, 0, boundary, encodingDebugMode);
    }

    /**
     * This method builds new EncodingHandler instance
     *
     * @param threshold Initial encoding threshold
     * @param minThreshold Minimal encoding threshold (for threshold decay)
     * @param thresholdStep Decay step for threshold decay
     * @param stepTrigger Sparse/Dense ratio that will trigger decay step. In range 0..100
     * @param stepDelay Minimal number of iterations between decay steps
     * @param shakeFrequency How ofter we'll be sending dense updates with lower threshold
     */
    public EncodingHandler(double threshold, double minThreshold, double thresholdStep, double stepTrigger,
                    int stepDelay, int shakeFrequency, boolean encodingDebugMode) {
        this(threshold, minThreshold, thresholdStep, stepTrigger, stepDelay, shakeFrequency, null, encodingDebugMode);
    }

    /**
     * This method builds new EncodingHandler instance
     *
     * @param threshold Initial encoding threshold
     * @param minThreshold Minimal encoding threshold (for threshold decay)
     * @param thresholdStep Decay step for threshold decay
     * @param stepTrigger Sparse/Dense ratio that will trigger decay step. In range 0..100
     * @param stepDelay Minimal number of iterations between decay steps
     * @param shakeFrequency How ofter we'll be sending dense updates with lower threshold
     * @param boundary
     */
    public EncodingHandler(double threshold, double minThreshold, double thresholdStep, double stepTrigger,
                    int stepDelay, int shakeFrequency, Double boundary, boolean encodingDebugMode) {
        this.threshold = threshold;
        this.minThreshold = minThreshold;
        this.stepTrigger = stepTrigger;
        this.stepDelay = stepDelay;
        this.thresholdStep = thresholdStep;
        this.shakeFrequency = shakeFrequency;
        this.boundary = boundary;
        this.encodingDebugMode = encodingDebugMode;
    }

    @Override
    public void initialize(@NonNull GradientsAccumulator accumulator) {
        this.accumulator = accumulator;

        compressor = Nd4j.getCompressor().getCompressor("THRESHOLD");
        if (compressor == null)
            throw new ND4JIllegalStateException("Can't find Threshold compressor implementation!");

        compressor.configure(threshold);
    }

    public INDArray encodeUpdates(INDArray updates) {

        // special op should be called here for encoding
        if (bitmapMode.get() == null) {
            bitmapMode.set(new AtomicBoolean(true));
            currentThreshold.set(new AtomicDouble(threshold));
            iterations.set(new AtomicLong(0));
            lastStep.set(new AtomicLong(0));
        }

        //Debug output if enabled:
        residualDebugOutputIfRequired(updates);

        iterations.get().incrementAndGet();

        if (boundary != null && atomicBoundary.get() < 0)
            atomicBoundary.compareAndSet(-1, (int) (updates.lengthLong() * boundary));

        INDArray encoded = null;

        if (!bitmapMode.get().get()) {
            // if shakeFrequency hits here, we'll use bitmap encoding for one round for 1/3 of current threshold
            if (shakeFrequency != 0 && iterations.get().get() % shakeFrequency == 0) {
                DataBuffer buffer = Nd4j.getDataBufferFactory().createInt(updates.lengthLong() / 16 + 5);
                encoded = Nd4j.createArrayFromShapeBuffer(buffer, updates.shapeInfoDataBuffer());

                Nd4j.getExecutioner().bitmapEncode(updates, encoded, currentThreshold.get().get() / 3);
            } else {
                // otherwise (probably most often - we go for sparse
                encoded = Nd4j.getExecutioner().thresholdEncode(updates, currentThreshold.get().get(),
                                boundary == null ? null : atomicBoundary.get());

                // updates were TOO sparse, nothing to share here
                if (encoded == null)
                    return null;


                double encLen = encoded.data().getInt(0);
                double encodingRatio = encLen * 100.0 / updates.length();

                // if updates are too dense - we fallback to bitmap encoding
                if (encLen >= (updates.lengthLong() / 16)) {
                    log.debug("Going back to bitmapEncoding");
                    bitmapMode.get().set(true);

                    DataBuffer buffer = Nd4j.getDataBufferFactory().createInt(updates.lengthLong() / 16 + 5);
                    encoded = Nd4j.createArrayFromShapeBuffer(buffer, updates.shapeInfoDataBuffer());

                    Nd4j.getExecutioner().bitmapEncode(updates, encoded, currentThreshold.get().get());

                    return encoded;
                }


                // after encoding is finished, and updates are sparse enough - let's step down a bit
                // and we don't step down too early, so we wait for 50 iterations at least to step down
                if (minThreshold <= currentThreshold.get().get()
                                && minThreshold < currentThreshold.get().get() - thresholdStep
                                && iterations.get().get() > lastStep.get().get() + stepDelay
                                && encodingRatio < stepTrigger) {
                    currentThreshold.get().addAndGet(-thresholdStep);
                    lastStep.set(iterations.get());
                    log.debug("Threshold steps down to {}", currentThreshold.get().get());
                }
            }
        } else {
            DataBuffer buffer = Nd4j.getDataBufferFactory().createInt(updates.lengthLong() / 16 + 5);
            encoded = Nd4j.createArrayFromShapeBuffer(buffer, updates.shapeInfoDataBuffer());

            long values = Nd4j.getExecutioner().bitmapEncode(updates, encoded, currentThreshold.get().get());

            if (values < (updates.lengthLong() / 16 + 5) / 2) {
                bitmapMode.get().set(false);
                log.debug("Switched to threshold encoding");
            }
        }

        //if (encoded != null)
        //log.info("Encoded length: {}, Original/encoded ratio: {}", encoded.data().length(), String.format("%.3f", encoded.data().length() * 100.0 / updates.lengthLong()));
        //log.info("Thread: {}; Encoded length: {}", Thread.currentThread().getId(), Arrays.toString(encoded.data().asInt()));

        return encoded;
    }

    @Deprecated
    public INDArray decodeUpdates(INDArray message) {
        // special op should be called here for decoding

        throw new UnsupportedOperationException();
    }

    /**
     * This method does loops encoded data back to updates queue
     * @param message
     */
    protected void sendMessage(INDArray message, int iterationNumber, int epochNumber) {
        //INDArray update = decodeUpdates(message);
        accumulator.receiveUpdate(message);
    }

    @Override
    public boolean broadcastUpdates(INDArray updates, int iterationNumber, int epochNumber) {
        /*
            we want to do 2 things here:
            1) encode updates
            2) send them somewhere
         */
        INDArray message = encodeUpdates(updates);
        if (message != null) {
            sendMessage(message, iterationNumber, epochNumber);
            return true;
        } else
            return false;
    }

    protected void residualDebugOutputIfRequired(INDArray residual){
        if(!encodingDebugMode)
            return;

        double currThreshold = currentThreshold.get().get();
        String currThresholdStr = format(currThreshold);


        INDArray absResidual = Transforms.abs(residual, true);

        double dAmean = absResidual.meanNumber().doubleValue();
        double dAMax = absResidual.maxNumber().doubleValue();
        double dPc50 = absResidual.percentileNumber(50).doubleValue();
        double dPc95 = absResidual.percentileNumber(95).doubleValue();
        double dPc99 = absResidual.percentileNumber(99).doubleValue();
        double dPc999 = absResidual.percentileNumber(99.9).doubleValue();
        double dPc9999 = absResidual.percentileNumber(99.99).doubleValue();

        String amean = format(dAmean).replace('E', 'e');
        String aMax = format(dAMax).replace('E', 'e');
        String pc50 = format(dPc50).replace('E', 'e');
        String pc95 = format(dPc95).replace('E', 'e');
        String pc99 = format(dPc99).replace('E', 'e');
        String pc999 = format(dPc999).replace('E', 'e');
        String pc9999 = format(dPc9999).replace('E', 'e');

        String ameanThr = format(dAmean / currThreshold).replace('E', 'e');
        String aMaxThr = format(dAMax / currThreshold).replace('E', 'e');
        String pc50Thr = format(dPc50 / currThreshold).replace('E', 'e');
        String pc95Thr = format(dPc95 / currThreshold).replace('E', 'e');
        String pc99Thr = format(dPc99 / currThreshold).replace('E', 'e');
        String pc999Thr = format(dPc999 / currThreshold).replace('E', 'e');
        String pc9999Thr = format(dPc9999 / currThreshold).replace('E', 'e');

        long length = absResidual.length();
        long countAbsGTEThreshold = absResidual.gte(currThreshold).sumNumber().longValue();
        double sparsity = countAbsGTEThreshold / (double)length;
        String sparsityStr = format(sparsity);

        log.info("Encoding debug info, residual vector: length: {}, threshold: {}, count > thr: {}, sparsity: {}, amean: {} ({}x); amax: {} ({}x); 50%: {} ({}x); 95%: {} ({}x}; 99%: {} ({}x);  99.9%: {} ({}x); 99.99%: {} ({}x)",
                length, currThresholdStr, countAbsGTEThreshold, sparsityStr,
                amean, ameanThr, aMax, aMaxThr, pc50, pc50Thr,
                pc95, pc95Thr, pc99, pc99Thr, pc999, pc999Thr, pc9999, pc9999Thr);
    }

    protected static ThreadLocal<DecimalFormat> formatter = new ThreadLocal<>();
    protected static ThreadLocal<DecimalFormat> formatter2 = new ThreadLocal<>();

    protected static String format(double d){
        if(d == 0){
            return "0.0";
        }
        if(d >= -0.1 && d < 100){
            if(formatter2.get() == null){
                formatter2.set(new DecimalFormat("0.###"));
            }
            return formatter2.get().format(d);
        }

        if(formatter.get() == null){
            formatter.set(new DecimalFormat("0.###E0"));
        }
        DecimalFormat df = formatter.get();
        return df.format(d).replace('E','e');
    }
}
