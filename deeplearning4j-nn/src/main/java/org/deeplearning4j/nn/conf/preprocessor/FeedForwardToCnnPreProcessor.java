/*-
 *
 *  * Copyright 2015 Skymind,Inc.
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

package org.deeplearning4j.nn.conf.preprocessor;

import lombok.*;
import org.deeplearning4j.nn.api.MaskState;
import org.deeplearning4j.nn.api.activations.Activations;
import org.deeplearning4j.nn.api.activations.ActivationsFactory;
import org.deeplearning4j.nn.api.gradients.Gradients;
import org.deeplearning4j.nn.api.gradients.GradientsFactory;
import org.deeplearning4j.nn.conf.InputPreProcessor;
import org.deeplearning4j.nn.conf.inputs.InputType;
import org.nd4j.linalg.api.ndarray.INDArray;
import org.nd4j.linalg.api.shape.Shape;
import org.nd4j.linalg.primitives.Pair;
import org.nd4j.linalg.util.ArrayUtil;
import org.nd4j.shade.jackson.annotation.JsonCreator;
import org.nd4j.shade.jackson.annotation.JsonProperty;

import java.util.Arrays;

/**
 * A preprocessor to allow CNN and standard feed-forward network layers to be used together.<br>
 * For example, DenseLayer -> CNN<br>
 * This does two things:<br>
 * (a) Reshapes activations out of FeedFoward layer (which is 2D or 3D with shape
 * [numExamples, inputHeight*inputWidth*numChannels]) into 4d activations (with shape
 * [numExamples, numChannels, inputHeight, inputWidth]) suitable to feed into CNN layers.<br>
 * (b) Reshapes 4d epsilons (weights*deltas) from CNN layer, with shape
 * [numExamples, numChannels, inputHeight, inputWidth]) into 2d epsilons (with shape
 * [numExamples, inputHeight*inputWidth*numChannels]) for use in feed forward layer
 * Note: numChannels is equivalent to depth or featureMaps referenced in different literature
 *
 * @author Adam Gibson
 * @see CnnToFeedForwardPreProcessor for opposite case (i.e., CNN -> DenseLayer etc)
 */
@Data
@EqualsAndHashCode(exclude = {"shape"})
public class FeedForwardToCnnPreProcessor implements InputPreProcessor {
    private int inputHeight;
    private int inputWidth;
    private int numChannels;

    @Getter(AccessLevel.NONE)
    @Setter(AccessLevel.NONE)
    private int[] shape;

    /**
     * Reshape to a channels x rows x columns tensor
     *
     * @param inputHeight the columns
     * @param inputWidth  the rows
     * @param numChannels the channels
     */
    @JsonCreator
    public FeedForwardToCnnPreProcessor(@JsonProperty("inputHeight") int inputHeight,
                    @JsonProperty("inputWidth") int inputWidth, @JsonProperty("numChannels") int numChannels) {
        this.inputHeight = inputHeight;
        this.inputWidth = inputWidth;
        this.numChannels = numChannels;
    }

    public FeedForwardToCnnPreProcessor(int inputWidth, int inputHeight) {
        this.inputHeight = inputHeight;
        this.inputWidth = inputWidth;
        this.numChannels = 1;
    }

    @Override
    public Activations preProcess(Activations a, int miniBatchSize, boolean training) {
        if(a.size() != 1){
            throw new IllegalArgumentException("Cannot preprocess input: Activations must have exactly 1 array. Got: "
                    + a.size());
        }
        INDArray input = a.get(0);

        if (input.ordering() != 'c' || !Shape.strideDescendingCAscendingF(input))
            input = input.dup('c');

        this.shape = input.shape();
        if (input.shape().length == 4)
            return a;
        if (input.columns() != inputWidth * inputHeight * numChannels)
            throw new IllegalArgumentException("Invalid input: expect output columns must be equal to rows "
                            + inputHeight + " x columns " + inputWidth + " x channels " + numChannels
                            + " but was instead " + Arrays.toString(input.shape()));

        INDArray ret = input.reshape('c', input.size(0), numChannels, inputHeight, inputWidth);
        Pair<INDArray, MaskState> p = feedForwardMaskArray(a.getMask(0), a.getMaskState(0), miniBatchSize);
        return ActivationsFactory.getInstance().create(ret, p.getFirst(), p.getSecond());
    }

    @Override
    // return 4 dimensions
    public Gradients backprop(Gradients g, int miniBatchSize) {
        if(g.size() != 1){
            throw new IllegalArgumentException("Cannot preprocess activation gradients: Activation gradients must have " +
                    "exactly 1 array. Got: " + g.size());
        }
        INDArray epsilons = g.get(0);
        if (epsilons.ordering() != 'c' || !Shape.strideDescendingCAscendingF(epsilons))
            epsilons = epsilons.dup('c');

        if (shape == null || ArrayUtil.prod(shape) != epsilons.length()) {
            if (epsilons.rank() == 2)
                return g; //should never happen

            return GradientsFactory.getInstance().create(
                    epsilons.reshape('c', epsilons.size(0), numChannels, inputHeight, inputWidth),
                    g.getParameterGradients());
        }

        return GradientsFactory.getInstance().create( epsilons.reshape('c', shape), g.getParameterGradients());
    }


    @Override
    public FeedForwardToCnnPreProcessor clone() {
        try {
            FeedForwardToCnnPreProcessor clone = (FeedForwardToCnnPreProcessor) super.clone();
            if (clone.shape != null)
                clone.shape = clone.shape.clone();
            return clone;
        } catch (CloneNotSupportedException e) {
            throw new RuntimeException(e);
        }
    }

    @Override
    public InputType[] getOutputType(InputType... inputTypes) {

        if (inputTypes == null || inputTypes.length != 1 ) {
            throw new IllegalStateException("Invalid input type: " + (inputTypes == null ? null : Arrays.toString(inputTypes)));
        }

        InputType inputType = inputTypes[0];
        InputType ret;
        switch (inputType.getType()) {
            case FF:
                InputType.InputTypeFeedForward c = (InputType.InputTypeFeedForward) inputType;
                int expSize = inputHeight * inputWidth * numChannels;
                if (c.getSize() != expSize) {
                    throw new IllegalStateException("Invalid input: expected FeedForward input of size " + expSize
                                    + " = (d=" + numChannels + " * w=" + inputWidth + " * h=" + inputHeight + "), got "
                                    + inputType);
                }
                ret = InputType.convolutional(inputHeight, inputWidth, numChannels);
                break;
            case CNN:
                InputType.InputTypeConvolutional c2 = (InputType.InputTypeConvolutional) inputType;

                if (c2.getDepth() != numChannels || c2.getHeight() != inputHeight || c2.getWidth() != inputWidth) {
                    throw new IllegalStateException("Invalid input: Got CNN input type with (d,w,h)=(" + c2.getDepth()
                                    + "," + c2.getWidth() + "," + c2.getHeight() + ") but expected (" + numChannels
                                    + "," + inputHeight + "," + inputWidth + ")");
                }
                ret = c2;
                break;
            case CNNFlat:
                InputType.InputTypeConvolutionalFlat c3 = (InputType.InputTypeConvolutionalFlat) inputType;
                if (c3.getDepth() != numChannels || c3.getHeight() != inputHeight || c3.getWidth() != inputWidth) {
                    throw new IllegalStateException("Invalid input: Got CNN input type with (d,w,h)=(" + c3.getDepth()
                                    + "," + c3.getWidth() + "," + c3.getHeight() + ") but expected (" + numChannels
                                    + "," + inputHeight + "," + inputWidth + ")");
                }
                ret = c3.getUnflattenedType();
                break;
            default:
                throw new IllegalStateException("Invalid input type: got " + inputType);
        }
        return new InputType[]{ret};
    }


    public Pair<INDArray, MaskState> feedForwardMaskArray(INDArray maskArray, MaskState currentMaskState,
                    int minibatchSize) {
        //Pass-through, unmodified (assuming here that it's a 1d mask array - one value per example)
        return new Pair<>(maskArray, currentMaskState);
    }

}
