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

//
// @author raver119@gmail.com
//
#include <array/NDArray.h>
#include <helpers/GradCheck.h>
#include <helpers/RandomLauncher.h>
#include <ops/declarable/CustomOperations.h>
#include <ops/ops.h>

#include "testlayers.h"

using namespace sd;

class AttentionTests : public NDArrayTests {
 public:
  AttentionTests() {
    printf("\n");
    fflush(stdout);
  }
};

TEST_F(AttentionTests, basic_dot_product_attention) {
  auto keys = NDArrayFactory::create<float>('c', {10, 4, 3});
  auto values = NDArrayFactory::create<float>('c', {10, 4, 3});
  auto queries = NDArrayFactory::create<float>('c', {10, 4, 1});

  ops::dot_product_attention op;
  auto result = op.evaluate({&queries, &keys, &values}, {1, 0});
  ASSERT_EQ(sd::Status::OK, result.status());
}

TEST_F(AttentionTests, basic_dot_product_attention_with_weights) {
  auto keys = NDArrayFactory::create<float>('c', {10, 4, 3});
  auto values = NDArrayFactory::create<float>('c', {10, 4, 3});
  auto queries = NDArrayFactory::create<float>('c', {10, 4, 1});

  ops::dot_product_attention op;
  auto result = op.evaluate({&queries, &keys, &values}, {1, 1});
  ASSERT_EQ(sd::Status::OK, result.status());
}

TEST_F(AttentionTests, basic_dot_product_attention_with_mask) {
  auto keys = NDArrayFactory::create<float>('c', {10, 4, 3});
  auto values = NDArrayFactory::create<float>('c', {10, 4, 3});
  auto queries = NDArrayFactory::create<float>('c', {10, 4, 1});
  auto mask = NDArrayFactory::create<float>('c', {10, 3});
  mask.assign(1.);

  ops::dot_product_attention op;
  auto result = op.evaluate({&queries, &keys, &values, &mask}, {1, 0});
  ASSERT_EQ(sd::Status::OK, result.status());
}



TEST_F(AttentionTests, multi_head_input_dot_product_attention_with_mask) {
  auto keys = NDArrayFactory::create<float>('c', {2, 5, 4, 3});
  auto values = NDArrayFactory::create<float>('c', {2, 5, 4, 3});
  auto queries = NDArrayFactory::create<float>('c', {2, 5, 4, 1});
  auto mask = NDArrayFactory::create<float>('c', {2, 3});
  mask.assign(1.);

  ops::dot_product_attention op;
  auto result = op.evaluate({&queries, &keys, &values, &mask}, {1, 0});
  ASSERT_EQ(sd::Status::OK, result.status());
}



TEST_F(AttentionTests, basic_multi_head_dot_product_attention) {
  auto keys = NDArrayFactory::create<float>('c', {10, 4, 5});
  auto values = NDArrayFactory::create<float>('c', {10, 4, 5});
  auto queries = NDArrayFactory::create<float>('c', {10, 4, 2});

  auto Wk = NDArrayFactory::create<float>('c', {2, 3, 4});
  auto Wv = NDArrayFactory::create<float>('c', {2, 3, 4});
  auto Wq = NDArrayFactory::create<float>('c', {2, 3, 4});
  auto Wo = NDArrayFactory::create<float>('c', {2 * 3, 4});

  ops::multi_head_dot_product_attention op;
  auto result = op.evaluate({&queries, &keys, &values, &Wk, &Wv, &Wq, &Wo}, {1, 0});
  ASSERT_EQ(sd::Status::OK, result.status());
}


TEST_F(AttentionTests, basic_multi_head_dot_product_attention_with_mask) {
  auto keys = NDArrayFactory::create<float>('c', {10, 4, 5});
  auto values = NDArrayFactory::create<float>('c', {10, 4, 5});
  auto queries = NDArrayFactory::create<float>('c', {10, 4, 2});

  auto Wk = NDArrayFactory::create<float>('c', {2, 3, 4});
  auto Wv = NDArrayFactory::create<float>('c', {2, 3, 4});
  auto Wq = NDArrayFactory::create<float>('c', {2, 3, 4});
  auto Wo = NDArrayFactory::create<float>('c', {2 * 3, 4});

  auto mask = NDArrayFactory::create<float>('c', {10, 5});
  mask.assign(1.);

  ops::multi_head_dot_product_attention op;
  auto result = op.evaluate({&queries, &keys, &values, &Wk, &Wv, &Wq, &Wo, &mask}, {1, 0});
  ASSERT_EQ(sd::Status::OK, result.status());
}

