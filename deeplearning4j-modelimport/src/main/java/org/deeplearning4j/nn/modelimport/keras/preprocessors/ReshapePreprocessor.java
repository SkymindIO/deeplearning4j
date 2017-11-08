/*-
 *
 *  * Copyright 2017 Skymind,Inc.
 *  *
 *  *    Licensed under the Apache License, Version 2.0 (the "License");
 *  *    you may not use this file except in compliance with the License.
 *  *    You may obtain a copy of the License at
 *  *
 *  *        http://www.apache.org/licenses/LICENSE-2.0
 *  *
 *  *    Unless required by applicable law or agreed to in writing, software
 *  *    distributed under the License is distributed on an "AS IS" BASIS,
 *  *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  *    See the License for the specific language governing permissions and
 *  *    limitations under the License.
 *
 */
package org.deeplearning4j.nn.modelimport.keras.preprocessors;

import lombok.Data;
import lombok.EqualsAndHashCode;
import lombok.extern.slf4j.Slf4j;
import org.deeplearning4j.nn.api.activations.Activations;
import org.deeplearning4j.nn.api.activations.ActivationsFactory;
import org.deeplearning4j.nn.api.gradients.Gradients;
import org.deeplearning4j.nn.api.gradients.GradientsFactory;
import org.deeplearning4j.nn.conf.inputs.InputType;
import org.deeplearning4j.nn.conf.inputs.InvalidInputTypeException;
import org.deeplearning4j.nn.conf.preprocessor.BaseInputPreProcessor;
import org.nd4j.linalg.api.ndarray.INDArray;

import java.util.Arrays;

/**
 * Generic reshape preprocessor
 *
 * @author Max Pumperla
 */
@Data
@Slf4j
@EqualsAndHashCode(callSuper = true)
public class ReshapePreprocessor extends BaseInputPreProcessor {

    private int[] inputShape;
    private int[] targetShape;
    private boolean hasMiniBatchDimension = false;

    public ReshapePreprocessor(int[] inputShape, int[] targetShape) {
        this.inputShape = inputShape;
        this.targetShape = targetShape;
    }


    private static int prod(int[] array) {
        int prod = 1;
        for (int i : array) {
            prod *= i;
        }
        return prod;
    }

    private static int[] prependMiniBatchSize(int[] shape, int miniBatchSize) {
        int shapeLength = shape.length;
        int[] miniBatchShape = new int[shapeLength + 1];
        for (int i = 0; i < miniBatchShape.length; i++) {
            if (i == 0)
                miniBatchShape[i] = miniBatchSize;
            else
                miniBatchShape[i] = shape[i-1];
        }
        return miniBatchShape;
    }

    @Override
    public Activations preProcess(Activations a, int miniBatchSize, boolean training) {
        if(a.size() != 1){
            throw new IllegalArgumentException("Cannot preprocess input: Activations must have exactly 1 array. Got: "
                    + a.size());
        }
        INDArray input = a.get(0);

        // the target shape read from a keras config does not have mini-batch size
        // included. We prepend it here dynamically.
        if (targetShape.length + 1 == input.shape().length) {
            targetShape = prependMiniBatchSize(targetShape, miniBatchSize);
            inputShape = prependMiniBatchSize(inputShape, miniBatchSize);
            this.hasMiniBatchDimension = true;
        }
        if (prod(input.shape()) == prod((targetShape))) {
            INDArray ret = input.reshape(this.targetShape);
            return ActivationsFactory.getInstance().create(ret, a.getMask(0), a.getMaskState(0));
        } else {
            throw new IllegalStateException("Input shape " + Arrays.toString(input.shape())
                    + " and output shape" + Arrays.toString(inputShape) + " do not match");
        }
    }

    @Override
    public Gradients backprop(Gradients g, int miniBatchSize) {
        if(g.size() != 1){
            throw new IllegalArgumentException("Cannot preprocess activation gradients: Activation gradients must have " +
                    "exactly 1 array. Got: " + g.size());
        }
        INDArray output = g.get(0);

        if (targetShape != output.shape()) {
            throw new IllegalStateException("Unexpected output shape" + Arrays.toString(output.shape())
                    + " (expected to be " + Arrays.toString(targetShape) + ")");
        }
        if (prod(output.shape()) == prod((targetShape))) {
            INDArray ret = output.reshape(this.inputShape);
            return GradientsFactory.getInstance().create(ret, g.getParameterGradients());
        } else {
            throw new IllegalStateException("Output shape" + Arrays.toString(output.shape())
                    + " and input shape" + Arrays.toString(targetShape) + " do not match");
        }
    }

    @Override
    public InputType[] getOutputType(InputType... inputType) throws InvalidInputTypeException {

        int[] shape = hasMiniBatchDimension ? targetShape : prependMiniBatchSize(targetShape, 0);
        InputType ret;
        switch (shape.length) {
            case 2:
                ret = InputType.feedForward(shape[1]);
                break;
            case 3:
                ret = InputType.recurrent(shape[1]);
                break;
            case 4:
                ret = InputType.convolutional(shape[2], shape[3], shape[1]);
                break;
            default:
                throw new UnsupportedOperationException(
                        "Cannot infer input type for reshape array " + Arrays.toString(shape));
        }
        return new InputType[]{ret};
    }
}