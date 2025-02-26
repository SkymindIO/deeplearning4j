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
// Created by raver119 on 13.01.2018.
//
#include <ops/declarable/CustomOperations.h>

#include "testlayers.h"

using namespace sd;
using namespace sd::ops;
using namespace sd::graph;

class BackpropTests : public NDArrayTests {
 public:
};

TEST_F(BackpropTests, Test_Add_1) {
  NDArray x('c', {2, 3, 4}, FLOAT32);
  NDArray y('c', {3, 4}, FLOAT32);
  NDArray e('c', {2, 3, 4}, FLOAT32);

  add_bp op;
  auto result = op.evaluate({&x, &y, &e});

  ASSERT_EQ(sd::Status::OK, result.status());

  auto eps = result.at(0);
  auto grad = result.at(1);

  ASSERT_TRUE(x.isSameShape(eps));
  ASSERT_TRUE(y.isSameShape(grad));
}
