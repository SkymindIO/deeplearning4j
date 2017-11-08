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

package org.deeplearning4j.nn.conf;


import org.deeplearning4j.nn.api.activations.Activations;
import org.deeplearning4j.nn.api.gradients.Gradients;
import org.deeplearning4j.nn.conf.inputs.InputType;
import org.nd4j.shade.jackson.annotation.JsonTypeInfo;

import java.io.Serializable;

/**
 * Input pre processor used
 * for pre processing input before passing it
 * to the neural network.
 *
 * @author Adam Gibson
 */
@JsonTypeInfo(use = JsonTypeInfo.Id.CLASS, include = JsonTypeInfo.As.PROPERTY, property = "@class")
public interface InputPreProcessor extends Serializable, Cloneable {


    /**
     * Pre preProcess input/activations for a multi layer network
     * @param input the input to pre preProcess
     * @param miniBatchSize
     * @return the processed input
     */
    Activations preProcess(Activations input, int miniBatchSize, boolean training);

    /**Reverse the preProcess during backprop. Process Gradient/epsilons before
     * passing them to the layer below.
     * @param gradients Activation gradients
     * @param miniBatchSize
     * @return the reverse of the pre preProcess step (if any)
     */
    Gradients backprop(Gradients gradients, int miniBatchSize);

    InputPreProcessor clone();

    /**
     * For a given type of input to this preprocessor, what is the type of the output?
     *
     * @param inputTypes    Type of input for the preprocessor
     * @return             Type of input after applying the preprocessor
     */
    InputType[] getOutputType(InputType... inputTypes);
}
