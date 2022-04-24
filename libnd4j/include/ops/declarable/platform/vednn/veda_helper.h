/*
 *  ******************************************************************************
 *  *
 *  *
 *  * This program and the accompanying materials are made available under the
 *  * terms of the Apache License, Version 2.0 which is available at
 *  * https://www.apache.org/licenses/LICENSE-2.0.
 *  *
 *  * See the NOTICE file distributed with this work for additional
 *  * information regarding copyright ownership.
 *  * Unless required by applicable law or agreed to in writing, software
 *  * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 *  * License for the specific language governing permissions and limitations
 *  * under the License.
 *  *
 *  * SPDX-License-Identifier: Apache-2.0
 *  *****************************************************************************
 */

#ifndef DEV_VEDAHELPERS_H
#define DEV_VEDAHELPERS_H

#include <helpers/logger.h>
#include <stdint.h>
#include <system/Environment.h>
#include <veda.h>

#include <mutex>

#define MAX_DEVICE_USAGE 1
#define VEDA_CALL(err) veda_check(err, __FILE__, __LINE__)

#define VEDA_CALL_THROW(err) veda_throw(veda_check(err, __FILE__, __LINE__))

struct VEDA_STATUS {
  const char* file = nullptr;
  int line = -1;
  VEDAresult status = VEDA_SUCCESS;
  operator bool() const { return status == VEDA_SUCCESS; }

  VEDA_STATUS() = default;

  VEDA_STATUS(VEDAresult result, const char* err_file, int err_line) {
    status = result;
    file = err_file;
    line = err_line;
  }

  std::string getErrorMsg() {
    if (status != VEDA_SUCCESS) {
      const char *name, *str;
      vedaGetErrorName(status, &name);
      vedaGetErrorString(status, &str);
      std::string err;
      if (file) {
        err = std::string(name) + ": " + str + " " + file + ":" + std::to_string(line);
      } else {
        err = std::string(name) + ": " + str;
      }
      return err;
    }
    return std::string{};
  }

  void printTheLatestError() {
    if (status != VEDA_SUCCESS) {
      const char *name, *str;
      vedaGetErrorName(status, &name);
      vedaGetErrorString(status, &str);
      if (file) {
        sd_printf("%s: %s @ %s:%i\n", name, str, file, line);
      } else {
        sd_printf("%s: %s \n", name, str);
      }
    }
  }
};

SD_INLINE VEDA_STATUS veda_check(VEDAresult err, const char* file, const int line) {
  if (err != VEDA_SUCCESS) {
    return VEDA_STATUS(err, file, line);
  }
  return VEDA_STATUS{};
}

SD_INLINE void veda_throw(VEDA_STATUS status) {
  if (!status) {
    throw std::runtime_error(status.getErrorMsg());
  }
}

// Scope to Set context to the current thread
struct SCOPED_VEDA_CONTEXT {
  VEDAcontext ctx;
  SCOPED_VEDA_CONTEXT(VEDAdevice device) {
    vedaDevicePrimaryCtxRetain(&ctx, device);
    vedaCtxPushCurrent(ctx);
  }

  void sync() { VEDA_CALL_THROW(vedaCtxSynchronize()); }

  ~SCOPED_VEDA_CONTEXT() { vedaCtxPopCurrent(&ctx); }
};

struct VEDA_HANDLE {
  using FUNC_NAME_PTR = const char*;
  SD_MAP_IMPL<FUNC_NAME_PTR, VEDAfunction> functionsLookUp;
  VEDAcontext ctx;
  VEDAmodule mod;
  VEDA_STATUS status;
  VEDAdevice device;

  VEDA_HANDLE(const char* library_name, VEDAdevice device_index, const char* dir_name = nullptr)
      : device(device_index) {
    sd_debug("it's loading veda device library: %s\n", library_name);
    auto status = VEDA_CALL(vedaCtxCreate(&ctx, VEDA_CONTEXT_MODE_OMP, 0));
    if (status) {
      if (const char* env_p = std::getenv("DEVICE_LIB_LOADPATH")) {
        std::string path_lib = std::string(env_p) + "/" + library_name;
        status = VEDA_CALL(vedaModuleLoad(&mod, path_lib.c_str()));
      } else if (dir_name) {
        std::string path_lib = std::string(dir_name) + "/" + library_name;
        status = VEDA_CALL(vedaModuleLoad(&mod, path_lib.c_str()));
      } else {
        status = VEDA_CALL(vedaModuleLoad(&mod, library_name));
      }
      if (status) {
        // lets just pop  thecontext from the current thread
        vedaCtxPopCurrent(&ctx);
      } else {
        // lets destroy context as well
        vedaCtxDestroy(ctx);
      }
    }
  }

  VEDAfunction getFunctionByConstPtrName(FUNC_NAME_PTR namePtr) {
    auto searchIter = functionsLookUp.find(namePtr);
    if (searchIter != functionsLookUp.end()) return searchIter->second;
    // insert to our lookUp
    VEDAfunction func;
    auto local_status = VEDA_CALL(vedaModuleGetFunction(&func, mod, namePtr));
    if (local_status) functionsLookUp.emplace(namePtr, func);
    return func;
  }

  VEDAdevice getDevice() { return device; }
};

struct VEDA {
  std::vector<VEDA_HANDLE> ve_handles;

  static VEDA& getInstance() {
    static VEDA instance(VEDA_VEDNN_LIBRARY);
    return instance;
  }

  VEDA_HANDLE& getVEDA_HANDLE(int device_index) {
    // we will let to throw out of range error
    return ve_handles[device_index];
  }

  int getHandlesCount() const { return ve_handles.size();}
 private:
  VEDA(const char* library_name);

  VEDA() = delete;
  VEDA(const VEDA&) = delete;
  VEDA(VEDA&&) = delete;
  VEDA& operator=(const VEDA&) = delete;
  VEDA& operator=(VEDA&&) = delete;

 protected:
  virtual ~VEDA() {}
};

#endif
