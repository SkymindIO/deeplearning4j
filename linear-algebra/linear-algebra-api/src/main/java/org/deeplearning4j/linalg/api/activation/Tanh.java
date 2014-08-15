package org.deeplearning4j.linalg.api.activation;


import org.deeplearning4j.linalg.api.complex.IComplexNDArray;
import org.deeplearning4j.linalg.api.ndarray.INDArray;
import org.deeplearning4j.linalg.factory.NDArrays;
import org.deeplearning4j.linalg.ops.ElementWiseOp;

/**
 * Tanh activation function
 *
 * @author Adam Gibson
 */
public class Tanh extends BaseActivationFunction {


    /**
     *
     */
    private static final long serialVersionUID = 4499150153988528321L;

    /**
     * The class used for transformation
     *
     * @return the class used for transformation
     */
    @Override
    public Class<? extends ElementWiseOp> transformClazz() {
        return org.deeplearning4j.linalg.ops.transforms.Tanh.class;
    }

    /**
     * Name of the function
     *
     * @return the name of the function
     */
    @Override
    public String type() {
        return "tanh";
    }



    @Override
    public INDArray applyDerivative(INDArray input) {
        //1 - tanh^2 x
        if(input instanceof IComplexNDArray) {
            return NDArrays.complexOnes(input.shape()).subi(input);
        }
        else
            return NDArrays.ones(input.shape()).subi(input);
    }




}
