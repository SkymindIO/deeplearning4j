/*
 *  ******************************************************************************
 *  *
 *  *
 *  * This program and the accompanying materials are made available under the
 *  * terms of the Apache License, Version 2.0 which is available at
 *  * https://www.apache.org/licenses/LICENSE-2.0.
 *  *
 *  *  See the NOTICE file distributed with this work for additional
 *  *  information regarding copyright ownership.
 *  * Unless required by applicable law or agreed to in writing, software
 *  * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 *  * License for the specific language governing permissions and limitations
 *  * under the License.
 *  *
 *  * SPDX-License-Identifier: Apache-2.0
 *  *****************************************************************************
 */

package org.nd4j.linalg.api.ops.impl.reduce3;

import org.nd4j.autodiff.samediff.SDVariable;
import org.nd4j.autodiff.samediff.SameDiff;
import org.nd4j.linalg.api.ndarray.INDArray;

import java.util.Arrays;
import java.util.List;

public class HammingDistance extends BaseReduce3Op {


    public HammingDistance(SameDiff sameDiff, SDVariable i_v, SDVariable i_v2, long... dimensions) {
        super(sameDiff, i_v, i_v2, dimensions);
    }

    public HammingDistance(SameDiff sameDiff, SDVariable i_v, SDVariable dimensions) {
        super(sameDiff, i_v, dimensions);
    }

    public HammingDistance(SameDiff sameDiff, SDVariable i_v, SDVariable i_v2, SDVariable dimensions) {
        super(sameDiff, i_v, i_v2, dimensions);
    }

    public HammingDistance() {
    }

    public HammingDistance(INDArray x, INDArray y, long... dimensions) {
        this(x, y, null, false, dimensions);
    }

    public HammingDistance(INDArray x, INDArray y, INDArray z, boolean allDistances, long... dimensions) {
        this(x, y, z, false, allDistances, dimensions);
        this.isComplex = allDistances;
    }

    public HammingDistance(INDArray x, INDArray y, boolean allDistances, long... dimensions) {
        this(x, y, null, allDistances, dimensions);
    }

    public HammingDistance(INDArray x, INDArray y, INDArray z) {
        this(x, y, z, false, null);
    }

    public HammingDistance(INDArray x, INDArray y, INDArray z, boolean keepDims, boolean allDistances, long... dimensions){
        super(x, y, z, keepDims, allDistances, dimensions);
        extraArgs = new Object[]{0.0f, 0.0f};
    }

    public HammingDistance(INDArray x, INDArray y, INDArray z, long... dimensions) {
        super(x, y, z, dimensions);
    }

    public HammingDistance(SameDiff sameDiff, SDVariable i_v, long[] dimensions) {
        super(sameDiff, i_v, dimensions);
    }

    public HammingDistance(SameDiff sd, SDVariable x, SDVariable y, boolean keepDims, boolean isComplex, long[] dimensions) {
        super(sd,x,y,keepDims,isComplex,dimensions);
    }

    public HammingDistance(INDArray x, INDArray y, boolean keepDims, boolean isComplex, long[] dimensions) {
        super(x,y,null,keepDims,isComplex,dimensions);
    }

    @Override
    public int opNum() {
        return 7;
    }

    @Override
    public String opName() {
        return "hammingdistance";
    }


    @Override
    public List<SDVariable> doDiff(List<SDVariable> f1) {
        //Hamming distance: "the Hamming distance between two strings of equal length is the number of positions at
        // which the corresponding symbols are different."
        //Consequently: it's not continuously differentiable, and gradients are 0 almost everywhere (but undefined
        // when x_i == y_i)
        return Arrays.asList(sameDiff.zerosLike(larg()), sameDiff.zerosLike(rarg()));
    }
}
