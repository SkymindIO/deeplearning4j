package org.deeplearning4j.nn.layers;

import lombok.extern.slf4j.Slf4j;
import org.deeplearning4j.nn.api.Layer;
import org.deeplearning4j.nn.api.MaskState;
import org.deeplearning4j.nn.api.activations.Activations;
import org.deeplearning4j.nn.api.gradients.Gradients;
import org.deeplearning4j.nn.api.gradients.GradientsFactory;
import org.deeplearning4j.nn.conf.CacheMode;
import org.deeplearning4j.nn.conf.InputPreProcessor;
import org.deeplearning4j.nn.conf.NeuralNetConfiguration;
import org.deeplearning4j.nn.gradient.DefaultGradient;
import org.deeplearning4j.nn.gradient.Gradient;
import org.deeplearning4j.optimize.api.ConvexOptimizer;
import org.deeplearning4j.optimize.api.IterationListener;
import org.deeplearning4j.util.OneTimeLogger;
import org.nd4j.linalg.api.ndarray.INDArray;
import org.nd4j.linalg.primitives.Pair;

import java.util.Collection;
import java.util.Map;

/**
 * For purposes of transfer learning
 * A frozen layers wraps another dl4j layer within it.
 * The params of the layer within it are "frozen" or in other words held constant
 * During the forward pass the frozen layer behaves as the layer within it would during test regardless of the training/test mode the network is in.
 * Backprop is skipped since parameters are not be updated.
 * @author susaneraly
 */

@Slf4j
public class FrozenLayer implements Layer {

    private Layer insideLayer;
    private boolean logUpdate = false;
    private boolean logFit = false;
    private boolean logTestMode = false;
    private boolean logGradient = false;
    private Gradient zeroGradient;

    public FrozenLayer(Layer insideLayer) {
        this.insideLayer = insideLayer;
        if (insideLayer instanceof OutputLayer) {
            throw new IllegalArgumentException("Output Layers are not allowed to be frozen " + layerId());
        }
        this.insideLayer = insideLayer;
        this.zeroGradient = new DefaultGradient(insideLayer.params());
        if (insideLayer.paramTable() != null) {
            for (String paramType : insideLayer.paramTable().keySet()) {
                //save memory??
                zeroGradient.setGradientFor(paramType, null);
            }
        }
    }

    @Override
    public int numInputs() {
        return 1;
    }

    @Override
    public int numOutputs() {
        return 1;
    }

    @Override
    public void setCacheMode(CacheMode mode) {
        // no-op
    }

    protected String layerId() {
        String name = insideLayer.conf().getLayerName();
        return "(layer name: " + (name == null ? "\"\"" : name) + ", layer index: " + insideLayer.getIndex() + ")";
    }

    @Override
    public double calcL2(boolean backpropOnlyParams) {
        return 0;
    }

    @Override
    public double calcL1(boolean backpropOnlyParams) {
        return 0;
    }

    //FIXME
    @Override
    public Gradients backpropGradient(Gradients epsilon) {
        return GradientsFactory.getInstance().create(null, zeroGradient);
    }

    @Override
    public Activations activate(boolean training) {
        logTestMode(training);
        return insideLayer.activate(false);
    }

    @Override
    public Activations activate(Activations input, boolean training) {
        logTestMode(training);
        return insideLayer.activate(input, false);
    }

    @Override
    public Activations activate(Activations input) {
        return activate(input, false);
    }

    @Override
    public void setInput(Activations activations) {
        insideLayer.setInput(activations);
    }

    @Override
    public Activations getInput() {
        return insideLayer.getInput();
    }

    @Override
    public void update(Gradient gradient) {
        if (!logUpdate) {
            OneTimeLogger.info(log, "Frozen layers will not be updated. Warning will be issued only once per instance");
            logUpdate = true;
        }
        //no op
    }

    @Override
    public String getName() {
        return insideLayer.getName();
    }

    @Override
    public INDArray params() {
        return insideLayer.params();
    }

    @Override
    public int numParams() {
        return insideLayer.numParams();
    }

    @Override
    public int numParams(boolean backwards) {
        return insideLayer.numParams(backwards);
    }

    @Override
    public void setParams(INDArray params) {
        insideLayer.setParams(params);
    }

    @Override
    public void setParamsViewArray(INDArray params) {
        insideLayer.setParamsViewArray(params);
    }

    @Override
    public INDArray getGradientsViewArray() {
        return insideLayer.getGradientsViewArray();
    }

    @Override
    public void setBackpropGradientsViewArray(INDArray gradients) {
        if (!logGradient) {
            OneTimeLogger.info(log,
                            "Gradients for the frozen layer are not set and will therefore will not be updated.Warning will be issued only once per instance");
            logGradient = true;
        }
        //no-op
    }

    @Override
    public org.deeplearning4j.nn.conf.layers.Layer conf() {
        return insideLayer.conf();
    }

    @Override
    public void setConf(org.deeplearning4j.nn.conf.layers.Layer conf) {
        insideLayer.setConf(conf);
    }

    @Override
    public INDArray getParam(String param) {
        return insideLayer.getParam(param);
    }

    @Override
    public Map<String, INDArray> paramTable() {
        return insideLayer.paramTable();
    }

    @Override
    public Map<String, INDArray> paramTable(boolean backpropParamsOnly) {
        return insideLayer.paramTable(backpropParamsOnly);
    }

    @Override
    public void setParamTable(Map<String, INDArray> paramTable) {
        insideLayer.setParamTable(paramTable);
    }

    @Override
    public void setParam(String key, INDArray val) {
        insideLayer.setParam(key, val);
    }

    @Override
    public void clear() {
        insideLayer.clear();
    }

    @Override
    public void applyConstraints(int iteration, int epoch) {
        //No-op
    }

    @Override
    public void setIndex(int index) {
        insideLayer.setIndex(index);
    }

    @Override
    public int getIndex() {
        return insideLayer.getIndex();
    }

    @Override
    public int getIterationCount() {
        return insideLayer.getIterationCount();
    }

    @Override
    public int getEpochCount() {
        return insideLayer.getEpochCount();
    }

    @Override
    public void setIterationCount(int iterationCount) {
        insideLayer.setIterationCount(iterationCount);
    }

    @Override
    public void setEpochCount(int epochCount) {
        insideLayer.setEpochCount(epochCount);
    }

    @Override
    public void setInputMiniBatchSize(int size) {
        insideLayer.setInputMiniBatchSize(size);
    }

    @Override
    public int getInputMiniBatchSize() {
        return insideLayer.getInputMiniBatchSize();
    }

    @Override
    public boolean isPretrainLayer() {
        return insideLayer.isPretrainLayer();
    }

    @Override
    public void clearNoiseWeightParams() {
        insideLayer.clearNoiseWeightParams();
    }

    @Override
    public InputPreProcessor getPreProcessor() {
        return insideLayer.getPreProcessor();
    }

    public void logTestMode(boolean training) {
        if (!training)
            return;
        if (logTestMode) {
            return;
        } else {
            OneTimeLogger.info(log,
                            "Frozen layer instance found! Frozen layers are treated as always in test mode. Warning will only be issued once per instance");
            logTestMode = true;
        }
    }

    public Layer getInsideLayer() {
        return insideLayer;
    }
}


