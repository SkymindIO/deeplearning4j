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
// This is special snowflake. This file builds bindings for ops availability tests
//
// @author raver119@gmail.com
//

#include <helpers/OpTracker.h>
#include <loops/legacy_ops.h>
#include <ops/declarable/CustomOperations.h>

namespace sd {

_loader::_loader() {
  //
  OpTracker::getInstance();

  BUILD_TRACKER(::graph::OpType_TRANSFORM_SAME, TRANSFORM_FLOAT_OPS);
  BUILD_TRACKER(::graph::OpType_TRANSFORM_SAME, TRANSFORM_SAME_OPS);
  BUILD_TRACKER(::graph::OpType_TRANSFORM_SAME, TRANSFORM_BOOL_OPS);
  BUILD_TRACKER(::graph::OpType_BROADCAST, BROADCAST_OPS);
  BUILD_TRACKER(::graph::OpType_PAIRWISE, PAIRWISE_TRANSFORM_OPS);
  BUILD_TRACKER(::graph::OpType_RANDOM, RANDOM_OPS);
  BUILD_TRACKER(::graph::OpType_REDUCE_FLOAT, REDUCE_FLOAT_OPS);
  BUILD_TRACKER(::graph::OpType_REDUCE_SAME, REDUCE_SAME_OPS);
  BUILD_TRACKER(::graph::OpType_REDUCE_BOOL, REDUCE_BOOL_OPS);
  BUILD_TRACKER(::graph::OpType_REDUCE_3, REDUCE3_OPS);
  BUILD_TRACKER(::graph::OpType_INDEX_REDUCE, INDEX_REDUCE_OPS);
  BUILD_TRACKER(::graph::OpType_SCALAR, SCALAR_OPS);
  BUILD_TRACKER(::graph::OpType_SUMMARYSTATS, SUMMARY_STATS_OPS);
};

static sd::_loader loader;
}  // namespace sd
