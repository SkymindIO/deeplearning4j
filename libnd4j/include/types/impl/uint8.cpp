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
// Created by raver119 on 07/11/17.
//

#include <types/float8.h>
#include <types/float16.h>
#include <types/int8.h>
#include <types/uint8.h>
#include <types/int16.h>
#include <types/uint16.h>

namespace nd4j {
    ///////  CAST INT TYPES

    uint8::operator int8() const {
        return int8(static_cast<int8_t>(data));
    }

    uint8::operator int16() const {
        return int16(static_cast<int16_t>(data));
    }

    uint8::operator uint16() const {
        return uint16(static_cast<uint16_t>(data));
    }

    ///////  ASSIGN INT TYPES

    void uint8::assign(const int8& rhs) {
        assign(static_cast<uint8_t>(rhs.data));
    }

    void uint8::assign(const int16& rhs) {
        assign(static_cast<uint8_t>(rhs.data));
    }

    void uint8::assign(const uint16& rhs) {
        assign(static_cast<uint8_t>(rhs.data));
    }

    ///////  CAST CUSTOM FLOAT TYPES
    uint8::operator float8() const {
        return static_cast<float8>(cpu_uint82float(data));
    }

    uint8::operator float16() const {
        return static_cast<float16>(cpu_uint82float(data));
    }

    ///////  ASSIGN CUSTOM FLOAT TYPES
    void uint8::assign(const float8& rhs) {
        assign((float)rhs);
    }

    void uint8::assign(const float16& rhs) {
        assign((float)rhs);
    }

}