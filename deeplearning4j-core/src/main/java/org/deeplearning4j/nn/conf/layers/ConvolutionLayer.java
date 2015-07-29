package org.deeplearning4j.nn.conf.layers;

import lombok.AllArgsConstructor;
import lombok.Data;
import lombok.NoArgsConstructor;

import org.deeplearning4j.nn.conf.distribution.Distribution;
import org.deeplearning4j.nn.weights.WeightInit;
import org.nd4j.linalg.convolution.Convolution;

/**
 * @author Adam Gibson
 */
@Data
@NoArgsConstructor
public class ConvolutionLayer extends FeedForwardLayer {

    private static final long serialVersionUID = 3073633667258683720L;
    protected int[] kernelSize; // Square filter
    protected Convolution.Type convolutionType; // FULL / VALID / SAME

    private ConvolutionLayer(Builder builder) {
    	super(builder);
        this.kernelSize = builder.filterSize;
        this.convolutionType = builder.convolutionType;
    }

    @AllArgsConstructor
    public static class Builder extends FeedForwardLayer.Builder {
        private int[] filterSize; // Square filter
        private Convolution.Type convolutionType; // FULL / VALID / SAME

        public Builder(int[] filterSize) {
            this.filterSize = filterSize;
        }

        @Override
        public Builder nIn(int nIn) {
            super.nIn(nIn);
            return this;
        }

        @Override
        public Builder nOut(int nOut) {
            super.nOut(nOut);
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
        public Builder dropOut(double dropOut) {
            throw new UnsupportedOperationException("ConvolutionLayer Layer does not accept dropout");
        }
        
        @Override
        public Builder dist(Distribution dist){
        	super.dist(dist);
        	return this;
        }

        @Override
        @SuppressWarnings("unchecked")
        public ConvolutionLayer build() {
            return new ConvolutionLayer(this);
        }
    }
}
