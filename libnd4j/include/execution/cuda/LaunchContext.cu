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
// @author raver119@gmail.com
//
#include <exceptions/cuda_exception.h>
#include <execution/AffinityManager.h>
#include <execution/LaunchContext.h>
#include <helpers/cublasHelper.h>
#include <helpers/logger.h>

#include <thread>

thread_local sd::ContextBuffers contextBuffers = sd::ContextBuffers();

namespace sd {

std::vector<std::shared_ptr<LaunchContext>> LaunchContext::_contexts = std::vector<std::shared_ptr<LaunchContext>>();
std::mutex LaunchContext::_mutex;
SD_MAP_IMPL<int, std::mutex*> LaunchContext::_deviceMutexes;

////////////////////////////////////////////////////////////////////////
LaunchContext::LaunchContext(cudaStream_t* cudaStream, cudaStream_t& specialCudaStream, void* reductionPointer,
                             void* scalarPointer, int* allocationPointer) {

  _workspace = nullptr;
  _isAllocated = false;
}

std::mutex* LaunchContext::deviceMutex() {
  auto deviceId = AffinityManager::currentDeviceId();
  return _deviceMutexes[deviceId];
}

LaunchContext::~LaunchContext() {
  if (_isAllocated) {
  }
}

////////////////////////////////////////////////////////////////////////
LaunchContext::LaunchContext() {
  // default constructor, just to make clang/ranlib happy
  _workspace = nullptr;
  _deviceID = 0;

  _isAllocated = true;
}

LaunchContext::LaunchContext(Pointer cudaStream, Pointer reductionPointer, Pointer scalarPointer,
                             Pointer allocationPointer) {
  _isAllocated = false;

}

LaunchContext* LaunchContext::defaultContext() {
  /**
   * This method returns LaunchContext, that has multiple entities within:
   * 1) temporary buffers. they must be per-thread
   * 2) CUDA stream. it must be either per-thread or per-device
   * 3) cuBLAS handle. it must be per-device
   */
  auto deviceId = AffinityManager::currentDeviceId();

  {
    // we need this block synchronous, to avoid double initialization etc
    std::lock_guard<std::mutex> lock(_mutex);
    if (_contexts.empty()) {
      // create one context per device
      auto numDevices = AffinityManager::numberOfDevices();

      _contexts.resize(numDevices);
      for (int e = 0; e < numDevices; e++) {
        _deviceMutexes[e] = new std::mutex();

        AffinityManager::setCurrentNativeDevice(e);

        _contexts[e] = std::make_shared<LaunchContext>();
      }

      // don't forget to restore device back again
      AffinityManager::setCurrentNativeDevice(deviceId);
    }
  }

  // return context for current device
  return _contexts[deviceId].get();
}

void* LaunchContext::getReductionPointer() const { return contextBuffers.reductionBuffer(); };

void* LaunchContext::getScalarPointer() const { return contextBuffers.scalarBuffer(); };

LongType* LaunchContext::getAllocationPointer() const { return reinterpret_cast<LongType*>(contextBuffers.allocationBuffer()); };

void* LaunchContext::getCublasHandle() const { return CublasHelper::getInstance().handle(); };

void* LaunchContext::getCusolverHandle() const { return CublasHelper::getInstance().solver(); };

cudaStream_t* LaunchContext::getCudaStream() const {
  return reinterpret_cast<cudaStream_t*>(contextBuffers.execStream());
};

cudaStream_t* LaunchContext::getCudaSpecialStream() const {
  return reinterpret_cast<cudaStream_t*>(contextBuffers.specialStream());
  ;
};

void LaunchContext::setReductionPointer(void* reductionPointer) {
  contextBuffers.setReductionBuffer(reductionPointer);
};

void LaunchContext::setScalarPointer(void* scalarPointer) { contextBuffers.setScalarBuffer(scalarPointer); };

void LaunchContext::setAllocationPointer(int* allocationPointer) {
  contextBuffers.setAllocationBuffer(allocationPointer);
};

void LaunchContext::setCudaStream(cudaStream_t* cudaStream){
};

void LaunchContext::setCudaSpecialStream(cudaStream_t* cudaStream){
};

void LaunchContext::setCublasHandle(void* handle) { _cublasHandle = handle; };

void LaunchContext::swapContextBuffers(ContextBuffers& buffers) { contextBuffers = buffers; };

void LaunchContext::releaseBuffers() {
  contextBuffers.release();
}

bool LaunchContext::isInitialized() { return contextBuffers.isInitialized(); }

void* LaunchContext::getCuDnnHandle() const { return CublasHelper::getInstance().cudnn(); }

ErrorReference* LaunchContext::errorReference() { return contextBuffers.errorReference(); }

void* LaunchContext::engine() { return _engine; }
}  // namespace sd
