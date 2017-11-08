package org.deeplearning4j.nn.conf.preprocessor;

import lombok.*;
import org.deeplearning4j.nn.api.MaskState;
import org.deeplearning4j.nn.api.activations.Activations;
import org.deeplearning4j.nn.api.activations.ActivationsFactory;
import org.deeplearning4j.nn.api.gradients.Gradients;
import org.deeplearning4j.nn.api.gradients.GradientsFactory;
import org.deeplearning4j.nn.conf.InputPreProcessor;
import org.deeplearning4j.nn.conf.inputs.InputType;
import org.deeplearning4j.util.TimeSeriesUtils;
import org.nd4j.linalg.api.ndarray.INDArray;
import org.nd4j.linalg.primitives.Pair;
import org.nd4j.linalg.util.ArrayUtil;
import org.nd4j.shade.jackson.annotation.JsonProperty;

import java.util.Arrays;

/**
 * A preprocessor to allow RNN and CNN layers to be used together<br>
 * For example, time series (video) input -> ConvolutionLayer, or conceivable GravesLSTM -> ConvolutionLayer<br>
 * Functionally equivalent to combining RnnToFeedForwardPreProcessor + FeedForwardToCnnPreProcessor<br>
 * Specifically, this does two things:<br>
 * (a) Reshape 3d activations out of RNN layer, with shape [miniBatchSize, numChannels*inputHeight*inputWidth, timeSeriesLength])
 * into 4d (CNN) activations (with shape [numExamples*timeSeriesLength, numChannels, inputWidth, inputHeight]) <br>
 * (b) Reshapes 4d epsilons (weights.*deltas) out of CNN layer (with shape
 * [numExamples*timeSeriesLength, numChannels, inputHeight, inputWidth]) into 3d epsilons with shape
 * [miniBatchSize, numChannels*inputHeight*inputWidth, timeSeriesLength] suitable to feed into CNN layers.
 * Note: numChannels is equivalent to depth or featureMaps referenced in different literature
 *
 * @author Alex Black
 */
@Data
@EqualsAndHashCode(exclude = {"product"})
public class RnnToCnnPreProcessor implements InputPreProcessor {

    private int inputHeight;
    private int inputWidth;
    private int numChannels;

    @Getter(AccessLevel.NONE)
    @Setter(AccessLevel.NONE)
    private int product;

    public RnnToCnnPreProcessor(@JsonProperty("inputHeight") int inputHeight,
                    @JsonProperty("inputWidth") int inputWidth, @JsonProperty("numChannels") int numChannels) {
        this.inputHeight = inputHeight;
        this.inputWidth = inputWidth;
        this.numChannels = numChannels;
        this.product = inputHeight * inputWidth * numChannels;
    }


    @Override
    public Activations preProcess(Activations a, int miniBatchSize, boolean training) {
        if(a.size() != 1){
            throw new IllegalArgumentException("Cannot preprocess input: Activations must have exactly 1 array. Got: "
                    + a.size());
        }
        INDArray input = a.get(0);
        if (input.ordering() == 'c')
            input = input.dup('f');
        //Input: 3d activations (RNN)
        //Output: 4d activations (CNN)
        int[] shape = input.shape();
        INDArray in2d;
        if (shape[0] == 1) {
            //Edge case: miniBatchSize = 1
            in2d = input.tensorAlongDimension(0, 1, 2).permutei(1, 0);
        } else if (shape[2] == 1) {
            //Edge case: time series length = 1
            in2d = input.tensorAlongDimension(0, 1, 0);
        } else {
            INDArray permuted = input.permute(0, 2, 1); //Permute, so we get correct order after reshaping
            in2d = permuted.reshape('f', shape[0] * shape[2], shape[1]);
        }

        INDArray ret = in2d.dup('c').reshape('c', shape[0] * shape[2], numChannels, inputHeight, inputWidth);

        Pair<INDArray, MaskState> p = feedForwardMaskArray(a.getMask(0), a.getMaskState(0), miniBatchSize);
        return ActivationsFactory.getInstance().create(ret, p.getFirst(), p.getSecond());
    }

    @Override
    public Gradients backprop(Gradients g, int miniBatchSize) {
        if(g.size() != 1){
            throw new IllegalArgumentException("Cannot preprocess activation gradients: Activation gradients must have " +
                    "exactly 1 array. Got: " + g.size());
        }
        INDArray output = g.get(0);

        //Input: 4d epsilons (CNN)
        //Output: 3d epsilons (RNN)
        if (output.ordering() == 'f')
            output = output.dup('c');
        int[] shape = output.shape();
        //First: reshape 4d to 2d
        INDArray twod = output.reshape('c', output.size(0), ArrayUtil.prod(output.shape()) / output.size(0));
        //Second: reshape 2d to 4d
        INDArray reshaped = twod.dup('f').reshape('f', miniBatchSize, shape[0] / miniBatchSize, product);
        INDArray ret = reshaped.permute(0, 2, 1);
        return GradientsFactory.getInstance().create(ret, g.getParameterGradients());
    }

    @Override
    public RnnToCnnPreProcessor clone() {
        return new RnnToCnnPreProcessor(inputHeight, inputWidth, numChannels);
    }

    @Override
    public InputType[] getOutputType(InputType... inputType) {
        if (inputType == null || inputType.length != 1 || inputType[0].getType() != InputType.Type.RNN) {
            throw new IllegalStateException("Invalid input type: Expected input of type RNN, got "
                    + (inputType == null ? null : Arrays.toString(inputType)));
        }

        InputType.InputTypeRecurrent c = (InputType.InputTypeRecurrent) inputType[0];
        int expSize = inputHeight * inputWidth * numChannels;
        if (c.getSize() != expSize) {
            throw new IllegalStateException("Invalid input: expected RNN input of size " + expSize + " = (d="
                            + numChannels + " * w=" + inputWidth + " * h=" + inputHeight + "), got " + inputType);
        }

        return new InputType[]{InputType.convolutional(inputHeight, inputWidth, numChannels)};
    }


    public Pair<INDArray, MaskState> feedForwardMaskArray(INDArray maskArray, MaskState currentMaskState,
                    int minibatchSize) {
        //Assume mask array is 2d for time series (1 value per time step)
        if (maskArray == null) {
            return new Pair<>(maskArray, currentMaskState);
        } else if (maskArray.rank() == 2) {
            //Need to reshape mask array from [minibatch,timeSeriesLength] to [minibatch*timeSeriesLength, 1]
            return new Pair<>(TimeSeriesUtils.reshapeTimeSeriesMaskToVector(maskArray), currentMaskState);
        } else {
            throw new IllegalArgumentException("Received mask array of rank " + maskArray.rank()
                            + "; expected rank 2 mask array. Mask array shape: " + Arrays.toString(maskArray.shape()));
        }
    }
}
