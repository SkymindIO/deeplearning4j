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
    ///////  CAST CUSTOM INT TYPES

    float8::operator int8() const {
        return static_cast<int8>(cpu_quarter2float(data));
    }

    float8::operator uint8() const {
        return static_cast<uint8>(cpu_quarter2float(data));
    }

    float8::operator int16() const {
        return static_cast<int16>(cpu_quarter2float(data));
    }

    float8::operator uint16() const {
        return static_cast<uint16>(cpu_quarter2float(data));
    }


    ///////  ASSIGN CUSTOM INT TYPES

    void float8::assign(const int8& rhs) {
        assign((float)rhs);
    }
    void float8::assign(const uint8& rhs) {
        assign((float)rhs);
    }

    void float8::assign(const int16& rhs) {
        assign((float)rhs);
    }

    void float8::assign(const uint16& rhs) {
        assign((float)rhs);
    }


    ///////  CAST CUSTOM FLOAT TYPES

    float8::operator float16() const {
        return static_cast<float16>(cpu_quarter2float(data));
    }

    ///////  ASSIGN CUSTOM FLOAT TYPES

    void float8::assign(const float16& rhs) {
        assign((float)rhs);
    }



/*
    template float8::float8(const float& rhs);
    template float8::float8(const double& rhs);

    template float8& float8::operator=<float>(const float& rhs);
    template float8& float8::operator=<double>(const double& rhs);
    */
}