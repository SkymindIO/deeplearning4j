package org.deeplearning4j.nn.api;

import org.deeplearning4j.optimize.api.StepFunction;

public interface OptimizationConfig {

    boolean isMiniBatch();

    int getMaxNumLineSearchIterations();

    OptimizationAlgorithm getOptimizationAlgo();

    org.deeplearning4j.nn.conf.stepfunctions.StepFunction getStepFunction();

    boolean isMinimize();

    int getIterationCount();

    int getEpochCount();

    void setIterationCount(int iterationCount);

    void setEpochCount(int epochCount);

}
