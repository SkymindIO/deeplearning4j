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

package org.deeplearning4j.nn.conf.layers;

import lombok.Data;
import lombok.EqualsAndHashCode;
import lombok.NoArgsConstructor;
import lombok.ToString;
import org.deeplearning4j.nn.api.OptimizationAlgorithm;
import org.deeplearning4j.nn.api.OptimizationConfig;
import org.deeplearning4j.nn.conf.GlobalConfiguration;
import org.deeplearning4j.nn.conf.stepfunctions.DefaultStepFunction;
import org.deeplearning4j.nn.conf.stepfunctions.StepFunction;
import org.deeplearning4j.nn.params.PretrainParamInitializer;
import org.nd4j.linalg.lossfunctions.LossFunctions;

@Data
@NoArgsConstructor
@ToString(callSuper = true)
@EqualsAndHashCode(callSuper = true)
public abstract class BasePretrainNetwork extends FeedForwardLayer implements OptimizationConfig {

    protected LossFunctions.LossFunction lossFunction;
    protected double visibleBiasInit;
    private int preTrainIterations;


    //batch size: primarily used for conv nets. Will be reinforced if set.
    protected Boolean miniBatch;
    //number of line search iterations
    protected Integer maxNumLineSearchIterations;
    protected Long seed;
    protected OptimizationAlgorithm optimizationAlgo;
    protected StepFunction stepFunction;
    //minimize or maximize objective
    protected Boolean minimize;

    //Counter for the number of parameter updates so far for this layer.
    //Note that this is only used for pretrain layers (RBM, VAE) - MultiLayerConfiguration and ComputationGraphConfiguration
    //contain counters for standard backprop training.
    // This is important for learning rate schedules, for example, and is stored here to ensure it is persisted
    // for Spark and model serialization
    protected int iterationCount = 0;

    //Counter for the number of epochs completed so far. Used for per-epoch schedules
    protected int epochCount = 0;

    public BasePretrainNetwork(Builder builder) {
        super(builder);
        this.lossFunction = builder.lossFunction;
        this.visibleBiasInit = builder.visibleBiasInit;
        this.preTrainIterations = builder.preTrainIterations;
    }

    @Override
    public int getMaxNumLineSearchIterations(){
        return maxNumLineSearchIterations == null ? 5 : maxNumLineSearchIterations;
    }

    @Override
    public boolean isMiniBatch(){
        return miniBatch == null ? true : miniBatch;
    }

    @Override
    public boolean isMinimize(){
        return minimize;
    }

    @Override
    public void applyGlobalConfiguration(GlobalConfiguration c){
        super.applyGlobalConfiguration(c);
        if(miniBatch == null)
            miniBatch = c.getMiniBatch() != null ? c.getMiniBatch() : true;
        if(maxNumLineSearchIterations == null)
            maxNumLineSearchIterations = c.getMaxNumLineSearchIterations() != null ? c.getMaxNumLineSearchIterations() : 5;
        if(seed == null)
            seed = c.getSeed() == null ? c.getSeed() : System.currentTimeMillis();
        if(optimizationAlgo == null)
            optimizationAlgo = c.getOptimizationAlgo() != null ? c.getOptimizationAlgo() : OptimizationAlgorithm.STOCHASTIC_GRADIENT_DESCENT;
        if(stepFunction == null)
            stepFunction = c.getStepFunction() != null ? c.getStepFunction() : new DefaultStepFunction();
        if(minimize == null)
            minimize = c.getMinimize() != null ? c.getMinimize() : true;
    }

    @Override
    public double getL1ByParam(String paramName) {
        switch (paramName) {
            case PretrainParamInitializer.WEIGHT_KEY:
                return l1;
            case PretrainParamInitializer.BIAS_KEY:
                return l1Bias;
            case PretrainParamInitializer.VISIBLE_BIAS_KEY:
                return l1Bias;
            default:
                throw new IllegalArgumentException("Unknown parameter name: \"" + paramName + "\"");
        }
    }

    @Override
    public double getL2ByParam(String paramName) {
        switch (paramName) {
            case PretrainParamInitializer.WEIGHT_KEY:
                return l2;
            case PretrainParamInitializer.BIAS_KEY:
                return l2Bias;
            case PretrainParamInitializer.VISIBLE_BIAS_KEY:
                return l2Bias;
            default:
                throw new IllegalArgumentException("Unknown parameter name: \"" + paramName + "\"");
        }
    }

    @Override
    public boolean isPretrainParam(String paramName) {
        return PretrainParamInitializer.VISIBLE_BIAS_KEY.equals(paramName);
    }

    public static abstract class Builder<T extends Builder<T>> extends FeedForwardLayer.Builder<T> {
        protected LossFunctions.LossFunction lossFunction = LossFunctions.LossFunction.RECONSTRUCTION_CROSSENTROPY;
        protected double visibleBiasInit = 0.0;
        protected int preTrainIterations = 1;
//        protected Boolean miniBatch = true;
//        protected Integer maxNumLineSearchIterations = 5;
//        protected Long seed;
//        protected OptimizationAlgorithm optimizationAlgo;
//        protected StepFunction stepFunction;
//        //minimize or maximize objective
//        protected Boolean minimize;

        public Builder() {}

        public T lossFunction(LossFunctions.LossFunction lossFunction) {
            this.lossFunction = lossFunction;
            return (T) this;
        }

        public T visibleBiasInit(double visibleBiasInit) {
            this.visibleBiasInit = visibleBiasInit;
            return (T) this;
        }

        public T preTrainIterations(int preTrainIterations) {
            this.preTrainIterations = preTrainIterations;
            return (T) this;
        }

    }
}
