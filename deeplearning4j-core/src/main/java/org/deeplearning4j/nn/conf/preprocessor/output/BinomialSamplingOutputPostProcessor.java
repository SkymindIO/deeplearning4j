/*
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

package org.deeplearning4j.nn.conf.preprocessor.output;


import org.nd4j.linalg.api.ndarray.INDArray;
import org.nd4j.linalg.factory.Nd4j;

/**
 * Binomial sampling pre processor
 * @author Adam Gibson
 */
public class BinomialSamplingOutputPostProcessor extends BaseOutputPostProcessor {

    @Override
    public INDArray preProcess(INDArray output) {
        return Nd4j.getDistributions().createBinomial(1,output).sample(output.shape());
    }

    @Override
    public INDArray backprop(INDArray input) {
        // TODO - How to reverse?
        return null;
    }




}
