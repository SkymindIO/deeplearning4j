package org.deeplearning4j.nn.graph.multioutput;

import org.deeplearning4j.datasets.iterator.impl.ListDataSetIterator;
import org.deeplearning4j.gradientcheck.GradientCheckUtil;
import org.deeplearning4j.nn.api.activations.Activations;
import org.deeplearning4j.nn.api.activations.ActivationsFactory;
import org.deeplearning4j.nn.conf.ComputationGraphConfiguration;
import org.deeplearning4j.nn.conf.NeuralNetConfiguration;
import org.deeplearning4j.nn.conf.distribution.NormalDistribution;
import org.deeplearning4j.nn.conf.graph.ElementWiseVertex;
import org.deeplearning4j.nn.conf.layers.DenseLayer;
import org.deeplearning4j.nn.conf.layers.OutputLayer;
import org.deeplearning4j.nn.conf.layers.variational.VariationalAutoencoder;
import org.deeplearning4j.nn.graph.ComputationGraph;
import org.deeplearning4j.nn.graph.multioutput.testlayers.SplitOutputLayerConf;
import org.deeplearning4j.nn.graph.vertex.Edge;
import org.deeplearning4j.nn.graph.multioutput.testlayers.SplitDenseLayerConf;
import org.deeplearning4j.nn.weights.WeightInit;
import org.junit.Test;
import org.nd4j.linalg.activations.Activation;
import org.nd4j.linalg.api.ndarray.INDArray;
import org.nd4j.linalg.dataset.DataSet;
import org.nd4j.linalg.dataset.api.MultiDataSet;
import org.nd4j.linalg.dataset.api.iterator.DataSetIterator;
import org.nd4j.linalg.factory.Nd4j;
import org.nd4j.linalg.learning.config.NoOp;
import org.nd4j.linalg.lossfunctions.LossFunctions;

import java.lang.reflect.Field;
import java.util.*;

import static org.junit.Assert.*;

public class TestCGMultiOutputLayers {

    private static final boolean PRINT_RESULTS = true;
    private static final boolean RETURN_ON_FIRST_FAILURE = false;
    private static final double DEFAULT_EPS = 1e-6;
    private static final double DEFAULT_MAX_REL_ERROR = 1e-5;
    private static final double DEFAULT_MIN_ABS_ERROR = 1e-8;

    private static final ActivationsFactory af = ActivationsFactory.getInstance();

    @Test
    public void testMultipleOutputSimple(){

        int nIn = 5;
        int minibatch = 3;

        ComputationGraphConfiguration conf = new NeuralNetConfiguration.Builder()
                .seed(12345)
                .weightInit(WeightInit.DISTRIBUTION).dist(new NormalDistribution(0,1))
                .activation(Activation.TANH)
                .updater(new NoOp())
                .graphBuilder()
                .addInputs("in")
                .layer("first", new DenseLayer.Builder().nIn(nIn).nOut(5).build(), "in")
                .layer("second", new SplitDenseLayerConf.Builder().nIn(5).nOut(5).build(), "first")
                .layer("out1", new OutputLayer.Builder().lossFunction(LossFunctions.LossFunction.MSE).nIn(2).nOut(3).build(), "second/0")
                .layer("out2", new OutputLayer.Builder().lossFunction(LossFunctions.LossFunction.MSE).nIn(3).nOut(4).build(), "second/1")
                .setOutputs("out1", "out2")
                .build();

        ComputationGraph net = new ComputationGraph(conf);
        net.init();

        //Validate the edges + structure:
        Set<String> gvInputVertex = Collections.singleton("in");
        Set<String> gvOutputVertex = new HashSet<>(Arrays.asList("out1","out2"));
        Map<String,Edge[]> gvEdgesIn = new HashMap<>();       //Key: vertex name X. Values: edges Y -> X, for all Y
        Map<String,Edge[]> gvEdgesOut = new HashMap<>();      //Key: vertex name X. Values: edges X -> Y, for all Y

        gvEdgesIn.put("first", new Edge[]{new Edge("in", 0, 0, "first", 1, 0)});
        gvEdgesIn.put("second", new Edge[]{new Edge("first", 1, 0, "second", 2, 0)});
        gvEdgesIn.put("out1", new Edge[]{new Edge("second", 2, 0, "out1", 3, 0)});
        gvEdgesIn.put("out2", new Edge[]{new Edge("second", 2, 1, "out2", 4, 0)});

        gvEdgesOut.put("in", new Edge[]{new Edge("in", 0, 0, "first", 1, 0)});
        gvEdgesOut.put("first", new Edge[]{new Edge("first", 1, 0, "second", 2, 0)});
        gvEdgesOut.put("second", new Edge[]{
                new Edge("second", 2, 0, "out1", 3, 0),
                new Edge("second", 2, 1, "out2", 4, 0)
        });


        Set<String> gvInputVertexAct = (Set<String>)getObject(net, "gvInputVertex");
        Set<String> gvOutputVertexAct = (Set<String>)getObject(net, "gvOutputVertex");
        Map<String,Edge[]> gvEdgesInAct = (Map<String,Edge[]>)getObject(net, "gvEdgesIn");
        Map<String,Edge[]> gvEdgesOutAct = (Map<String,Edge[]>)getObject(net, "gvEdgesOut");

        assertEquals(gvInputVertex, gvInputVertexAct);
        assertEquals(gvOutputVertex, gvOutputVertexAct);
        assertEqualsMap(gvEdgesIn, gvEdgesInAct);
        assertEqualsMap(gvEdgesOut, gvEdgesOutAct);

        INDArray input = Nd4j.rand(minibatch, nIn);
        Map<String,Activations> act = net.feedForward(af.create(input), true);

        assertEquals(5, act.size());    //Including input

        for( Map.Entry<String,Activations> e : act.entrySet()){
            if(e.getKey().equals("second")){
                assertEquals(2, e.getValue().size());
            } else {
                assertEquals(1, e.getValue().size());
            }
        }



        //Gradient check:
        INDArray labels1 = Nd4j.rand(minibatch, 3);
        INDArray labels2 = Nd4j.rand(minibatch, 4);
        INDArray[] labels = new INDArray[]{labels1, labels2};

        boolean gradOK = GradientCheckUtil.checkGradients(net, DEFAULT_EPS, DEFAULT_MAX_REL_ERROR,
                DEFAULT_MIN_ABS_ERROR, PRINT_RESULTS, RETURN_ON_FIRST_FAILURE, new INDArray[]{input},
                labels);

        String msg = "testMultipleOutputsSimple()";
        assertTrue(msg, gradOK);
    }

    @Test
    public void testMultipleOutputStructure2(){
        int nIn = 5;
        int minibatch = 3;

        ComputationGraphConfiguration conf = new NeuralNetConfiguration.Builder()
                .seed(12345)
                .updater(new NoOp())
                .graphBuilder()
                .addInputs("in")
                .layer("first", new DenseLayer.Builder().nIn(nIn).nOut(5).build(), "in")
                .layer("second", new SplitDenseLayerConf.Builder().nIn(5).nOut(6).build(), "first")
                .addVertex("ewise", new ElementWiseVertex(ElementWiseVertex.Op.Add), "second/0", "second/1")
                .layer("out", new OutputLayer.Builder().lossFunction(LossFunctions.LossFunction.MSE).nIn(3).nOut(3).build(), "ewise")
                .setOutputs("out")
                .build();

        ComputationGraph net = new ComputationGraph(conf);
        net.init();

        //Validate the edges + structure:
        Set<String> gvInputVertex = Collections.singleton("in");
        Set<String> gvOutputVertex = Collections.singleton("out");
        Map<String,Edge[]> gvEdgesIn = new HashMap<>();       //Key: vertex name X. Values: edges Y -> X, for all Y
        Map<String,Edge[]> gvEdgesOut = new HashMap<>();      //Key: vertex name X. Values: edges X -> Y, for all Y

        gvEdgesIn.put("first", new Edge[]{new Edge("in", 0, 0, "first", 1, 0)});
        gvEdgesIn.put("second", new Edge[]{new Edge("first", 1, 0, "second", 2, 0)});
        gvEdgesIn.put("ewise", new Edge[]{
                new Edge("second", 2, 0, "ewise", 3, 0),
                new Edge("second", 2, 1, "ewise", 3, 1),
        });
        gvEdgesIn.put("out", new Edge[]{new Edge("ewise", 3, 0, "out", 4, 0)});

        gvEdgesOut.put("in", new Edge[]{new Edge("in", 0, 0, "first", 1, 0)});
        gvEdgesOut.put("first", new Edge[]{new Edge("first", 1, 0, "second", 2, 0)});
        gvEdgesOut.put("second", new Edge[]{
                new Edge("second", 2, 0, "ewise", 3, 0),
                new Edge("second", 2, 1, "ewise", 3, 1)
        });
        gvEdgesOut.put("ewise", new Edge[]{new Edge("ewise", 3, 0, "out", 4, 0)});



        Set<String> gvInputVertexAct = (Set<String>)getObject(net, "gvInputVertex");
        Set<String> gvOutputVertexAct = (Set<String>)getObject(net, "gvOutputVertex");
        Map<String,Edge[]> gvEdgesInAct = (Map<String,Edge[]>)getObject(net, "gvEdgesIn");
        Map<String,Edge[]> gvEdgesOutAct = (Map<String,Edge[]>)getObject(net, "gvEdgesOut");

        assertEquals(gvInputVertex, gvInputVertexAct);
        assertEquals(gvOutputVertex, gvOutputVertexAct);
        assertEqualsMap(gvEdgesIn, gvEdgesInAct);
        assertEqualsMap(gvEdgesOut, gvEdgesOutAct);

        INDArray input = Nd4j.rand(minibatch, nIn);
        Map<String,Activations> act = net.feedForward(af.create(input), true);

        assertEquals(5, act.size());    //Including input

        for( Map.Entry<String,Activations> e : act.entrySet()){
            if(e.getKey().equals("second")){
                assertEquals(2, e.getValue().size());
            } else {
                assertEquals(1, e.getValue().size());
            }
        }


        //Gradient check:
        INDArray labels1 = Nd4j.rand(minibatch, 3);
        INDArray[] labels = new INDArray[]{labels1};

        boolean gradOK = GradientCheckUtil.checkGradients(net, DEFAULT_EPS, DEFAULT_MAX_REL_ERROR,
                DEFAULT_MIN_ABS_ERROR, PRINT_RESULTS, RETURN_ON_FIRST_FAILURE, new INDArray[]{input},
                labels);

        String msg = "testMultipleOutputStructure2()";
        assertTrue(msg, gradOK);
    }

    @Test
    public void testPretrainLayerMultipleOutputs(){

        int nIn = 5;
        int minibatch = 3;

        ComputationGraphConfiguration conf = new NeuralNetConfiguration.Builder()
                .seed(12345)
                .weightInit(WeightInit.DISTRIBUTION).dist(new NormalDistribution(0,1))
                .activation(Activation.TANH)
                .updater(new NoOp())
                .graphBuilder()
                .addInputs("in")
                .layer("first", new DenseLayer.Builder().nIn(nIn).nOut(5).build(), "in")
                .layer("second", new SplitDenseLayerConf.Builder().nIn(5).nOut(5).build(), "first")
                .layer("vae", new VariationalAutoencoder.Builder().nIn(3).nOut(3).encoderLayerSizes(5).decoderLayerSizes(5).build(), "second/1")
                .layer("out1", new OutputLayer.Builder().lossFunction(LossFunctions.LossFunction.MSE).nIn(2).nOut(3).build(), "second/0")
                .layer("out2", new OutputLayer.Builder().lossFunction(LossFunctions.LossFunction.MSE).nIn(3).nOut(4).build(), "vae")
                .setOutputs("out1", "out2")
                .build();

        ComputationGraph net = new ComputationGraph(conf);
        net.init();

        //Simple sanity check on pretrainLayer method:
        INDArray input = Nd4j.rand(minibatch, nIn);
        DataSet ds = new DataSet(input, null);
        DataSetIterator trainIter = new ListDataSetIterator<>(Collections.singletonList(ds), minibatch);
        net.pretrainLayer("vae", trainIter);

        Activations out = net.output(input);
        assertEquals(2, out.size());
    }


    @Test
    public void testMultiOutputOL(){
        //Multi-output output layer

        int minibatch = 3;
        int nIn = 5;
        int nOut = 7;

        ComputationGraphConfiguration conf = new NeuralNetConfiguration.Builder()
                .seed(12345)
                .weightInit(WeightInit.DISTRIBUTION).dist(new NormalDistribution(0,1))
                .activation(Activation.TANH)
                .updater(new NoOp())
                .graphBuilder()
                .addInputs("in")
                .layer("first", new DenseLayer.Builder().nIn(nIn).nOut(5).build(), "in")
                .layer("out", new SplitOutputLayerConf.Builder()
                        .lossFunction(LossFunctions.LossFunction.MSE).nIn(5).nOut(nOut).build(), "first")
                .setOutputs("out")
                .build();

        ComputationGraph cg = new ComputationGraph(conf);
        cg.init();

        assertEquals(2, cg.numOutputs());
        assertEquals(2, cg.getNumOutputArrays());

        INDArray in = Nd4j.rand(minibatch, nIn);
        Activations out = cg.output(in);

        assertEquals(2, out.size());

        INDArray[] labels = new INDArray[]{Nd4j.rand(minibatch, nOut/2), Nd4j.rand(minibatch, nOut - nOut/2)};

        MultiDataSet mds = new org.nd4j.linalg.dataset.MultiDataSet(new INDArray[]{in}, labels);
        cg.fit(mds);

        cg.clear();


        //Gradient check:
        boolean gradOK = GradientCheckUtil.checkGradients(cg, DEFAULT_EPS, DEFAULT_MAX_REL_ERROR,
                DEFAULT_MIN_ABS_ERROR, PRINT_RESULTS, RETURN_ON_FIRST_FAILURE, new INDArray[]{in},
                labels);

        String msg = "testMultiOutputOL()";
        assertTrue(msg, gradOK);
    }


    private static Object getObject(Object from, String name){
        try{
            Field f = from.getClass().getDeclaredField(name);
            f.setAccessible(true);
            return f.get(from);
        }catch (Exception e){
            throw new RuntimeException(e);
        }
    }

    private static void assertEqualsMap(Map<String,Edge[]> expected, Map<String,Edge[]> act){
        assertEquals(expected.keySet(), act.keySet());
        for( Map.Entry<String,Edge[]> s : expected.entrySet()){
            assertArrayEquals(s.getValue(), act.get(s.getKey()));
        }
    }
}
