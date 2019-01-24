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
//  @author raver119@gmail.com
//

#include <ops/ops.h>
#include <helpers/shape.h>
#include <helpers/TAD.h>
#include <ops/declarable/helpers/prefix.h>

namespace nd4j {
    namespace ops {
        namespace helpers {
            template <typename T>
            static void __prefix(scalar::Ops op, void* vx, Nd4jLong* xShapeInfo, void* vz, Nd4jLong* zShapeInfo, bool exclusive, bool reverse) {

            }

            template <typename T>
            static void __prefix(scalar::Ops op, NDArray* x, NDArray* z, std::vector<int>& dims, bool exclusive, bool reverse) {

            };

            template <typename T>
            static void __prefix(scalar::Ops op, NDArray* x, NDArray* z, bool exclusive, bool reverse) {
                    __prefix<T>(op, x->buffer(), x->shapeInfo(), z->buffer(), z->shapeInfo(), exclusive, reverse);
            };

            void _prefix(scalar::Ops op, NDArray* x, NDArray* z, bool exclusive, bool reverse) {
                BUILD_SINGLE_SELECTOR(x->dataType(), __prefix, (op, x, z, exclusive, reverse), LIBND4J_TYPES);
            }

            void _prefix(scalar::Ops op, NDArray* x, NDArray* z, std::vector<int>& dims, bool exclusive, bool reverse) {
                BUILD_SINGLE_SELECTOR(x->dataType(), __prefix, (op, x, z, dims, exclusive, reverse), LIBND4J_TYPES);
            }

            BUILD_SINGLE_TEMPLATE(template void __prefix, (scalar::Ops op, void* vx, Nd4jLong* xShapeInfo, void* vz, Nd4jLong* zShapeInfo, bool exclusive, bool reverse), LIBND4J_TYPES);
            BUILD_SINGLE_TEMPLATE(template void __prefix, (scalar::Ops op, NDArray* x, NDArray* z, std::vector<int>& dims, bool exclusive, bool reverse), LIBND4J_TYPES);
            BUILD_SINGLE_TEMPLATE(template void __prefix, (scalar::Ops op, NDArray* x, NDArray* z, bool exclusive, bool reverse), LIBND4J_TYPES);



        }
    }
}