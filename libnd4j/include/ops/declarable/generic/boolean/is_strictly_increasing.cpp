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
//  @author @cpuheater
//

#include <system/op_boilerplate.h>
#if NOT_EXCLUDED(OP_is_strictly_increasing)

#include <ops/declarable/CustomOperations.h>
#include <ops/declarable/helpers/compare_elem.h>

namespace sd {
namespace ops {
BOOLEAN_OP_IMPL(is_strictly_increasing, 1, true) {
  auto input = INPUT_VARIABLE(0);

  // in case of empty input there's nothing to do
  if (input->isEmpty()) return Status::EQ_TRUE;

  bool isStrictlyIncreasing = true;

  helpers::compare_elem(block.launchContext(), input, true, isStrictlyIncreasing);

  if (isStrictlyIncreasing)
    return Status::EQ_TRUE;
  else
    return Status::EQ_FALSE;
}

DECLARE_TYPES(is_strictly_increasing) {
  getOpDescriptor()->setAllowedInputTypes(0, ANY)->setAllowedOutputTypes(0, BOOL);
}
}  // namespace ops
}  // namespace sd

#endif
