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
//  @author raver119@gmail.com
//

#ifndef DEV_TESTS_TADPACK_H
#define DEV_TESTS_TADPACK_H

#include <array/ConstantOffsetsBuffer.h>
#include <array/ConstantShapeBuffer.h>
#include <system/common.h>

#include <array/NDArray.h>

namespace sd {
class SD_LIB_EXPORT TadPack {
 private:
  ConstantShapeBuffer _tadShape;
  ConstantOffsetsBuffer _tadOffsets;
  LongType _numTads = 0;
  LongType _shapeInfoLength = 0;
  LongType* _dimensions = nullptr;
  LongType _dimensionsLength = 0;
 public:
  explicit TadPack(const ConstantShapeBuffer& shapes,
                   const ConstantOffsetsBuffer& offets, LongType numTads,
                   LongType* dimensions = nullptr, LongType dimLength = 0);
  TadPack() = default;
  ~TadPack() {};

  LongType* primaryShapeInfo();
  LongType* primaryOffsets();

  LongType* specialShapeInfo();
  LongType* specialOffsets();

  LongType numberOfTads() const;
  LongType shapeInfoLength();
  /**
   * Extracts an NDArray view for the given TAD index.
   * @param input The input NDArray.
   * @param tadIndex The index of the TAD to extract.
   * @return A new NDArray view representing the TAD.
   */
  NDArray *extractTadView(NDArray* input, sd::LongType tadIndex) {
    auto shapeInfo = primaryShapeInfo();
    auto offsets = primaryOffsets();

    auto tadOffset = offsets[tadIndex];
    auto x = input->buffer();
    NDArray *ret = new NDArray(x,shapeInfo,LaunchContext::defaultContext(),false,tadOffset);
    return ret;
  }

  /**
   * These methods return either primary or special pointers depending on platform binaries were compiled for
   * @return
   */
  LongType* platformShapeInfo();
  LongType* platformOffsets();

  void print(const char* msg);
};
}  // namespace sd

#endif  // DEV_TESTS_TADPACK_H
