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
// @author raver119@gmail.com
//

#include <ops/declarable/helpers/helpers.h>
#include <ops/declarable/helpers/hamming.h>

namespace nd4j {
    namespace ops {
        namespace helpers {
            template <typename X, typename Z>
            static void _hamming(NDArray &x, NDArray &y, NDArray &z) {
                auto xEws = x.ews();
                auto yEws = y.ews();

                auto xBuffer = x.bufferAsT<X>();
                auto yBuffer = y.bufferAsT<X>();

                Nd4jLong distance = 0;

                if (xEws == 1 && yEws == 1 && x.ordering() == y.ordering()) {
                    PRAGMA_OMP_PARALLEL_FOR_SIMD_REDUCTION(+:distance)
                    for (Nd4jLong e = 0; e < x.lengthOf(); e++) {
                        auto _x = static_cast<unsigned long long>(xBuffer[e]);
                        auto _y = static_cast<unsigned long long>(yBuffer[e]);

                        distance += __builtin_popcountll(_x ^ _y);
                    }

                } else if (xEws > 1 && yEws > 1 && x.ordering() == y.ordering()) {
                    PRAGMA_OMP_PARALLEL_FOR_SIMD_REDUCTION(+:distance)
                    for (Nd4jLong e = 0; e < x.lengthOf(); e++) {
                        auto _x = static_cast<unsigned long long>(xBuffer[e * xEws]);
                        auto _y = static_cast<unsigned long long>(yBuffer[e * yEws]);

                        distance += __builtin_popcountll(_x ^ _y);
                    }
                } else {
                    PRAGMA_OMP_PARALLEL_FOR_SIMD_REDUCTION(+:distance)
                    for (Nd4jLong e = 0; e < x.lengthOf(); e++) {
                        auto _x = static_cast<unsigned long long>(x.e<Nd4jLong>(e));
                        auto _y = static_cast<unsigned long long>(y.e<Nd4jLong>(e));

                        distance += __builtin_popcountll(_x ^ _y);
                    }
                }

                z.p(0, distance);
            }

            void hamming(LaunchContext *context, NDArray &x, NDArray &y, NDArray &output) {
                BUILD_DOUBLE_SELECTOR(x.dataType(), output.dataType(), _hamming, (x, y, output), INTEGER_TYPES, INDEXING_TYPES);
            }
        }
    }
}