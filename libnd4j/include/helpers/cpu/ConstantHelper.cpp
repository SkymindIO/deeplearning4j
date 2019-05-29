/*******************************************************************************
 * Copyright (c) 2015-2018 Skymind, Inc.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License, Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0.
 *
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

#include <ConstantHelper.h>
#include <types/types.h>
#include <loops/type_conversions.h>
#include <type_boilerplate.h>
#include <cstring>

namespace nd4j {
    ConstantHelper::ConstantHelper() {
        int numDevices = getNumberOfDevices();
        _cache.resize(numDevices);
        for (int e = 0; e < numDevices; e++) {
            std::map<ConstantDescriptor, ConstantHolder> map;
            _cache[e] = map;
        }
    }

    ConstantHelper* ConstantHelper::getInstance() {
        if (!_INSTANCE)
            _INSTANCE = new nd4j::ConstantHelper();

        return _INSTANCE;
    }

    void* ConstantHelper::replicatePointer(void *src, size_t numBytes, memory::Workspace *workspace) {
        int8_t *ptr = nullptr;
        ALLOCATE(ptr, workspace, numBytes, int8_t);
        std::memcpy(ptr, src, numBytes);
        return ptr;
    }

    int ConstantHelper::getCurrentDevice() {
        return 0L;
    }

    int ConstantHelper::getNumberOfDevices() {
        return 1;
    }

    ConstantDataBuffer* ConstantHelper::constantBuffer(ConstantDescriptor &descriptor, nd4j::DataType dataType) {
        const auto deviceId = getCurrentDevice();

        if (_cache[deviceId].count(descriptor) == 0) {
            ConstantHolder holder;
            _cache[deviceId][descriptor] = holder;
        }

        ConstantHolder* holder = &_cache[deviceId][descriptor];

        if (holder->hasBuffer(dataType))
            return holder->getConstantDataBuffer(dataType);
        else {
            int8_t *cbuff = new int8_t[descriptor.length() * DataTypeUtils::sizeOf(dataType)];

            // create buffer with this dtype
            if (descriptor.isFloat()) {
                BUILD_DOUBLE_SELECTOR(nd4j::DataType::DOUBLE, dataType, nd4j::TypeCast::convertGeneric, (nullptr, descriptor.floatValues().data(), descriptor.length(), cbuff), (nd4j::DataType::DOUBLE, double), LIBND4J_TYPES);
            } else if (descriptor.isInteger()) {
                BUILD_DOUBLE_SELECTOR(nd4j::DataType::INT64, dataType, nd4j::TypeCast::convertGeneric, (nullptr, descriptor.integerValues().data(), descriptor.length(), cbuff), (nd4j::DataType::INT64, Nd4jLong), LIBND4J_TYPES);
            }

            ConstantDataBuffer dataBuffer(cbuff, nullptr, descriptor.length(), DataTypeUtils::sizeOf(dataType));
            holder->addBuffer(dataBuffer, dataType);

            return holder->getConstantDataBuffer(dataType);
        }
    }

    nd4j::ConstantHelper* nd4j::ConstantHelper::_INSTANCE = 0;
}