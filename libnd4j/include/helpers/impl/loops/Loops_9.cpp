/*******************************************************************************
 * Copyright (c) 2015-2018 Skymind, Inc.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License, Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 ******************************************************************************/

//
// @author Yurii Shyrma (iuriish@yahoo.com), created on 25.03.2019
//

#include "helpers/Loops.hpp"

using namespace simdOps;

namespace nd4j {
    
template<typename X>
class TransformStrictWrapper {

    public:

        //////////////////////////////////////////////////////////////////////////////
        template<typename OpType>
        static void wrapper(const X *x, const Nd4jLong* xShapeInfo, X *z, const Nd4jLong *zShapeInfo, X *extras) {
            Loops::loopXZ<X, X, X, OpType>(x, xShapeInfo, z, zShapeInfo, extras);
        }

        //////////////////////////////////////////////////////////////////////////////
        static void wrap(const int opNum, const X *x, const Nd4jLong *xShapeInfo, X *z, const Nd4jLong *zShapeInfo, X *extras) {
            DISPATCH_BY_OPNUM_T(wrapper, PARAMS(x, xShapeInfo, z, zShapeInfo, extras), TRANSFORM_STRICT_OPS);
        }
};



BUILD_SINGLE_TEMPLATE(template class TransformStrictWrapper, , FLOAT_TYPES);

}