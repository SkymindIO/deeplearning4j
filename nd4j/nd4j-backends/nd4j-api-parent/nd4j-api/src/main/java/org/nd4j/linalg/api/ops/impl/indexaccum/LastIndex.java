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

package org.nd4j.linalg.api.ops.impl.indexaccum;

import lombok.Data;
import lombok.NoArgsConstructor;
import lombok.NonNull;
import org.nd4j.autodiff.samediff.SDVariable;
import org.nd4j.autodiff.samediff.SameDiff;
import org.nd4j.imports.NoOpNameFoundException;
import org.nd4j.linalg.api.ndarray.INDArray;
import org.nd4j.linalg.api.ops.BaseIndexAccumulation;
import org.nd4j.linalg.factory.Nd4j;
import org.nd4j.linalg.indexing.conditions.Condition;
import org.nd4j.linalg.indexing.conditions.Conditions;

import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

@Data
@NoArgsConstructor
public class LastIndex extends BaseIndexAccumulation {
    protected Condition condition;
    protected double compare;
    protected double eps;
    protected Conditions.ConditionMode mode;

    public LastIndex(SameDiff sameDiff, SDVariable i_v, boolean keepDims, Condition condition, long... dimensions) {
        this(sameDiff, i_v, condition, keepDims, dimensions);
    }
    public LastIndex(SameDiff sameDiff, SDVariable i_v, Condition condition, boolean keepDims, long... dimensions) {
        super(sameDiff, i_v, keepDims, dimensions);
        this.condition = condition;
        this.compare = condition.getValue();
        this.mode = condition.conditionType();
        this.eps = Nd4j.EPS_THRESHOLD;
        this.extraArgs = new Object[] {compare, eps, (double) mode.index};
    }

    public LastIndex(SameDiff sameDiff, SDVariable x, @NonNull Condition condition, long... dimensions) {
        super(sameDiff, x, false, dimensions);
        this.condition = condition;
    }

    public LastIndex(INDArray x, @NonNull Condition condition, long... dimensions) {
        this(x, condition, Nd4j.EPS_THRESHOLD, dimensions);
    }

    public LastIndex(INDArray in, boolean keepDim, Condition condition, long... dimensions) {
        this(in, condition, keepDim, dimensions);
    }

    public LastIndex(INDArray x, @NonNull Condition condition, boolean keepDim, long... dimensions) {
        this(x, condition, Nd4j.EPS_THRESHOLD, dimensions);
        this.keepDims = keepDim;
    }

    public LastIndex(INDArray x, @NonNull Condition condition, double eps, long... dimensions) {
        super(x,null, dimensions);
        this.condition = condition;
        this.compare = condition.getValue();
        this.mode = condition.conditionType();
        this.eps = eps;
        this.extraArgs = new Object[] {compare, eps, (double) mode.index};
    }

    @Override
    public Map<String, Object> propertiesForFunction() {
        Map<String,Object> ret = new LinkedHashMap<>();
        ret.put("compare",compare);
        ret.put("eps",eps);
        ret.put("mode",mode);
        return ret;
    }


    @Override
    public int opNum() {
        return 5;
    }

    @Override
    public String opName() {
        return "last_index";
    }



    @Override
    public String onnxName() {
        throw new NoOpNameFoundException("No onnx op opName found for " +  opName());
    }

    @Override
    public String tensorflowName() {
        throw new NoOpNameFoundException("No tensorflow op opName found for " +  opName());
    }

    @Override
    public List<SDVariable> doDiff(List<SDVariable> f1) {
        return Collections.singletonList(sameDiff.zerosLike(arg()));
    }
}
