package org.deeplearning4j.nn.conf;

import static org.junit.Assert.*;

import org.deeplearning4j.nn.conf.distribution.UniformDistribution;
import org.deeplearning4j.nn.conf.layers.DenseLayer;
import org.deeplearning4j.nn.multilayer.MultiLayerNetwork;
import org.deeplearning4j.nn.params.DefaultParamInitializer;
import org.deeplearning4j.nn.weights.WeightInit;
import org.junit.Test;
import org.nd4j.linalg.api.ndarray.INDArray;

/**@author Alex Black 28/07/15
 */
public class LayerwiseConfigurationTest {
	
	@Test
	public void testLayerConfigActivation(){
		//Idea: Set some common values for all layers. Then selectively override
		// the global config, and check they actually work.
		
		//Without layerwise override:
		MultiLayerConfiguration conf = new NeuralNetConfiguration.Builder()
		.activationFunction("relu")
		.list(2)
		.layer(0, new DenseLayer.Builder().nIn(2).nOut(2).build() )
		.layer(1, new DenseLayer.Builder().nIn(2).nOut(2).build() )
		.build();
		MultiLayerNetwork net = new MultiLayerNetwork(conf);
		net.init();
		
		assertTrue(conf.getConf(0).getActivationFunction().equals("relu"));
		assertTrue(conf.getConf(1).getActivationFunction().equals("relu"));
		
		//With:
		conf = new NeuralNetConfiguration.Builder()
			.activationFunction("sigmoid")
			.list(2)
			.layer(0, new DenseLayer.Builder().nIn(2).nOut(2).activation("tanh").build() )
			.layer(1, new DenseLayer.Builder().nIn(2).nOut(2).activation("relu").build() )
			.build();
		
		net = new MultiLayerNetwork(conf);
		net.init();
		
		assertTrue(conf.getConf(0).getActivationFunction().equals("tanh"));
		assertTrue(conf.getConf(1).getActivationFunction().equals("relu"));
	}
	
	@Test
	public void testLayerWeightInit(){
		//Without layerwise override:
		MultiLayerConfiguration conf = new NeuralNetConfiguration.Builder()
		.weightInit(WeightInit.ZERO)
		.list(2)
		.layer(0, new DenseLayer.Builder().nIn(2).nOut(2).build() )
		.layer(1, new DenseLayer.Builder().nIn(2).nOut(2).build() )
		.build();
		MultiLayerNetwork net = new MultiLayerNetwork(conf);
		net.init();
		
		assertTrue(conf.getConf(0).getWeightInit() == WeightInit.ZERO);
		assertTrue(conf.getConf(1).getWeightInit() == WeightInit.ZERO);
		INDArray w0 = net.getLayer(0).getParam(DefaultParamInitializer.WEIGHT_KEY).linearView();
		for( int i=0; i<w0.length(); i++ ) assertTrue(w0.getDouble(i)==0.0);	//Weights should be 0.0
		INDArray w1 = net.getLayer(1).getParam(DefaultParamInitializer.WEIGHT_KEY).linearView();
		for( int i=0; i<w1.length(); i++ ) assertTrue(w1.getDouble(i)==0.0);	//Weights should be 0.0
		
		//With:
		conf = new NeuralNetConfiguration.Builder()
			.activationFunction("sigmoid")
			.weightInit(WeightInit.ZERO)
			.list(2)
			.layer(0, new DenseLayer.Builder().nIn(2).nOut(2).weightInit(WeightInit.DISTRIBUTION).dist(new UniformDistribution(10,11)).build() )
			.layer(1, new DenseLayer.Builder().nIn(2).nOut(2).weightInit(WeightInit.DISTRIBUTION).dist(new UniformDistribution(20,21)).build() )
			.build();
		
		net = new MultiLayerNetwork(conf);
		net.init();
		assertTrue(conf.getConf(0).getWeightInit() == WeightInit.DISTRIBUTION);
		assertTrue(conf.getConf(1).getWeightInit() == WeightInit.DISTRIBUTION);
		assertTrue(conf.getConf(0).getDist() instanceof UniformDistribution);
		assertTrue(conf.getConf(1).getDist() instanceof UniformDistribution);
		UniformDistribution d0 = (UniformDistribution)conf.getConf(0).getDist();
		UniformDistribution d1 = (UniformDistribution)conf.getConf(1).getDist();
		assertTrue(d0.getLower()==10 && d0.getUpper()==11);
		assertTrue(d1.getLower()==20 && d1.getUpper()==21);
		
		w0 = net.getLayer(0).getParam(DefaultParamInitializer.WEIGHT_KEY).linearView();
		for( int i=0; i<w0.length(); i++ ) assertTrue(w0.getDouble(i)>=10.0 && w0.getDouble(i)<=11.0);
		w1 = net.getLayer(1).getParam(DefaultParamInitializer.WEIGHT_KEY).linearView();
		for( int i=0; i<w1.length(); i++ ) assertTrue(w1.getDouble(i)>=20.0 && w0.getDouble(i)<=21.0);
		
		
		conf = new NeuralNetConfiguration.Builder()
			.activationFunction("sigmoid")
			.weightInit(WeightInit.DISTRIBUTION).dist(new UniformDistribution(-30,-20))
			.list(2)
			.layer(0, new DenseLayer.Builder().nIn(2).nOut(2).build() )
			.layer(1, new DenseLayer.Builder().nIn(2).nOut(2).build() )
			.build();
		net = new MultiLayerNetwork(conf);
		net.init();
		assertTrue(conf.getConf(0).getWeightInit() == WeightInit.DISTRIBUTION);
		assertTrue(conf.getConf(1).getWeightInit() == WeightInit.DISTRIBUTION);
		assertTrue(conf.getConf(0).getDist() instanceof UniformDistribution);
		assertTrue(conf.getConf(1).getDist() instanceof UniformDistribution);
	}
	
	@Test
	public void testLayerDropout(){
		//Without layerwise override:
		MultiLayerConfiguration conf = new NeuralNetConfiguration.Builder()
		.dropOut(0.47)
		.list(2)
		.layer(0, new DenseLayer.Builder().nIn(2).nOut(2).build() )
		.layer(1, new DenseLayer.Builder().nIn(2).nOut(2).build() )
		.build();
		MultiLayerNetwork net = new MultiLayerNetwork(conf);
		net.init();
		
		assertTrue(conf.getConf(0).getDropOut() == 0.47);
		assertTrue(conf.getConf(1).getDropOut() == 0.47);
		
		//With:
		conf = new NeuralNetConfiguration.Builder()
			.dropOut(0.5)
			.list(3)
			.layer(0, new DenseLayer.Builder().nIn(2).nOut(2).dropOut(0.2).build() )
			.layer(1, new DenseLayer.Builder().nIn(2).nOut(2).dropOut(0.4).build() )
			.layer(2, new DenseLayer.Builder().nIn(2).nOut(2).dropOut(0.0).build() )
			.build();
		
		net = new MultiLayerNetwork(conf);
		net.init();
		
		assertTrue(conf.getConf(0).getDropOut()==0.2);
		assertTrue(conf.getConf(1).getDropOut()==0.4);
		assertTrue(conf.getConf(2).getDropOut()==0.0);
	}

}
