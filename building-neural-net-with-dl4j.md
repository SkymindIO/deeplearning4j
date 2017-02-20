---
layout: default
title: Building Neural Networks with DeepLearning4J
---

# Building Neural Networks with DeepLearning4J

Contents

* [MultiLayer Network and Computation Graph](#multilayer)
* [Configuration](#configuration)
* [Configuration Details](#configurationdetails)
* [Attaching a UI to your Network](#ui)


## <a name="multilayer">MultiLayerNetwork and Computationgraph</a>

Deeplearning4J has two classes for building and training Neural Networks, MultiLayerNetwork and ComputationGraph. 

### MultiLayerNetwork

MultiLayerNetwork is a neural network with multiple layers in a stack, and usually an output layer. 

MultiLayerNetwork is trainable via backpropagation, with optional pretraining, depending on the type of layers it contains.

### ComputationGraph

ComputationGraph is for neural networks with a more complex connection architecture. ComputationGraph which allows for an arbitrary directed acyclic graph connection structure between layers. 

A ComputationGraph may also have an arbitrary number of inputs and outputs.

## <a name="configuration">Configuration</a>

To use either ComputationGraph or MultiLayerNetwork you typically start with the Configuration classes for each. 

Both configuration classes provide a convenient builder architecture. 

### ComputationGraph Configuration

Here is a configuration of a Recurrent Neural Network taken from our examples. 

```
ComputationGraphConfiguration configuration = new NeuralNetConfiguration.Builder()
   	.weightInit(WeightInit.XAVIER)
	.learningRate(0.5)
	.updater(Updater.RMSPROP)
	.optimizationAlgo(OptimizationAlgorithm.STOCHASTIC_GRADIENT_DESCENT).iterations(nIterations)
	.seed(seed)
	.graphBuilder()
	.addInputs("additionIn", "sumOut")
	.setInputTypes(InputType.recurrent(FEATURE_VEC_SIZE), InputType.recurrent(FEATURE_VEC_SIZE))
	.addLayer("encoder", new GravesLSTM.Builder().nIn(FEATURE_VEC_SIZE).nOut(numHiddenNodes).activation("softsign").build(),"additionIn")
	.addVertex("lastTimeStep", new LastTimeStepVertex("additionIn"), "encoder")
	.addVertex("duplicateTimeStep", new DuplicateToTimeSeriesVertex("sumOut"), "lastTimeStep")
	.addLayer("decoder", new GravesLSTM.Builder().nIn(FEATURE_VEC_SIZE+numHiddenNodes).nOut(numHiddenNodes).activation("softsign").build(), "sumOut","duplicateTimeStep")
	.addLayer("output", new RnnOutputLayer.Builder().nIn(numHiddenNodes).nOut(FEATURE_VEC_SIZE).activation("softmax").lossFunction(LossFunctions.LossFunction.MCXENT).build(), "decoder")
	.setOutputs("output")
	.pretrain(false).backprop(true)
	.build();
				
```				

### MultiLayerNetwork Configuration

Here is a configuration of a simple FeedForward Network also from our examples. 

```
MultiLayerConfiguration conf = new NeuralNetConfiguration.Builder()
	.seed(seed)
    .iterations(iterations)
	 .activation("tanh")
	 .weightInit(WeightInit.XAVIER)
	 .learningRate(0.1)
	 .regularization(true).l2(1e-4)
	 .list()
	 .layer(0, new DenseLayer.Builder().nIn(numInputs).nOut(3)
	     .build())
	 .layer(1, new DenseLayer.Builder().nIn(3).nOut(3)
	     .build())
	 .layer(2, new OutputLayer.Builder(LossFunctions.LossFunction.NEGATIVELOGLIKELIHOOD)
	     .activation("softmax")
	     .nIn(3).nOut(outputNum).build())
	 .backprop(true).pretrain(false)
	 .build();
			
			
```			


## <a name="configurationdetails">Configuration Details</a>

### Seed

In both network types it is typical to set a random seed. Hard coding the random seed allows for repeatable results. The random seed is used for initialization of the weights of the neural network. 

### Iterations

An iteration is simply one update of the neural net model’s parameters. 

Not to be confused with an epoch which is one complete pass through the dataset. 

Many iterations can occur before an epoch is over. Epoch and iteration are only synonymous if you update your parameters once for each pass through the whole dataset; if you update using mini-batches, they mean different things. Say your data has 2 minibatches: A and B. .numIterations(3) performs training like AAABBB, while 3 epochs looks like ABABAB.

### Activation


The activation function determines what output a node will generate based upon its input. Sigmoid activation functions had been very popular, ReLU is currently very popular. In DeepLearnging4J the activation function is set at the layer level and applies to all neurons in that layer.


Configuring an activation function

```
layer(2, new OutputLayer.Builder(LossFunctions.LossFunction.NEGATIVELOGLIKELIHOOD).activation(Activation.SOFTMAX)
``` 

### Output Layer Activation

The activation function of the ouptut layer determines the output. The question you are asking your network determines the choice of your output layer Activation function.

Some examples. 

To generate Binary(0,1) output use a sigmoid activation function on the output layer. Output will be between 0, and 1. Treat it as probability of 0 or 1. 

To generate output of categorical Targets (1 of C coding) use Softmax Activation Function forthe  output layer. St the number of output nodes set to number of classes. Treat output as probability per label. 

To generate Continuous-Valued (bounded range) values use Sigmoid or Tanh Activation (with scaled range of outputs to range of targets).

To generate positive (no known upper bound) output use  ReLU-family Activation Function, Softplus Activation Function
(or use logarithmic normalization to transform to unbounded continuous)

To generate continuous-Valued (Not bounded) output use a Linear Activation Function (equates to no activation function)

### Supported Activation Functions


DeepLearning4J supports the following Activation functions

* CUBE
* ELU
* HARDSIGMOID
* HARDTANH
* IDENTITY
* LEAKYRELU
* RATIONALTANH
* RELU
* RRELU
* SIGMOID
* SOFTMAX
* SOFTPLUS
* SOFTSIGN
* TANH

### Custom Activation Functions

DeepLearning4J supports custom Activation Functions. 

### Weight Initialization

At startup the weights of the Neural Network are randomly initialized. Randomization is important, if the weights of two neurons are identical then they will never be able to diverge and learn different features. Xavier Initialization is widely used and is described in our [glossary](https://deeplearning4j.org/glossary.html#xavier)

### Learning Rate

Learning Rate determines how big of a step towards less error when weights are updated. Too big of a Learning rate and the error may fluctuate widely or fail to converge. Too small of a Learning rate and the network will take an unnecessarily long time to train. A high learning rate that is working well initially often had to be adjusted to a lower rate as the model trained. Adaptive updaters such as ADAM or AdaGrad are used to deal with the need to adjust the speed of learning. 


### Backpropagation

Most effective methods for training Neural networks use backpropagation or some variant. 

### Pretrain

For unsupervised training for specialty networks like rbms and autoencoders it may be useful to set pretrain to true. For other networks it should be set to false. 

### Regularization 

Regularization is way to fight the tendency of Neural Networks to overfit the training data. Overfitting is when the network learns to be very accurate on the training data but fails to generalize well and performs poorly on the test data. 

Deeplearning4J supports both L1 and L2 Regularization.  

### Adding Layers

To add a layer to your configuration use addLayer for ComputationGraph and .layer for MultiLayerNetworks. Activation functions are set per layer. Loss Function is set for the output Layer. 


### Updaters

DL4J support the following Updaters

* ADADELTA
* ADAGRAD
* ADAM
* NESTEROVS
* NONE
* RMSPROP
* SGD
* CONJUGATE GRADIENT
* HESSIAN FREE
* LBFGS
* LINE GRADIENT DESCENT

The JavaDoc for updaters is part of the DeepLearning4J JavaDoc and is available [here.](https://deeplearning4j.org/doc/org/deeplearning4j/nn/conf/Updater.html)



### Animation of Updaters 

Thanks to Alec Radford for allowing us to use these animations

#### Updater progress on complex error surface

![Alt text](./img/updater_animation.gif)
<br />
<br />

#### Updater progress on a less complex error surface

![Alt text](./img/updater_animation2.gif)





### Listeners 

Listeners that gather statistics or report progress can be attached. Here is a code example of applying a Listener to a model, note that you can provide setListeners multiple Listeners. 

```
model.setListeners(new ScoreIterationListener(100));
```

One Example use of a listener is to print out accuracy to the console as the network trains. 

Another example is to deliver statistics to the User interface. 

```

UIServer uiServer = UIServer.getInstance();
        StatsStorage statsStorage = new InMemoryStatsStorage();
        model.setListeners(new StatsListener(statsStorage),new ScoreIterationListener(1));
        uiServer.attach(statsStorage);
```		


#### CollectScoresIterationListener	
CollectScoresIterationListener simply stores the model scores internally (along with the iteration) every 1 or N iterations (this is configurable).

#### ComposableIterationListener	
A group of listeners

### ParamAndGradientIterationListener	
An iteration listener that provides details on parameters and gradients at each iteration during traning.

### PerformanceListener	
Simple IterationListener that tracks time spend on training per iteration.


## <a name="ui"> Attaching a UI to a Neural Network</a>

A web based User Interface is available. The server will be available on the machine running the java code on port 9000. In order to use you create an instance of UIServer. Create a StatsStorage Listener  for the neural network and attach the Stats Listener to the UI Server. 

Here is the code to demonstrate. 

```
UIServer uiServer = UIServer.getInstance();
StatsStorage statsStorage = new InMemoryStatsStorage();
model.setListeners(new StatsListener(statsStorage),new ScoreIterationListener(1));
uiServer.attach(statsStorage);

```

