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
// @author George A. Shulinok <sgazeos@gmail.com>
//

#include <system/op_boilerplate.h>
#if NOT_EXCLUDED(OP_bitcast)

#include <array/DataTypeUtils.h>
#include <ops/declarable/CustomOperations.h>

namespace sd {
namespace ops {
CUSTOM_OP_IMPL(bitcast, 1, 1, false, 0, 1) {
  auto input = INPUT_VARIABLE(0);
  auto output = OUTPUT_VARIABLE(0);
  // when empty - nothing to do
  DataType newType = DataTypeUtils::fromInt(INT_ARG(0));
  DataType oldType = input->dataType();
  // correct output shape to conform with output data type
  auto inputSize = DataTypeUtils::sizeOf(oldType);
  auto outputSize = DataTypeUtils::sizeOf(newType);
  auto lastSize = outputSize / inputSize;
  if (inputSize < outputSize) {
    REQUIRE_TRUE(static_cast<size_t>(input->sizeAt(-1)) == lastSize, 0,
                 "BITCAST: %llu > %llu. So last dimension should be %i, but %i given.", inputSize, outputSize, lastSize,
                 input->sizeAt(-1));
  }
  if (input->isEmpty()) {
    REQUIRE_TRUE(output->isEmpty(), 0, "BITCAST: If input is empty, output array must also be empty.");
    return sd::Status::OK;
  }

  // just memcpy data
  DataBuffer::memcpy(output->dataBuffer(), input->dataBuffer(), 0, 0);

  return sd::Status::OK;
}
DECLARE_SYN(BitCast, bitcast);

DECLARE_SHAPE_FN(bitcast) {
  auto inShape = inputShape->at(0);
  auto inputRank = shape::rank(inShape);
  auto it = INT_ARG(0);
  DataType newType = DataTypeUtils::fromInt(it);
  DataType oldType = ArrayOptions::dataType(inShape);
  // correct output shape to conform with output data type
  auto inputSize = DataTypeUtils::sizeOf(oldType);
  auto outputSize = DataTypeUtils::sizeOf(newType);

  if (shape::length(inShape) == 0) {
    auto ret = SHAPELIST(ConstantShapeHelper::getInstance().castToDataType(inShape, newType));
    return ret;
  }
  if (inputSize == outputSize) {
    // only type should be changed
    auto ret = SHAPELIST(ConstantShapeHelper::getInstance().castToDataType(inShape, newType));
    return ret;
  } else if (inputSize > outputSize) {
    // range of output increased by 1 with inputSize / outputSize as last dimension
    std::vector<sd::LongType> shapeOf(inputRank + 1);
    int i;
    for (i = 0; i < inputRank; ++i) {
      shapeOf[i] = inShape[i + 1];
    }
    shapeOf[i] = inputSize / outputSize;
    auto outputShape = ConstantShapeHelper::getInstance().bufferForShapeInfo(newType, shape::order(inShape), shapeOf)->primary();
    return SHAPELIST(outputShape);
  }
  REQUIRE_TRUE(shape::sizeAt(inShape, static_cast<size_t>(static_cast<sd::LongType>(-1))) ==
                   static_cast<sd::LongType>(outputSize / inputSize), 0,
               "BITCAST: %llu > %llu. So last dimension should be %i, but %i given.", inputSize, outputSize,
               outputSize / inputSize, shape::sizeAt(inShape, static_cast<sd::LongType>(-1)));
  std::vector<sd::LongType> shapeOf(inputRank - 1);

  for (size_t i = 0; i < shapeOf.size(); ++i) {
    shapeOf[i] = inShape[i + 1];
  }

  auto outputShape = ConstantShapeHelper::getInstance().createShapeInfo(newType, shape::order(inShape), shapeOf);
  return SHAPELIST(outputShape);
}

DECLARE_TYPES(bitcast) {
  getOpDescriptor()->setAllowedInputTypes(sd::DataType::ANY)->setAllowedOutputTypes(sd::DataType::ANY);
}
}  // namespace ops
}  // namespace sd

#endif
