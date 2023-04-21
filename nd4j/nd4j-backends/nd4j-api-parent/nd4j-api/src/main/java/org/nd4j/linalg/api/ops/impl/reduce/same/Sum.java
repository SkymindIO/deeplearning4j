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

package org.nd4j.linalg.api.ops.impl.reduce.same;

import lombok.extern.slf4j.Slf4j;
import org.nd4j.autodiff.samediff.SDVariable;
import org.nd4j.autodiff.samediff.SameDiff;
import org.nd4j.linalg.api.ndarray.INDArray;
import org.nd4j.linalg.api.ops.BaseReduceSameOp;
import org.nd4j.linalg.api.ops.impl.reduce.bp.SumBp;

import java.util.List;

@Slf4j
public class Sum extends BaseReduceSameOp {
    public Sum(SameDiff sameDiff, SDVariable i_v, boolean keepDims, long[] dimensions) {
        super(sameDiff, i_v, dimensions, keepDims);
    }

    public Sum(SameDiff sameDiff, SDVariable i_v, SDVariable i_v2, long[] dimensions) {
        super(sameDiff, i_v, i_v2, dimensions);
    }

    public Sum(SameDiff sameDiff, SDVariable i_v, boolean keepDims) {
        super(sameDiff, i_v, keepDims);
    }

    public Sum(SameDiff sameDiff, SDVariable i_v, SDVariable dimensions, boolean keepDims) {
        super(sameDiff, i_v, dimensions, keepDims);
    }

    public Sum(SameDiff sameDiff, SDVariable i_v, SDVariable i_v2) {
        super(sameDiff, i_v, i_v2);
    }

    public Sum(SameDiff sameDiff, SDVariable input, long[] dimensions, boolean keepDims) {
        super(sameDiff, input, dimensions, keepDims);
    }

    public Sum(SameDiff sameDiff, SDVariable i_v, SDVariable i_v2, long[] dimensions, boolean keepDims) {
        super(sameDiff, i_v, i_v2, dimensions, keepDims);
    }

    public Sum(SameDiff sameDiff, SDVariable i_v) {
        super(sameDiff, i_v);
    }

    public Sum(SameDiff sameDiff, SDVariable input, long... dimensions) {
        super(sameDiff, input, dimensions);
    }

    public Sum() {
    }

    public Sum(INDArray x, INDArray y, INDArray z, boolean keepDims, long[] dimensions) {
        super(x, y, z, keepDims, dimensions);
    }

    public Sum(INDArray x, long... dimensions) {
        super(x, dimensions);
    }

    public Sum(INDArray x, INDArray z, long... dimensions) {
        super(x, null, z, dimensions);
    }

    public Sum(INDArray x, INDArray z, boolean keepDims, long... dimensions) {
        super(x, z, keepDims, dimensions);
    }

    public Sum(INDArray x, INDArray y, INDArray z, long... dimensions) {
        super(x, y, z, dimensions);
    }

    public Sum(SameDiff sameDiff) {
        super(sameDiff);
    }

    public Sum(SameDiff sameDiff, SDVariable i_v, SDVariable i_v2, SDVariable dimensions) {
        super(sameDiff, i_v, i_v2, dimensions);
    }

    public Sum(INDArray x, boolean keepDims, long... dimensions) {
        this(x, null, keepDims, dimensions);
    }

    public Sum(INDArray in, long[] dimensions, boolean keepDims) {
        super(in,keepDims,dimensions);
    }

    @Override
    public int opNum() {
        return 0;
    }

    @Override
    public String opName() {
        return "reduce_sum";
    }

    @Override
    public List<SDVariable> doDiff(List<SDVariable> i_v1) {
        //Out = sum(in)
        // dL/dIn = dL/dOut * dOut/dIn
        //        = dL/dOut * 1
        // But broadcast to shape of the input
        return new SumBp(sameDiff, arg(), i_v1.get(0), keepDims, dimensions).outputs();
    }


    @Override
    public String onnxName() {
        return "Sum";
    }

    @Override
    public String tensorflowName() {
        return "Sum";
    }

}
