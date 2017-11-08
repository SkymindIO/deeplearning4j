package org.deeplearning4j.nn.conf.preprocessor;

import lombok.Data;
import lombok.NoArgsConstructor;
import org.deeplearning4j.nn.api.MaskState;
import org.deeplearning4j.nn.api.activations.Activations;
import org.deeplearning4j.nn.api.activations.ActivationsFactory;
import org.deeplearning4j.nn.api.gradients.Gradients;
import org.deeplearning4j.nn.api.gradients.GradientsFactory;
import org.deeplearning4j.nn.conf.InputPreProcessor;
import org.deeplearning4j.nn.conf.inputs.InputType;
import org.deeplearning4j.util.TimeSeriesUtils;
import org.nd4j.linalg.api.ndarray.INDArray;
import org.nd4j.linalg.api.shape.Shape;
import org.nd4j.linalg.primitives.Pair;

import java.util.Arrays;

/**
 * A preprocessor to allow RNN and feed-forward network layers to be used together.<br>
 * For example, DenseLayer -> GravesLSTM<br>
 * This does two things:<br>
 * (a) Reshapes activations out of FeedFoward layer (which is 2D with shape
 * [miniBatchSize*timeSeriesLength,layerSize]) into 3d activations (with shape
 * [miniBatchSize,layerSize,timeSeriesLength]) suitable to feed into RNN layers.<br>
 * (b) Reshapes 3d epsilons (weights*deltas from RNN layer, with shape
 * [miniBatchSize,layerSize,timeSeriesLength]) into 2d epsilons (with shape
 * [miniBatchSize*timeSeriesLength,layerSize]) for use in feed forward layer
 *
 * @author Alex Black
 * @see RnnToFeedForwardPreProcessor for opposite case (i.e., GravesLSTM -> DenseLayer etc)
 */
@Data
@NoArgsConstructor
public class FeedForwardToRnnPreProcessor implements InputPreProcessor {

    @Override
    public Activations preProcess(Activations a, int miniBatchSize, boolean training) {
        if(a.size() != 1){
            throw new IllegalArgumentException("Cannot preprocess input: Activations must have exactly 1 array. Got: "
                    + a.size());
        }
        INDArray input = a.get(0);
        //Need to reshape FF activations (2d) activations to 3d (for input into RNN layer)
        if (input.rank() != 2)
            throw new IllegalArgumentException(
                            "Invalid input: expect NDArray with rank 2 (i.e., activations for FF layer)");
        if (input.ordering() == 'c')
            input = Shape.toOffsetZeroCopy(input, 'f');

        int[] shape = input.shape();
        INDArray reshaped = input.reshape('f', miniBatchSize, shape[0] / miniBatchSize, shape[1]);
        INDArray ret = reshaped.permute(0, 2, 1);
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
        //Need to reshape RNN epsilons (3d) to 2d (for use in FF layer backprop calculations)
        if (output.rank() != 3)
            throw new IllegalArgumentException(
                            "Invalid input: expect NDArray with rank 3 (i.e., epsilons from RNN layer)");
        if (output.ordering() != 'f')
            output = output.dup('f');
        int[] shape = output.shape();
        INDArray ret = null;
        if (shape[0] == 1) {
            ret = output.tensorAlongDimension(0, 1, 2).permutei(1, 0); //Edge case: miniBatchSize==1
        } else if (shape[2] == 1) {
            ret = output.tensorAlongDimension(0, 1, 0); //Edge case: timeSeriesLength=1
        }

        if(ret == null) {
            INDArray permuted = output.permute(0, 2, 1); //Permute, so we get correct order after reshaping
            ret = permuted.reshape('f', shape[0] * shape[2], shape[1]);
        }

        return GradientsFactory.getInstance().create(ret, g.getParameterGradients());
    }

    @Override
    public FeedForwardToRnnPreProcessor clone() {
        try {
            FeedForwardToRnnPreProcessor clone = (FeedForwardToRnnPreProcessor) super.clone();
            return clone;
        } catch (CloneNotSupportedException e) {
            throw new RuntimeException(e);
        }
    }

    @Override
    public InputType[] getOutputType(InputType... inputType) {
        if (inputType == null || inputType.length != 1 || (inputType[0].getType() != InputType.Type.FF
                        && inputType[0].getType() != InputType.Type.CNNFlat)) {
            throw new IllegalStateException("Invalid input: expected input of type FeedForward, got "
                    + (inputType == null ? null : Arrays.toString(inputType)));
        }

        if (inputType[0].getType() == InputType.Type.FF) {
            InputType.InputTypeFeedForward ff = (InputType.InputTypeFeedForward) inputType[0];
            return new InputType[]{InputType.recurrent(ff.getSize())};
        } else {
            InputType.InputTypeConvolutionalFlat cf = (InputType.InputTypeConvolutionalFlat) inputType[0];
            return new InputType[]{InputType.recurrent(cf.getFlattenedSize())};
        }
    }

    public Pair<INDArray, MaskState> feedForwardMaskArray(INDArray maskArray, MaskState currentMaskState,
                    int minibatchSize) {
        //Assume mask array is 1d - a mask array that has been reshaped from [minibatch,timeSeriesLength] to [minibatch*timeSeriesLength, 1]
        if (maskArray == null) {
            return new Pair<>(maskArray, currentMaskState);
        } else if (maskArray.isVector()) {
            //Need to reshape mask array from [minibatch*timeSeriesLength, 1] to [minibatch,timeSeriesLength]
            return new Pair<>(TimeSeriesUtils.reshapeVectorToTimeSeriesMask(maskArray, minibatchSize),
                            currentMaskState);
        } else {
            throw new IllegalArgumentException("Received mask array with shape " + Arrays.toString(maskArray.shape())
                            + "; expected vector.");
        }
    }
}
