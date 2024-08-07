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
#include <execution/ThreadPool.h>
#include <execution/Threads.h>
#include <loops/type_conversions.h>
#include <ops/declarable/CustomOperations.h>

#include <chrono>

#include "testlayers.h"

using namespace samediff;
using namespace sd;
using namespace sd::ops;
using namespace sd::graph;

class ThreadsTests : public NDArrayTests {
 public:
  ThreadsTests() { sd_printf("\n", ""); }
};

TEST_F(ThreadsTests, th_test_1) {
  ASSERT_EQ(1, ThreadsHelper::numberOfThreads(6, 1023));
  ASSERT_EQ(1, ThreadsHelper::numberOfThreads(6, 1024));
  ASSERT_EQ(1, ThreadsHelper::numberOfThreads(6, 1026));

  ASSERT_EQ(1, ThreadsHelper::numberOfThreads(6, 2043));
  ASSERT_EQ(2, ThreadsHelper::numberOfThreads(6, 2048));
}

TEST_F(ThreadsTests, th_test_2) {
  // in this case we'll get better split over second loop - exactly 32 elements per thread
  ASSERT_EQ(2, ThreadsHelper::pickLoop2d(32, 48, 1024));
  ASSERT_EQ(2, ThreadsHelper::pickLoop2d(6, 4, 16384));

  // in this case we'll get better split over first loop - 2 loops/2048 elements per thread
  ASSERT_EQ(1, ThreadsHelper::pickLoop2d(32, 64, 1024));
  ASSERT_EQ(1, ThreadsHelper::pickLoop2d(6, 6, 16384));

  // in this case none of loops are good enough, but second loop is too small for split
  ASSERT_EQ(1, ThreadsHelper::pickLoop2d(6, 64, 32));

  // all loops are good enough, but we go with bigger one, since small
  ASSERT_EQ(1, ThreadsHelper::pickLoop2d(2, 64, 32));

  // obviously split goes into second loop, to give 1024 elements per thread
  ASSERT_EQ(2, ThreadsHelper::pickLoop2d(2, 1, 2048));
}

TEST_F(ThreadsTests, th_test_3) {
  // typical conv cases
  ASSERT_EQ(1, ThreadsHelper::pickLoop3d(4, 32, 3, 128));
  ASSERT_EQ(2, ThreadsHelper::pickLoop3d(4, 1, 128, 64));
  ASSERT_EQ(3, ThreadsHelper::pickLoop3d(4, 1, 3, 128));

  // checking for optimal threads for conv inference
  ASSERT_EQ(6, ThreadsHelper::numberOfThreads3d(6, 1, 3, 128));
  ASSERT_EQ(4, ThreadsHelper::numberOfThreads3d(4, 1, 3, 128));
  ASSERT_EQ(8, ThreadsHelper::numberOfThreads3d(8, 1, 3, 128));

  // checking for optimal threads for conv training
  ASSERT_EQ(6, ThreadsHelper::numberOfThreads3d(6, 16, 3, 128));
  ASSERT_EQ(6, ThreadsHelper::numberOfThreads3d(6, 8, 3, 128));

  ASSERT_EQ(6, ThreadsHelper::numberOfThreads3d(6, 8, 3, 64));
  ASSERT_EQ(1, ThreadsHelper::pickLoop3d(6, 8, 3, 64));
}

TEST_F(ThreadsTests, th_test_5) {
  ASSERT_EQ(6, ThreadsHelper::numberOfThreads3d(6, 32, 112, 112));

  ASSERT_EQ(1, ThreadsHelper::pickLoop3d(6, 32, 112, 112));

  for (auto e = 0; e < 6; e++) {
    auto span = Span3::build(1, e, 6, 0, 32, 1, 0, 112, 1, 0, 112, 1);
  }
}

TEST_F(ThreadsTests, th_test_4) {
  // typical conv cases
  ASSERT_EQ(2, ThreadsHelper::numberOfThreads2d(2, 32, 3));
  ASSERT_EQ(4, ThreadsHelper::numberOfThreads2d(4, 32, 3));
  ASSERT_EQ(6, ThreadsHelper::numberOfThreads2d(6, 32, 1));
  ASSERT_EQ(8, ThreadsHelper::numberOfThreads2d(8, 16, 64));

  ASSERT_EQ(1, ThreadsHelper::pickLoop2d(4, 32, 1));
  ASSERT_EQ(1, ThreadsHelper::pickLoop2d(8, 19, 17));

  // primes edge cases
  ASSERT_EQ(6, ThreadsHelper::numberOfThreads2d(6, 19, 17));
  ASSERT_EQ(8, ThreadsHelper::numberOfThreads2d(8, 19, 17));

  ASSERT_EQ(1, ThreadsHelper::pickLoop2d(8, 19, 17));

  for (auto e = 0; e < 6; e++) {
    auto span = Span2::build(1, e, 6, 0, 19, 1, 0, 17, 1);

  }
  for (auto e = 0; e < 6; e++) {
    auto span = Span2::build(1, e, 6, 0, 32, 1, 0, 3, 1);
  }
}

TEST_F(ThreadsTests, test_span_converage_1) {
  for (int b = 1; b <= 128; b++) {
    for (int c = 1; c <= 64; c++) {
      for (int t = 1; t <= 64; t++) {
        auto threads = ThreadsHelper::numberOfThreads2d(t, b, c);
        auto loop = ThreadsHelper::pickLoop2d(threads, b, c);

        auto sum = 0;
        for (auto a = 0; a < threads; a++) {
          auto span = Span2::build(loop, a, threads, 0, b, 1, 0, c, 1);

          if (loop == 1)
            sum += span.stopX() - span.startX();
          else if (loop == 2)
            sum += span.stopY() - span.startY();
          else
            THROW_EXCEPTION("Bad loop!");
        }

        if (loop == 1)
          ASSERT_EQ(b, sum);
        else
          ASSERT_EQ(c, sum);
      }
    }
  }
}

TEST_F(ThreadsTests, validation_test_2d_1) {
  if (1 > 0) return;

  std::vector<int> threads({1, 2, 4, 6, 8, 12, 16, 20, 32, 48, 64});

  for (int e = 1; e < 1024; e++) {
    for (int i = 1; i <= 1024; i++) {
      for (auto t : threads) {
        std::atomic<int64_t> sum;
        sum.store(0);

        auto func = PRAGMA_THREADS_FOR_2D {
          for (auto x = start_x; x < stop_x; x += inc_x) {
            for (auto y = start_y; y < stop_y; y += inc_y) {
              sum++;
            }
          }
        };

        Threads::parallel_for(func, 0, e, 1, 0, i, 1, t, true);

        ASSERT_EQ(e * i, sum.load());
      }
    }

  }
}

TEST_F(ThreadsTests, reduction_test_1) {
  auto func = PRAGMA_REDUCE_LONG {
    int64_t sum = 0;

    for (auto e = start; e < stop; e++) {
      sum++;
    };

    return sum;
  };

  auto sum = Threads::parallel_long(
      func, LAMBDA_AL { return _old + _new; }, 0, 8192, 1, 4);
  ASSERT_EQ(8192, sum);
}

static void _code(int thread_id) {
  auto x = NDArrayFactory::create<float>('c', {65536 * 16});
  x.assign(1.1f);
}

TEST_F(ThreadsTests, crash_test_1) {
  if (!Environment::getInstance().isCPU()) return;

  for (int e = 0; e < 3; e++) {
    std::vector<std::thread> threads(std::thread::hardware_concurrency());

    // creating some threads
    for (int t = 0; t < threads.size(); t++) threads[t] = std::thread(_code, t);

    // blocking until everything is finished
    for (auto &t : threads) t.join();
  }
}


