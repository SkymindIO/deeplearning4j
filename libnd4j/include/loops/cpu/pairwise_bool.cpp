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
// Created by remote on 2018-09-20.
//
#include <loops/pairwise_bool.h>
#include <types/types.h>
#include <helpers/LoopKind.h>
#include <helpers/OmpLaunchHelper.h>
#include <execution/Threads.h>

using namespace simdOps;

namespace functions {
namespace pairwise_transforms {

template <typename X, typename Y>
void PairWiseBoolTransform<X, Y>::exec(int opNum, const void *x, sd::LongType xEws, const void *y,
                                      sd::LongType yEws, void *z, sd::LongType zEws, void *extraParams, sd::LongType n,
                                      sd::LongType start, sd::LongType stop) {
 DISPATCH_BY_OPNUM_TT(exec, PARAMS(x, xEws, y, yEws, z, zEws, extraParams, n, start, stop), PAIRWISE_BOOL_OPS);
};

template <typename X, typename Z>
template <typename OpType>
void PairWiseBoolTransform<X, Z>::exec(const void *vx, sd::LongType xEws, const void *vy, sd::LongType yEws, void *vz,
                                      sd::LongType zEws, void *vextraParams, sd::LongType n, sd::LongType start,
                                      sd::LongType stop) {
 auto x = reinterpret_cast<const X *>(vx);
 auto y = reinterpret_cast<const X *>(vy);
 auto z = reinterpret_cast<Z *>(vz);
 auto extraParams = reinterpret_cast<X *>(vextraParams);

 if (xEws == 1 && yEws == 1 && zEws == 1) {
   PRAGMA_OMP_SIMD
   for (sd::LongType i = start; i < stop; i++) z[i] = OpType::op(x[i], y[i], extraParams);
 } else {
   PRAGMA_OMP_SIMD
   for (sd::LongType i = start; i < stop; i++) z[i * zEws] = OpType::op(x[i * xEws], y[i * yEws], extraParams);
 }
}

template <typename X, typename Y>
void PairWiseBoolTransform<X, Y>::exec(int opNum, const void *x, const sd::LongType *xShapeInfo, const void *y,
                                      const sd::LongType *yShapeInfo, void *z, const sd::LongType *zShapeInfo,
                                      void *extraParams, sd::LongType start, sd::LongType stop) {
 DISPATCH_BY_OPNUM_TT(exec, PARAMS(x, xShapeInfo, y, yShapeInfo, z, zShapeInfo, extraParams, start, stop),
                      PAIRWISE_BOOL_OPS);
};

template <typename X, typename Z>
template <typename OpType>
void PairWiseBoolTransform<X, Z>::exec(const void *vx, const sd::LongType *xShapeInfo, const void *vy,
                                      const sd::LongType *yShapeInfo, void *vz, const sd::LongType *zShapeInfo,
                                      void *vextraParams, sd::LongType start, sd::LongType stop) {
 auto x = reinterpret_cast<const X *>(vx);
 auto y = reinterpret_cast<const X *>(vy);
 auto z = reinterpret_cast<Z *>(vz);
 auto extraParams = reinterpret_cast<X *>(vextraParams);

 sd::LongType n = shape::length(xShapeInfo);
 sd::LongType xEws = shape::elementWiseStride(xShapeInfo);
 sd::LongType yEws = shape::elementWiseStride(yShapeInfo);
 sd::LongType zEws = shape::elementWiseStride(zShapeInfo);

 if (shape::isScalar(yShapeInfo)) {
   if (shape::haveSameShapeAndStrides(xShapeInfo, zShapeInfo)) {
     PRAGMA_OMP_SIMD
     for (sd::LongType i = start; i < stop; i++) {
       sd::LongType coords[SD_MAX_RANK];
       shape::index2coords(i, xShapeInfo, coords);
       sd::LongType offset = shape::getOffset(xShapeInfo, coords);
       z[offset] = OpType::op(x[offset], y[0], extraParams);
     };
   } else {
     PRAGMA_OMP_SIMD
     for (sd::LongType i = start; i < stop; i++) {
       sd::LongType coords[SD_MAX_RANK];
       shape::index2coords(i, xShapeInfo, coords);
       sd::LongType xOffset = shape::getOffset(xShapeInfo, coords);
       sd::LongType zOffset = shape::getOffset(zShapeInfo, coords);
       z[zOffset] = OpType::op(x[xOffset], y[0], extraParams);
     };
   }
   return;
 }

 const sd::LoopKind::Kind kindOfLoop = sd::LoopKind::deduceKindOfLoopXYZ(xShapeInfo, yShapeInfo, zShapeInfo);
 const bool sameShapesXY = shape::shapeEquals(xShapeInfo, yShapeInfo);
 const bool isSameLength = shape::length(xShapeInfo) == shape::length(yShapeInfo);
 if ((kindOfLoop == sd::LoopKind::EWS1 || kindOfLoop == sd::LoopKind::EWSNONZERO) && sameShapesXY) {
   exec<OpType>(x, xEws, y, yEws, z, zEws, extraParams, n, start, stop);
 } else if ((kindOfLoop == sd::LoopKind::EWS1 || kindOfLoop == sd::LoopKind::EWSNONZERO) &&
            !sameShapesXY && isSameLength) {  // not same shape, same length array
   exec<OpType>(x, xEws, y, yEws, z, zEws, extraParams, shape::length(yShapeInfo), start, stop);
 } else {
   if (shape::haveSameShapeAndStrides(xShapeInfo, yShapeInfo) &&
       shape::haveSameShapeAndStrides(xShapeInfo, zShapeInfo)) {
     PRAGMA_OMP_SIMD
     for (sd::LongType i = start; i < stop; i++) {
       sd::LongType coords[SD_MAX_RANK];
       shape::index2coords(i, xShapeInfo, coords);
       sd::LongType offset = shape::getOffset(xShapeInfo, coords);
       z[offset] = OpType::op(x[offset], y[offset], extraParams);
     };
   } else if (shape::haveSameShapeAndStrides(xShapeInfo, yShapeInfo)) {
     PRAGMA_OMP_SIMD
     for (sd::LongType i = start; i < stop; i++) {
       sd::LongType coords[SD_MAX_RANK];
       shape::index2coords(i, xShapeInfo, coords);
       auto offset = shape::getOffset(xShapeInfo, coords);
       auto zOffset = shape::getOffset(zShapeInfo, coords);
       z[zOffset] = OpType::op(x[offset], y[offset], extraParams);
     };
   } else if (shape::haveSameShapeAndStrides(xShapeInfo, zShapeInfo)) {
     PRAGMA_OMP_SIMD
     for (sd::LongType i = start; i < stop; i++) {
       sd::LongType coords[SD_MAX_RANK];
       shape::index2coords(i, xShapeInfo, coords);
       auto offset = shape::getOffset(xShapeInfo, coords);
       auto yOffset = shape::getOffset(yShapeInfo, coords);
       z[offset] = OpType::op(x[offset], y[yOffset], extraParams);
     };
   } else if (shape::haveSameShapeAndStrides(yShapeInfo, zShapeInfo)) {
     PRAGMA_OMP_SIMD
     for (sd::LongType i = start; i < stop; i++) {
       sd::LongType coords[SD_MAX_RANK];
       shape::index2coords(i, yShapeInfo, coords);
       sd::LongType xOffset = shape::getOffset(xShapeInfo, coords);
       sd::LongType offset = shape::getOffset(yShapeInfo, coords);
       z[offset] = OpType::op(x[xOffset], y[offset], extraParams);
     };
   } else {
     PRAGMA_OMP_SIMD
     for (sd::LongType i = start; i < stop; i++) {
       sd::LongType coords[SD_MAX_RANK];
       shape::index2coords(i, zShapeInfo, coords);
       sd::LongType xOffset = shape::getOffset(xShapeInfo, coords);
       sd::LongType yOffset = shape::getOffset(yShapeInfo, coords);
       sd::LongType zOffset = shape::getOffset(zShapeInfo, coords);
       z[zOffset] = OpType::op(x[xOffset], y[yOffset], extraParams);
     };
   }
 }
}

BUILD_DOUBLE_TEMPLATE(template class PairWiseBoolTransform, , SD_COMMON_TYPES, SD_BOOL_TYPES);
}  // namespace pairwise_transforms
}  // namespace functions