# Keras and DL4J models

DL4J and Keras both have two model types. In DL4J, a simple model constructed by
linearly stacking layers is called a `MultiLayerNetwork`, whereas in Keras this
kind of model is called `Sequential`. More complex neural networks, for instance
networks with many inputs, are built as `ComputationGraph`s in DL4J, while Keras
uses the so called `Model` API.

Keras model import maps `Sequential` models to `MultiLayerNetwork`s and `Model`s to
`ComputationGraph`s.

----

- [Model Import API](./model-import)
- [KerasSequential](./sequential)
- [KerasModel](./model)
 