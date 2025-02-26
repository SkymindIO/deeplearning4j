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
//  @author raver119@gmail.com
//
#include <exceptions/graph_execution_exception.h>
#include <exceptions/graph_exists_exception.h>
#include <graph/GraphExecutioner.h>
#include <graph/GraphHolder.h>

namespace sd {
namespace graph {
GraphHolder& GraphHolder::getInstance() {
  static GraphHolder instance;
  return instance;
};

void GraphHolder::registerGraph(sd::LongType graphId, Graph* graph) {
  if (hasGraphAny(graphId)) throw graph_exists_exception(graphId);

  _graphF[graphId] = graph;

  sd::SimpleReadWriteLock lock;
  _locks[graphId] = lock;
}

Graph* GraphHolder::cloneGraph(sd::LongType graphId) {
  if (!this->hasGraph(graphId)) {
    sd_printf("GraphHolder doesn't have graph stored for [%lld]\n", graphId);
    THROW_EXCEPTION("Bad argument");
  }

  auto graph = _graphF[graphId]->cloneWithProxy();

  return graph;
}

Graph* GraphHolder::pullGraph(sd::LongType graphId) {
  if (!this->hasGraph(graphId)) {
    sd_printf("GraphHolder doesn't have graph stored for [%lld]\n", graphId);
    THROW_EXCEPTION("Bad argument");
  }

  auto graph = _graphF[graphId];

  return graph;
}

void GraphHolder::forgetGraph(sd::LongType graphId) {
  if (this->hasGraph(graphId)) _graphF.erase(graphId);
}

void GraphHolder::dropGraph(sd::LongType graphId) {
  if (this->hasGraph(graphId)) {
    auto g = _graphF[graphId];
    forgetGraph(graphId);
    delete g;
  }
}

void GraphHolder::dropGraphAny(sd::LongType graphId) {
  if (!hasGraphAny(graphId)) return;

  this->lockWrite(graphId);

  this->dropGraph(graphId);

  this->unlockWrite(graphId);
}

bool GraphHolder::hasGraphAny(sd::LongType graphId) { return this->hasGraph(graphId); }

bool GraphHolder::hasGraph(sd::LongType graphId) { return _graphF.count(graphId) > 0; }

void GraphHolder::replaceGraph(sd::LongType graphId, Graph* graph) {
  if (!hasGraph(graphId)) {
    registerGraph(graphId, graph);
    return;
  }

  this->lockWrite(graphId);

  _graphF[graphId] = graph;

  this->unlockWrite(graphId);
}

flatbuffers::Offset<::graph::FlatResult> GraphHolder::execute(sd::LongType graphId, flatbuffers::FlatBufferBuilder& builder,
                                                     const ::graph::FlatInferenceRequest* request) {
  if (!hasGraph(graphId)) throw unknown_graph_exception(graphId);

  lockRead(graphId);

  auto graph = cloneGraph(graphId);
  auto res = GraphExecutioner::execute(graph, builder, request);
  delete graph;

  unlockRead(graphId);

  return res;
}
}  // namespace graph
}  // namespace sd
