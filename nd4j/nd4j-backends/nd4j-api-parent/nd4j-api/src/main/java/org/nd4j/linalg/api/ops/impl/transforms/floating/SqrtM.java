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

package org.nd4j.linalg.api.ops.impl.transforms.floating;

import org.nd4j.autodiff.samediff.SDVariable;
import org.nd4j.autodiff.samediff.SameDiff;
import org.nd4j.linalg.api.ndarray.INDArray;
import org.nd4j.linalg.api.ops.BaseTransformFloatOp;
import org.nd4j.linalg.api.ops.DynamicCustomOp;

import java.util.Collections;
import java.util.List;

public class SqrtM extends DynamicCustomOp {

    public SqrtM(SameDiff sameDiff, SDVariable arg) {
        super(sameDiff, arg);
    }

    public SqrtM(SameDiff sameDiff, SDVariable[] args) {
        super(null, sameDiff, args);
    }

    public SqrtM() {
    }

    public SqrtM(INDArray[] inputs, INDArray[] outputs, List<Double> tArguments, int[] iArguments) {
        super(null, inputs, outputs, tArguments, iArguments);
    }

    public SqrtM( INDArray[] inputs, INDArray[] outputs, List<Double> tArguments, List<Integer> iArguments) {
        super(null, inputs, outputs, tArguments, iArguments);
    }

    public SqrtM( INDArray[] inputs, INDArray[] outputs) {
        super(null, inputs, outputs);
    }

    public SqrtM(INDArray input) {
        this(new INDArray[]{input},null);
    }


    @Override
    public List<SDVariable> doDiff(List<SDVariable> f1) {
        throw new UnsupportedOperationException("Derivative of op SqrtM not supported");
    }

    @Override
    public String opName() {
        return "sqrtm";
    }




}
