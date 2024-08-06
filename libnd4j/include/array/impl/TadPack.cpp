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
#include "../TadPack.h"

#include <helpers/shape.h>
#include <system/Environment.h>

namespace sd {
    TadPack::TadPack(const ConstantShapeBuffer& shapes,
                     const ConstantOffsetsBuffer& offets, LongType numTads,
                 LongType* dimensions, LongType dimLength)
            : _tadShape(shapes),
              _tadOffsets(offets) {
        _numTads = numTads;
        _dimensionsLength = dimLength;
        if(dimensions != nullptr) {
            _dimensions = new LongType[dimLength];
            for(int i = 0; i < dimLength; i++) {
                _dimensions[i] = dimensions[i];
            }
        }

    }

    const LongType* TadPack::primaryShapeInfo() const {
        if(_tadShape.primary() == nullptr)
            THROW_EXCEPTION("TadPack::primaryShapeInfo: primary shape info is nullptr!");
        return _tadShape.primary();
    }

    const LongType* TadPack::primaryOffsets() const {
        return _tadOffsets.primary();
    }

    const LongType* TadPack::specialShapeInfo() const { return _tadShape.special(); }

    const LongType* TadPack::specialOffsets() const { return _tadOffsets.special(); }

LongType TadPack::numberOfTads() const { return _numTads; }

    const LongType* TadPack::platformShapeInfo() const {
        return Environment::getInstance().isCPU() ? primaryShapeInfo() : specialShapeInfo();
    }

    const LongType* TadPack::platformOffsets() const {
        return Environment::getInstance().isCPU() ? primaryOffsets() : specialOffsets();
    }


    void TadPack::print(const char* msg) const {
  printf("---------------------------\n");
  printf("%s: ", msg);
  printf("Offsets:\n");
  for (int e = 0; e < _numTads; e++) {
    printf("%lld, ", _tadOffsets.primary()[e]);
  }
  printf("\n");

  printf("Dimensions:\n");
  if (_dimensions == nullptr || _dimensionsLength == 0) {
    printf("none\n");
  } else {
    for (int i = 0; i < _dimensionsLength; i++) {
      printf("%lld, ", _dimensions[i]);
    }
    printf("\n");
  }

  printf("tad pack shape info:");
  shape::printShapeInfo(_tadShape.primary());
  printf("\n");
  printf("number of tads: %lld\n", _numTads);
  printf("shape info length: %lld\n", _shapeInfoLength);
  printf("---------------------------\n");
}

LongType TadPack::shapeInfoLength() const { return shape::shapeInfoLength(primaryShapeInfo()); }
}  // namespace sd
