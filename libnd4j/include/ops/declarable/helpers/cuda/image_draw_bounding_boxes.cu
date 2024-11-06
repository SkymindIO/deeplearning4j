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
//  @author sgazeos@gmail.com
//
#include <array/NDArray.h>
#include <system/op_boilerplate.h>

#include "execution/cuda/LaunchDims.h"


namespace sd {
namespace ops {
namespace helpers {

typedef NDArray ColorTable_t;
static NDArray DefaultColorTable(int depth, LaunchContext* context) {
  // std::vector<std::vector<float>> colorTable;
  const LongType kDefaultTableLength = 10;
  const LongType kDefaultChannelLength = 4;
  std::vector<sd::LongType> shape = {kDefaultTableLength, kDefaultChannelLength};
  NDArray colorTable('c', shape,
                     {
                         1,   1,   0,   1,  // yellow
                         0,   0,   1,   1,  // 1: blue
                         1,   0,   0,   1,  // 2: red
                         0,   1,   0,   1,  // 3: lime
                         0.5, 0,   0.5, 1,  // 4: purple
                         0.5, 0.5, 0,   1,  // 5: olive
                         0.5, 0,   0,   1,  // 6: maroon
                         0,   0,   0.5, 1,  // 7: navy blue
                         0,   1,   1,   1,  // 8: aqua
                         1,   0,   1,   1   // 9: fuchsia
                     },
                     FLOAT32, context);

  if (depth == 1) {
    colorTable.assign(1.f);  // all to white when black and white colors
  }
  return colorTable;
}

template <typename T>
static SD_KERNEL void drawBoundingBoxesKernel(T const* images, const LongType* imagesShape, float const* boxes,
                                              const LongType* boxesShape, float const* colorTable,
                                              const LongType* colorTableShape, T* output,
                                              const LongType* outputShape,
                                              LongType batchSize, LongType width, LongType height, LongType channels,
                                              LongType boxSize, LongType colorTableLen) {
  for (auto batch = blockIdx.x; batch < (int)batchSize; batch += gridDim.x) {  // loop by batch
    for (auto boxIndex = 0; boxIndex < boxSize; ++boxIndex) {
      // box with shape
      // auto internalBox = &boxes[b * colorSetSize * 4 + c * 4];//(*boxes)(b, {0})(c, {0});//internalBoxes->at(c);
      auto colorIndex = boxIndex % colorTableLen;  // colorSet->at(c);
      LongType indices0[] = {batch, boxIndex, 0};
      LongType indices1[] = {batch, boxIndex, 1};
      LongType indices2[] = {batch, boxIndex, 2};
      LongType indices3[] = {batch, boxIndex, 3};
      auto rowStart = LongType((height - 1) * boxes[shape::getOffset(boxesShape, indices0, 0)]);
      auto rowStartBound = math::sd_max(LongType(0), rowStart);
      auto rowEnd = LongType((height - 1) * boxes[shape::getOffset(boxesShape, indices2, 0)]);
      auto rowEndBound = math::sd_min(LongType(height - 1), rowEnd);
      auto colStart = LongType((width - 1) * boxes[shape::getOffset(boxesShape, indices1, 0)]);
      auto colStartBound = math::sd_max(LongType(0), colStart);
      auto colEnd = LongType((width - 1) * boxes[shape::getOffset(boxesShape, indices3, 0)]);
      auto colEndBound = math::sd_min(LongType(width - 1), colEnd);
      if (rowStart > rowEnd || colStart > colEnd) {
        continue;
      }
      if (rowStart >= height || rowEnd < 0 || colStart >= width || colEnd < 0) {
        continue;
      }

      // Draw upper line
      if (rowStart >= 0) {
        for (auto j = colStartBound + threadIdx.x; j <= colEndBound; j += blockDim.x)
          for (auto c = 0; c < channels; c++) {
            LongType zPos[] = {batch, rowStart, j, c};
            LongType cPos[] = {colorIndex, c};
            auto cIndex = shape::getOffset(colorTableShape, cPos, 0);
            auto zIndex = shape::getOffset(outputShape, zPos, 0);
            output[zIndex] = (T)colorTable[cIndex];
          }
      }
      // Draw bottom line.
      if (rowEnd < height) {
        for (auto j = colStartBound + threadIdx.x; j <= colEndBound; j += blockDim.x)
          for (auto c = 0; c < channels; c++) {
            LongType zPos[] = {batch, rowEnd, j, c};
            LongType cPos[] = {colorIndex, c};
            auto cIndex = shape::getOffset(colorTableShape, cPos, 0);
            auto zIndex = shape::getOffset(outputShape, zPos, 0);
            output[zIndex] = (T)colorTable[cIndex];
          }
      }

      // Draw left line.
      if (colStart >= 0) {
        for (auto i = rowStartBound + threadIdx.x; i <= rowEndBound; i += blockDim.x)
          for (auto c = 0; c < channels; c++) {
            LongType zPos[] = {batch, i, colStart, c};
            LongType cPos[] = {colorIndex, c};
            auto cIndex = shape::getOffset(colorTableShape, cPos, 0);
            auto zIndex = shape::getOffset(outputShape, zPos, 0);
            output[zIndex] = (T)colorTable[cIndex];
          }
      }
      // Draw right line.
      if (colEnd < width) {
        for (auto i = rowStartBound + threadIdx.x; i <= rowEndBound; i += blockDim.x)
          for (auto c = 0; c < channels; c++) {
            LongType zPos[] = {batch, i, colEnd, c};
            LongType cPos[] = {colorIndex, c};
            auto cIndex = shape::getOffset(colorTableShape, cPos, 0);
            auto zIndex = shape::getOffset(outputShape, zPos, 0);
            output[zIndex] = (T)colorTable[cIndex];
          }
      }
    }
  }
}

template <typename T>
void drawBoundingBoxesH(LaunchContext* context, NDArray * images, NDArray * boxes, NDArray * colors,
                        NDArray* output) {
  auto batchSize = images->sizeAt(0);
  auto height = images->sizeAt(1);
  auto width = images->sizeAt(2);
  auto channels = images->sizeAt(3);
  auto stream = context->getCudaStream();
  auto boxSize = boxes->sizeAt(1);
  NDArray colorsTable = DefaultColorTable(channels, context);
  if ((colors != nullptr && colors->lengthOf() > 0)) {
    colorsTable = *colors;
  }

  auto imagesBuf = images->getDataBuffer()->specialAsT<T>();
  auto boxesBuf = boxes->getDataBuffer()->specialAsT<float>();             // boxes should be float32
  auto colorsTableBuf = colorsTable.getDataBuffer()->specialAsT<float>();  // color table is float32
  auto outputBuf = output->dataBuffer()->specialAsT<T>();
  dim3 launchDims = getLaunchDims("draw_bounding_boxes");
  drawBoundingBoxesKernel<<<launchDims.x, launchDims.y, launchDims.z, *stream>>>(
      imagesBuf, images->specialShapeInfo(), boxesBuf, boxes->specialShapeInfo(), colorsTableBuf,
      colorsTable.specialShapeInfo(), outputBuf, output->specialShapeInfo(), batchSize, width, height, channels,
      boxSize, colorsTable.lengthOf());
}

void drawBoundingBoxesFunctor(LaunchContext* context, NDArray* images, NDArray* boxes, NDArray* colors,
                              NDArray* output) {
  // images - batch of 3D images with BW (last dim = 1), RGB (last dim = 3) or RGBA (last dim = 4) channel set
  // boxes - batch of 2D bounds with last dim (y_start, x_start, y_end, x_end) to compute i and j as
  // floor((height - 1 ) * y_start) => rowStart, floor((height - 1) * y_end) => rowEnd
  // floor((width - 1 ) * x_start) => colStart, floor((width - 1) * x_end) => colEnd
  // height = images->sizeAt(1), width = images->sizeAt(2)
  // colors - colors for each box given
  // set up color for each box as frame
  NDArray::prepareSpecialUse({output}, {images, boxes, colors});
  output->assign(images);
  BUILD_SINGLE_SELECTOR(output->dataType(), drawBoundingBoxesH, (context, images, boxes, colors, output),
                        SD_FLOAT_TYPES);
  NDArray::registerSpecialUse({output}, {images, boxes, colors});
}

}  // namespace helpers
}  // namespace ops
}  // namespace sd
