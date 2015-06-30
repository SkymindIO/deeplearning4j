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

package org.deeplearning4j.spark.util;

import org.apache.spark.api.java.JavaPairRDD;
import org.apache.spark.api.java.JavaRDD;
import org.apache.spark.api.java.JavaSparkContext;
import org.apache.spark.api.java.function.Function;
import org.apache.spark.input.PortableDataStream;
import org.apache.spark.mllib.linalg.Matrices;
import org.apache.spark.mllib.linalg.Matrix;
import org.apache.spark.mllib.linalg.Vector;
import org.apache.spark.mllib.linalg.Vectors;
import org.apache.spark.mllib.regression.LabeledPoint;
import org.canova.api.records.reader.RecordReader;
import org.canova.api.split.InputStreamInputSplit;
import org.canova.api.writable.Writable;
import org.nd4j.linalg.api.ndarray.INDArray;
import org.nd4j.linalg.dataset.DataSet;
import org.nd4j.linalg.factory.Nd4j;
import org.nd4j.linalg.util.FeatureUtil;
import scala.Tuple2;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;


/**
 * Dl4j <----> MLLib
 *
 * @author Adam Gibson
 */
public class MLLibUtil {


    /**
     * This is for the edge case where
     * you have a single output layer
     * and need to convert the output layer to
     * an index
     * @param vector the vector to get the classifier prediction for
     * @return the prediction for the given vector
     */
    public static double toClassifierPrediction(Vector vector) {
        double max = Double.NEGATIVE_INFINITY;
        int maxIndex = 0;
        for(int i = 0; i < vector.size(); i++) {
            double curr = vector.apply(i);
            if(curr > max) {
                maxIndex = i;
                max = curr;
            }
        }

        return maxIndex;
    }

    /**
     * Convert an ndarray to a matrix.
     * Note that the matrix will be con
     * @param arr the array
     * @return an mllib vector
     */
    public static INDArray toMatrix(Matrix arr) {
        return Nd4j.create(arr.toArray(), new int[]{arr.numRows(), arr.numCols()});
    }

    /**
     * Convert an ndarray to a vector
     * @param arr the array
     * @return an mllib vector
     */
    public static INDArray toVector(Vector arr) {
        return Nd4j.create(Nd4j.createBuffer(arr.toArray()));
    }


    /**
     * Convert an ndarray to a matrix.
     * Note that the matrix will be con
     * @param arr the array
     * @return an mllib vector
     */
    public static Matrix toMatrix(INDArray arr) {
        if(!arr.isMatrix()) {
            throw new IllegalArgumentException("passed in array must be a matrix");
        }
        return Matrices.dense(arr.rows(), arr.columns(), arr.data().asDouble());
    }

    /**
     * Convert an ndarray to a vector
     * @param arr the array
     * @return an mllib vector
     */
    public static Vector toVector(INDArray arr) {
        if(!arr.isVector()) {
            throw new IllegalArgumentException("passed in array must be a vector");
        }
        double[] ret = new double[arr.length()];
        for(int i = 0; i < arr.length(); i++) {
            ret[i] = arr.getDouble(i);
        }

        return Vectors.dense(ret);
    }


    /**
     * Convert a traditional sc.binaryFiles
     * in to something usable for machine learning
     * @param binaryFiles the binary files to convert
     * @param reader the reader to use
     * @return the labeled points based on
     * the given rdd
     */
    public static JavaRDD<LabeledPoint> fromBinary(JavaPairRDD<String, PortableDataStream> binaryFiles,final RecordReader reader) {
        JavaRDD<Collection<Writable>> records = binaryFiles.map(new Function<Tuple2<String, PortableDataStream>, Collection<Writable>>() {
            @Override
            public Collection<Writable> call(Tuple2<String, PortableDataStream> stringPortableDataStreamTuple2) throws Exception {
                reader.initialize(new InputStreamInputSplit(stringPortableDataStreamTuple2._2().open(),stringPortableDataStreamTuple2._1()));
                return reader.next();
            }
        });

        JavaRDD<LabeledPoint> ret = records.map(new Function<Collection<Writable>, LabeledPoint>() {
            @Override
            public LabeledPoint call(Collection<Writable> writables) throws Exception {
                return pointOf(writables);
            }
        });
        return ret;
    }


    /**
     * Returns a labeled point of the writables
     * where the final item is the point and the rest of the items are
     * features
     * @param writables the writables
     * @return the labeled point
     */
    public static LabeledPoint pointOf(Collection<Writable> writables) {
        double[] ret = new double[writables.size()];
        int count = 0;
        double target = 0;
        for(Writable w : writables) {
            if(count < writables.size() - 1)
                ret[count++] = Float.parseFloat(w.toString());
            else
                target = Float.parseFloat(w.toString());
        }

        return new LabeledPoint(target,Vectors.dense(ret));
    }


    /**
     * From labeled point
     * @param sc the org.deeplearning4j.spark context used for creating the rdd
     * @param data the data to convert
     * @param numPossibleLabels the number of possible labels
     * @return
     */
    public static JavaRDD<DataSet> fromLabeledPoint(JavaSparkContext sc,JavaRDD<LabeledPoint> data,int numPossibleLabels) {
        List<DataSet> list  = fromLabeledPoint(data.collect(), numPossibleLabels);
        return sc.parallelize(list);
    }

    /**
     * Convert an rdd of data set in to labeled point
     * @param sc the spark context to use
     * @param data the dataset to convert
     * @return an rdd of labeled point
     */
    public static JavaRDD<LabeledPoint> fromDataSet(JavaSparkContext sc,JavaRDD<DataSet> data) {
        List<LabeledPoint> list  = toLabeledPoint(data.collect());
        return sc.parallelize(list);
    }


    /**
     * Convert a list of dataset in to a list of labeled points
     * @param labeledPoints the labeled points to convert
     * @return the labeled point list
     */
    private static List<LabeledPoint> toLabeledPoint(List<DataSet> labeledPoints) {
        List<LabeledPoint> ret = new ArrayList<>();
        for(DataSet point : labeledPoints) {
            ret.add(toLabeledPoint(point));
        }
        return ret;
    }

    /**
     * Convert a dataset (feature vector) to a labeled point
     * @param point the point to convert
     * @return the labeled point derived from this dataset
     */
    private static LabeledPoint toLabeledPoint(DataSet point) {
        if(!point.getFeatureMatrix().isVector()) {
            throw new IllegalArgumentException("Feature matrix must be a vector");
        }

        Vector features = toVector(point.getFeatureMatrix().dup());

        double label = Nd4j.getBlasWrapper().iamax(point.getLabels());
        return new LabeledPoint(label,features);
    }


    /**
     *
     * @param labeledPoints
     * @param numPossibleLabels
     * @return List of {@link DataSet}
     */
    private static List<DataSet> fromLabeledPoint(List<LabeledPoint> labeledPoints,int numPossibleLabels) {
        List<DataSet> ret = new ArrayList<>();
        for(LabeledPoint point : labeledPoints) {
            ret.add(fromLabeledPoint(point, numPossibleLabels));
        }
        return ret;
    }

    /**
     *
     * @param point
     * @param numPossibleLabels
     * @return {@link DataSet}
     */
    private static DataSet fromLabeledPoint(LabeledPoint point,int numPossibleLabels) {
        Vector features = point.features();
        double label = point.label();
        return new DataSet(Nd4j.create(features.toArray()), FeatureUtil.toOutcomeVector((int) label, numPossibleLabels));
    }


}
