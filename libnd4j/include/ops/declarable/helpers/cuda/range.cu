/* ******************************************************************************
 *
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License, Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0.
 *
 *  See the NOTICE file distributed with this work for additional
 *  information regarding copyright ownership.
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 ******************************************************************************/

//
// @author Yurii Shyrma (iuriish@yahoo.com), created on 27.08.2018
//

#include <ops/declarable/helpers/range.h>

#include "execution/cuda/LaunchDims.h"


namespace sd {
namespace ops {
namespace helpers {

template <typename T>
static SD_KERNEL void global_range(void* output, LongType length, T start, T delta) {
  auto buff = reinterpret_cast<T*>(output);
  const auto tid = blockIdx.x * blockDim.x + threadIdx.x;
  const auto step = gridDim.x * blockDim.x;
  for (LongType i = tid; i < length; i += step) {
    buff[i] = static_cast<T>(start) + static_cast<T>(i) * static_cast<T>(delta);
  }
}

//////////////////////////////////////////////////////////////////////////
// be careful: outVector must have c-order and ews = 1 !!!
template <typename T>
static void _range(LaunchContext* context, NDArray& start, NDArray& delta, NDArray& outVector) {
 dim3 launchDims = getLaunchDims("range");
  global_range<T><<<launchDims.y, launchDims.x, launchDims.z, *context->getCudaStream()>>>(outVector.specialBuffer(), outVector.lengthOf(),
                                                                 start.e<T>(0), delta.e<T>(0));
  sd::DebugHelper::checkErrorCode(context->getCudaStream(), "global_range failed");

}

void range(LaunchContext* context, NDArray& start, NDArray& delta, NDArray& outVector) {
  NDArray::prepareSpecialUse({&outVector}, {&start, &delta});
  BUILD_SINGLE_SELECTOR(outVector.dataType(), _range, (context, start, delta, outVector), SD_COMMON_TYPES);
  NDArray::registerSpecialUse({&outVector}, {&start, &delta});
}

}  // namespace helpers
}  // namespace ops
}  // namespace sd
