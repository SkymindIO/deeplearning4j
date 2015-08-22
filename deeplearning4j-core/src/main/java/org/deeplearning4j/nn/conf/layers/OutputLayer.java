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

package org.deeplearning4j.nn.conf.layers;

import lombok.*;
import org.deeplearning4j.nn.conf.Updater;
import org.deeplearning4j.nn.conf.distribution.Distribution;
import org.deeplearning4j.nn.weights.WeightInit;
import org.nd4j.linalg.lossfunctions.LossFunctions.LossFunction;

/**
 * Output layer with different objective co-occurrences for different objectives.
 * This includes classification as well as prediction
 *
 */
@Data @NoArgsConstructor
@ToString(callSuper = true)
@EqualsAndHashCode(callSuper = true)
public class OutputLayer extends FeedForwardLayer {
    private LossFunction lossFunction;

    private OutputLayer(Builder builder) {
    	super(builder);
        this.lossFunction = builder.lossFunction;
    }

    @AllArgsConstructor
    public static class Builder extends FeedForwardLayer.Builder {
        private LossFunction lossFunction = LossFunction.RMSE_XENT;

        public Builder() {}

        public Builder loss(LossFunction lossFunction) {
            this.lossFunction = lossFunction;
            return this;
        }

        @Override
        public Builder nIn(int nIn) {
            this.nIn = nIn;
            return this;
        }
        @Override
        public Builder nOut(int nOut) {
            this.nOut = nOut;
            return this;
        }
        @Override
        public Builder activation(String activationFunction) {
            this.activationFunction = activationFunction;
            return this;
        }
        @Override
        public Builder weightInit(WeightInit weightInit) {
            this.weightInit = weightInit;
            return this;
        }
        
        @Override
        public Builder dist(Distribution dist){
        	super.dist(dist);
        	return this;
        }
        
        @Override
        public Builder dropOut(double dropOut) {
            this.dropOut = dropOut;
            return this;
        }
        
        @Override
        public Builder updater(Updater updater){
        	this.updater = updater;
        	return this;
        }
        
        @Override
        @SuppressWarnings("unchecked")
        public OutputLayer build() {
            return new OutputLayer(this);
        }

    }
}

