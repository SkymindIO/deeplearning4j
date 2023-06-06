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
// Created by Yurii Shyrma on 02.01.2018
//
#include <array/ResultSet.h>
#include <exceptions/cuda_exception.h>
#include <helpers/ConstantTadHelper.h>
#include <helpers/PointersManager.h>
#include <helpers/ShapeUtils.h>
#include <helpers/TAD.h>
#include <ops/declarable/helpers/stack.h>

namespace sd {
namespace ops {
namespace helpers {

///////////////////////////////////////////////////////////////////
template <typename T>
static SD_KERNEL void stackScalarsCuda(void* pVx, void* vz, const sd::LongType* zShapeInfo) {
  T* z = reinterpret_cast<T*>(vz);

  __shared__ sd::LongType zLen, totalThreads;

  if (threadIdx.x == 0) {
    zLen = shape::length(zShapeInfo);
    totalThreads = gridDim.x * blockDim.x;
  }
  __syncthreads();

  const auto tid = blockIdx.x * blockDim.x + threadIdx.x;

  for (sd::LongType i = tid; i < zLen; i += totalThreads) {
    const T* x = reinterpret_cast<const T*>(reinterpret_cast<void**>(pVx)[i]);
    z[shape::getIndexOffset(i, zShapeInfo)] = *x;
  }
}

///////////////////////////////////////////////////////////////////
template <typename T>
SD_HOST static void stackScalarsCudaLauncher(const int blocksPerGrid, const int threadsPerBlock,
                                             const cudaStream_t* stream, void* pVx, void* vz,
                                             const sd::LongType* zShapeInfo) {
  stackScalarsCuda<T><<<blocksPerGrid, threadsPerBlock, 256, *stream>>>(pVx, vz, zShapeInfo);
}

///////////////////////////////////////////////////////////////////
template <typename T>
static void stack_(sd::LaunchContext* context, const std::vector<const NDArray*>& inArrs, NDArray& output,
                   const int dim) {
  const int numOfSubArrs = inArrs.size();

  NDArray::prepareSpecialUse({&output}, inArrs);

  if (inArrs[0]->rankOf() == 0) {
    std::vector<void const*> hInBuffers(numOfSubArrs);

    for (int i = 0; i < numOfSubArrs; ++i) hInBuffers[i] = inArrs[i]->specialBuffer();

    PointersManager manager(context, "helpers::stack cuda");

    void* dInBuffers = manager.replicatePointer(hInBuffers.data(), hInBuffers.size() * sizeof(void*));

    const int threadsPerBlock = SD_MAX_NUM_THREADS / 2;
    const int blocksPerGrid = (output.lengthOf() + threadsPerBlock - 1) / threadsPerBlock;

    stackScalarsCudaLauncher<T>(blocksPerGrid, threadsPerBlock, context->getCudaStream(), dInBuffers,
                                output.specialBuffer(), output.specialShapeInfo());

    manager.synchronize();
  } else {
    auto zTadPack = ConstantTadHelper::getInstance().tadForDimensions(
        output.shapeInfo(), ShapeUtils::evalDimsToExclude(output.rankOf(), {dim}));
    auto zTadShapeInfo = zTadPack->primaryShapeInfo();

    for (sd::LongType i = 0; i < numOfSubArrs; ++i) {
      void* zBuff = output.specialBufferWithOffset(zTadPack->primaryOffsets()[i]);

      NativeOpExecutioner::execTransformAny(context, transform::Assign, nullptr, inArrs[i]->shapeInfo(),
                                            inArrs[i]->specialBuffer(), inArrs[i]->specialShapeInfo(), nullptr,
                                            zTadShapeInfo, zBuff, zTadPack->specialShapeInfo(), nullptr, nullptr,
                                            nullptr, false /*allowParallelism*/);
    }
  }

  NDArray::registerSpecialUse({&output}, inArrs);
}

////////////////////////////////////////////////////////////////////////
void stack(sd::LaunchContext* context, const std::vector<const NDArray*>& inArrs, NDArray& output, const int dim) {
  BUILD_SINGLE_SELECTOR(output.dataType(), stack_, (context, inArrs, output, dim), SD_COMMON_TYPES);
}
BUILD_SINGLE_TEMPLATE(template void stack_,
                      (sd::LaunchContext * context, const std::vector<const NDArray*>& inArrs, NDArray& output,
                       const int dim),
                      SD_COMMON_TYPES);

///////////////////////////////////////////////////////////////////
template <typename T>
static SD_KERNEL void unstackScalarsCuda(const void* vx, const sd::LongType* xShapeInfo, void* pVz) {
  const T* x = reinterpret_cast<const T*>(vx);

  __shared__ sd::LongType xLen, totalThreads;

  if (threadIdx.x == 0) {
    xLen = shape::length(xShapeInfo);
    totalThreads = gridDim.x * blockDim.x;
  }
  __syncthreads();

  const auto tid = blockIdx.x * blockDim.x + threadIdx.x;

  for (sd::LongType i = tid; i < xLen; i += totalThreads) {
    T* z = reinterpret_cast<T*>(reinterpret_cast<void**>(pVz)[i]);
    *z = x[shape::getIndexOffset(i, xShapeInfo)];
  }
}

///////////////////////////////////////////////////////////////////
template <typename T>
SD_HOST static void unstackScalarsCudaLauncher(const int blocksPerGrid, const int threadsPerBlock,
                                               const cudaStream_t* stream, const void* vx,
                                               const sd::LongType* xShapeInfo, void* pVz) {
  unstackScalarsCuda<T><<<blocksPerGrid, threadsPerBlock, 256, *stream>>>(vx, xShapeInfo, pVz);
}

///////////////////////////////////////////////////////////////////
template <typename T>
static void unstack_(sd::LaunchContext* context, const NDArray& input, const std::vector<NDArray*>& outArrs,
                     const int dim) {
  const int numOfSubArrs = outArrs.size();

  // NDArray::prepareSpecialUse(outArrs, {&input});
  input.syncToDevice();
  for (const auto a : outArrs) a->getDataBuffer()->allocateSpecial();

  if (outArrs[0]->rankOf() == 0) {
    std::vector<void*> hOutBuffers(numOfSubArrs);

    for (int i = 0; i < numOfSubArrs; ++i) hOutBuffers[i] = outArrs[i]->specialBuffer();

    PointersManager manager(context, "helpers::unstack cuda");

    void* dOutBuffers = manager.replicatePointer(hOutBuffers.data(), hOutBuffers.size() * sizeof(void*));

    const int threadsPerBlock = SD_MAX_NUM_THREADS / 2;
    const int blocksPerGrid = (input.lengthOf() + threadsPerBlock - 1) / threadsPerBlock;

    unstackScalarsCudaLauncher<T>(blocksPerGrid, threadsPerBlock, context->getCudaStream(), input.specialBuffer(),
                                  input.specialShapeInfo(), dOutBuffers);

    manager.synchronize();
  } else {
    auto xTadPack = ConstantTadHelper::getInstance().tadForDimensions(
        input.shapeInfo(), ShapeUtils::evalDimsToExclude(input.rankOf(), {dim}));
    auto xTadShapeInfo = xTadPack->primaryShapeInfo();

    for (sd::LongType i = 0; i < numOfSubArrs; ++i) {
      auto xBuff = input.specialBufferWithOffset(xTadPack->primaryOffsets()[i]);

      NativeOpExecutioner::execTransformAny(input.getContext(), transform::Assign, nullptr, xTadShapeInfo, xBuff,
                                            xTadPack->specialShapeInfo(), nullptr, outArrs[i]->shapeInfo(),
                                            outArrs[i]->specialBuffer(), outArrs[i]->specialShapeInfo(), nullptr,
                                            nullptr, nullptr, false /*allowParallelism*/);
    }
  }

  // NDArray::registerSpecialUse(outArrs, {&input});
  input.tickReadDevice();
  for (const auto p : outArrs) p->tickWriteDevice();
}

////////////////////////////////////////////////////////////////////////
void unstack(sd::LaunchContext* context, const NDArray& input, const std::vector<NDArray*>& outArrs, const int dim) {
  BUILD_SINGLE_SELECTOR(input.dataType(), unstack_, (context, input, outArrs, dim), SD_COMMON_TYPES);
}
BUILD_SINGLE_TEMPLATE(template void unstack_,
                      (sd::LaunchContext * context, const NDArray& input, const std::vector<NDArray*>& outArrs,
                       const int dim),
                      SD_COMMON_TYPES);



}  // namespace helpers
}  // namespace ops
}  // namespace sd
