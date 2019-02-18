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
// Created by raver119 on 20.11.17.
//

#include "testlayers.h"
#include <Graph.h>
#include <chrono>
#include <Node.h>
#include <ops/declarable/CustomOperations.h>
#include <graph/profiling/GraphProfilingHelper.h>
#include <type_conversions.h>
#include <helpers/threshold.h>
#include <ops/ops.h>
#include <OmpLaunchHelper.h>

using namespace nd4j;
using namespace nd4j::graph;

class PlaygroundTests : public testing::Test {
public:
    int numIterations = 3;
    int poolSize = 10;

    PlaygroundTests() {
        printf("\n");
        fflush(stdout);
    }
};

TEST_F(PlaygroundTests, Test_Reduce_Mechanics) {
    auto length = 8192;
    auto x = new double[length];
    double finalVal = 0.0;
    for (int e = 0; e < length; e++) {
        x[e] = 1.0;
    }

    BlockInformation info(length, 1024);
    auto blocks = new double[info.threads];

    printf("num_threads: [%i]\n", info.threads);

#pragma omp parallel num_threads(info.threads) if (info.threads > 1) proc_bind(AFFINITY) default(shared)
    {
        double local = 0.0;
        for (int i = omp_get_thread_num(); i < info.chunks; i += info.threads) {
            Nd4jLong newOffset = (i * info.items);
            auto chunk = x + newOffset;
            Nd4jLong itemsToLoop = info.items;
            if (i * info.items >= length) {
                break;
            }

            //handle modulo case
            if (newOffset + info.items >= length) {
                itemsToLoop = length - newOffset;
            }

// FIXME: proper reduction should be used here
            for (Nd4jLong j = 0; j < itemsToLoop && i * info.items + j < length; j++) {
                auto curr = simdOps::Mean<double,double>::op(chunk[j], nullptr);
                local = simdOps::Mean<double,double>::update(local, curr, nullptr);
            }

        }

        blocks[omp_get_thread_num()] = local;
    }

// FIXME: proper reduction should be used here
    for (int i = 0; i < info.threads; i++) {
        finalVal = simdOps::Mean<double,double>::update(finalVal, blocks[i], nullptr);
    }


    finalVal = simdOps::Mean<double,double>::postProcess(finalVal, length, nullptr);
    delete[] blocks;
    delete[] x;
    printf("result: [%f]\n", (float) finalVal);
    ASSERT_NEAR(1.0, finalVal, 1e-5);
}

TEST_F(PlaygroundTests, LambdaTest_1) {
    auto array = NDArrayFactory::create<float>('c', {8192, 1024});
    array.linspace(1);

    auto lambda = LAMBDA_F(_x) {
        return _x + 32.12f;
    };

    auto timeStart = std::chrono::system_clock::now();

    for (int e = 0; e < numIterations; e++) {
        array.applyLambda<float>(lambda);
    }


    auto timeEnd = std::chrono::system_clock::now();

    auto outerTime = std::chrono::duration_cast<std::chrono::microseconds> (timeEnd - timeStart).count();

    // nd4j_printf("Lambda 1 time %lld us\n", outerTime / numIterations);
}




TEST_F(PlaygroundTests, LambdaTest_2) {
    auto array = NDArrayFactory::create<float>('c', {8192, 1024});
    auto row = NDArrayFactory::create<float>('c', {1, 1024});
    array.linspace(1);

    auto lambda = LAMBDA_F(_x) {
        return _x + 32.12f;
    };

    auto timeStart = std::chrono::system_clock::now();

    for (int e = 0; e < numIterations; e++) {
        array.applyBroadcast(broadcast::Add, {1}, &row);
    }


    auto timeEnd = std::chrono::system_clock::now();

    auto outerTime = std::chrono::duration_cast<std::chrono::microseconds> (timeEnd - timeStart).count();

    // nd4j_printf("Broadcast time %lld us\n", outerTime / numIterations);
}


TEST_F(PlaygroundTests, NoCacheTest_1) {
    std::vector<NDArray*> pool(poolSize);
    auto source = NDArrayFactory::create<float>('c', {8192, 1024});
    for (int e = 0; e < pool.size(); e++)
        pool[e] = source.dup();

    auto lambda = LAMBDA_F(_x) {
        return _x * 32.12f;
    };

    auto timeStart = std::chrono::system_clock::now();
    int cnt = 0;
    for (int e = 0; e < numIterations; e++) {
        auto v = pool[poolSize - 1 - (cnt++)];
        v->applyLambda<float>(lambda);

        if (cnt == poolSize)
            cnt = 0;
    }

    auto timeEnd = std::chrono::system_clock::now();

    auto outerTime = std::chrono::duration_cast<std::chrono::microseconds> (timeEnd - timeStart).count();

    // nd4j_printf("Non-cached time %lld us\n", outerTime / numIterations);

    for (auto v: pool)
        delete v;
}


TEST_F(PlaygroundTests, NoCacheTest_2) {
    std::vector<NDArray*> pool1(poolSize);
    std::vector<NDArray*> pool2(poolSize);
    auto source = NDArrayFactory::create<float>('c', {8192, 1024});
    for (int e = 0; e < pool1.size(); e++) {
        pool1[e] = source.dup();
        pool2[e] = source.dup();
    }

    auto lambda = LAMBDA_FF(_x, _y) {
        return _x * 32.12f + _y;
    };

    auto timeStart = std::chrono::system_clock::now();
    int cnt = 0;
    for (int e = 0; e < numIterations; e++) {
        auto v1 = pool1[poolSize - 1 - cnt];
        auto v2 = pool2[cnt++];
        v1->applyPairwiseLambda<float>(v2, lambda);

        if (cnt == poolSize)
            cnt = 0;
    }

    auto timeEnd = std::chrono::system_clock::now();

    auto outerTime = std::chrono::duration_cast<std::chrono::microseconds> (timeEnd - timeStart).count();

    // nd4j_printf("Non-cached PWT time %lld us\n", outerTime / numIterations);

    for (auto v: pool1)
        delete v;

    for (auto v: pool2)
        delete v;
}


TEST_F(PlaygroundTests, ReductionTest_1) {
    std::vector<NDArray*> pool1(poolSize);
    std::vector<NDArray*> pool2(poolSize);
    auto source = NDArrayFactory::create<float>('c', {1, 100});
    for (int e = 0; e < pool1.size(); e++) {
        pool1[e] = source.dup();
        pool2[e] = source.dup();
    }


    auto timeStart = std::chrono::system_clock::now();
    int cnt = 0;
    for (int e = 0; e < 100000; e++) {
        auto v = pool1[poolSize - 1 - cnt];
        auto r = v->sumNumber();

        if (cnt == poolSize)
            cnt = 0;
    }
    auto timeEnd = std::chrono::system_clock::now();

    auto outerTime = std::chrono::duration_cast<std::chrono::nanoseconds> (timeEnd - timeStart).count();
    auto outerTimeMs = std::chrono::duration_cast<std::chrono::milliseconds> (timeEnd - timeStart).count();

    // nd4j_printf("Non-cached reduction time avg: %lld ns; Total time: %lld ms;\n", outerTime / 100000, outerTimeMs);

    for (auto v: pool1)
        delete v;

    for (auto v: pool2)
        delete v;
}


TEST_F(PlaygroundTests, ScalarTest_1) {
    std::vector<NDArray*> pool1(poolSize);
    std::vector<NDArray*> pool2(poolSize);
    auto source = NDArrayFactory::create<float>('c', {1, 100});
    for (int e = 0; e < pool1.size(); e++) {
        pool1[e] = source.dup();
        pool2[e] = source.dup();
    }


    auto timeStart = std::chrono::system_clock::now();
    int cnt = 0;
    float *buff = reinterpret_cast<float*>(source.buffer());
    for (int e = 0; e < 100000; e++) {
        //auto v = pool1[poolSize - 1 - cnt];
        //v->template applyScalar<simdOps::Add<float>>(2.0f);
        source.applyScalar(scalar::Add,2.0f);
        //functions::scalar::ScalarTransform<float>::template transformEx<simdOps::Add<float>>(source.buffer(), 1, source.buffer(), 1, 2.0f, nullptr, source.lengthOf());
        //functions::scalar::ScalarTransform<float>::template transform<simdOps::Add<float>>(buff, 1, buff, 1, 2.0f, nullptr, 100);

        cnt++;
        if (cnt == poolSize)
            cnt = 0;
    }
    auto timeEnd = std::chrono::system_clock::now();

    auto outerTime = std::chrono::duration_cast<std::chrono::nanoseconds> (timeEnd - timeStart).count();
    auto outerTimeMs = std::chrono::duration_cast<std::chrono::milliseconds> (timeEnd - timeStart).count();

    // nd4j_printf("Cached scalar time avg: %lld ns; Total time: %lld ms;\n", outerTime / 100000L, outerTimeMs);

    for (auto v: pool1)
        delete v;

    for (auto v: pool2)
        delete v;
}


TEST_F(PlaygroundTests, ScalarTest_2) {
    std::vector<NDArray*> pool1(poolSize);
    std::vector<NDArray*> pool2(poolSize);
    auto source = NDArrayFactory::create<float>('c', {1, 100});
    for (int e = 0; e < pool1.size(); e++) {
        pool1[e] = source.dup();
        pool2[e] = source.dup();
    }


    auto timeStart = std::chrono::system_clock::now();
    int cnt = 0;
    float * array = reinterpret_cast<float*>(source.buffer());
    for (int e = 0; e < 100000; e++) {

#pragma omp simd
        for (int i = 0; i < source.lengthOf(); i++) {
            array[i] = simdOps::Add<float, float, float>::op(array[i], 2.0f);
        }

        cnt++;
        if (cnt == poolSize)
            cnt = 0;
    }
    auto timeEnd = std::chrono::system_clock::now();

    auto outerTime = std::chrono::duration_cast<std::chrono::nanoseconds> (timeEnd - timeStart).count();
    auto outerTimeMs = std::chrono::duration_cast<std::chrono::milliseconds> (timeEnd - timeStart).count();

    // nd4j_printf("Cached manual scalar time avg: %lld ns; Total time: %lld ms;\n", outerTime / 100000, outerTimeMs);

    for (auto v: pool1)
        delete v;

    for (auto v: pool2)
        delete v;
}

TEST_F(PlaygroundTests, Test_Profile_1) {
    GraphProfile prof;

    prof.setBuildTime(70);
    prof.setExecutionTime(130);

    prof.startEvent("omega");
    prof.spotEvent("alpha");
    prof.spotEvent("beta");
    prof.spotEvent("gamma");
    prof.recordEvent("omega");

    auto nodeA = prof.nodeById(1, "MatMul");
    auto nodeB = prof.nodeById(2, "Sum");
    auto nodeC = prof.nodeById(3, "Conv2D");

    nodeA->setObjectsSize(512);
    nodeA->setTemporarySize(65536);
    nodeA->setActivationsSize(512387);
    nodeA->setPreparationTime(127);
    nodeA->setExecutionTime(6539);


    nodeB->setObjectsSize(0);
    nodeB->setTemporarySize(0);
    nodeB->setActivationsSize(512387);
    nodeB->setPreparationTime(132);
    nodeB->setExecutionTime(2047);


    nodeC->setObjectsSize(1536);
    nodeC->setTemporarySize(2355674);
    nodeC->setActivationsSize(1022092);
    nodeC->setPreparationTime(129);
    nodeC->setExecutionTime(12983);

    // prof.printOut();
}

#ifdef GRAPH_FILES_OK
TEST_F(PlaygroundTests, Test_Profile_2) {
    Environment::getInstance()->setProfiling(true);
    auto graph = GraphExecutioner::importFromFlatBuffers("./resources/ae_00.fb");

    auto profile = GraphProfilingHelper::profile(graph, 2);
    // profile->printOut();

    delete graph;
    delete profile;
}
#endif

TEST_F(PlaygroundTests, Test_Im2Col_1) {
    
    int bS=16, iH=224,iW=224,  iC=3,oC=3,  kH=11,kW=11,  sH=4,sW=4,  pH=2,pW=2,  dH=1,dW=1;    
    int        oH=55, oW=55;
    int iterations = 1;

    auto input = NDArrayFactory::create<float>('c', {bS, iC, iH, iW});
    auto output = NDArrayFactory::create<float>('c', {bS, iC, kH, kW, oH, oW});

    auto outputPermuted = NDArrayFactory::create<float>('c', {bS, oH, oW, iC, kH, kW});
    outputPermuted.permutei({0, 3, 4, 5, 1, 2});

    nd4j::ops::im2col op;

    auto timeStart = std::chrono::system_clock::now();

    for (int e = 0; e < iterations; e++) {
        auto result = op.execute({&input}, {&output}, {}, {kH, kW, sH, sW, pH, pW, dH, dW, 0}, {});
        ASSERT_EQ(Status::OK(), result);
    }

    auto timeEnd = std::chrono::system_clock::now();
    auto outerTime = std::chrono::duration_cast<std::chrono::microseconds> (timeEnd - timeStart).count();

    // outputPermuted.printShapeInfo("permuted shape");

    auto permStart = std::chrono::system_clock::now();

    for (int e = 0; e < iterations; e++) {
        auto result = op.execute({&input}, {&outputPermuted}, {}, {kH, kW, sH, sW, pH, pW, dH, dW, 0}, {});
        ASSERT_EQ(Status::OK(), result);
    }

    auto permEnd = std::chrono::system_clock::now();
    auto permTime = std::chrono::duration_cast<std::chrono::microseconds> (permEnd - permStart).count();


    auto legacyStart = std::chrono::system_clock::now();

    float extra[] = {(float)kH, (float)kW, (float)sH, (float)sW, (float)pH, (float)pW, (float)dH, (float)dW, 0.f, 0.f};
    for (int e = 0; e < iterations; e++) {
        input.applyTransform(transform::Im2col, &output, extra);
    }

    auto legacyEnd = std::chrono::system_clock::now();
    auto legacyTime = std::chrono::duration_cast<std::chrono::microseconds> (legacyEnd - legacyStart).count();


    auto legacyPermStart = std::chrono::system_clock::now();

    for (int e = 0; e < iterations; e++) {
        input.applyTransform(transform::Im2col, &outputPermuted, extra);
    }

    auto legacyPermEnd = std::chrono::system_clock::now();
    auto legacyPermTime = std::chrono::duration_cast<std::chrono::microseconds> (legacyPermEnd - legacyPermStart).count();


    NativeOps nativeOps;

    Nd4jLong iArgs[] = {kH, kW, sH, sW, pH, pW, dH, dW, 0};
    Nd4jPointer inputBuffers[] = {input.buffer()};
    Nd4jPointer inputShapes[] = {input.shapeInfo()};

    Nd4jPointer outputBuffers[] = {output.buffer()};
    Nd4jPointer outputShapes[] = {output.shapeInfo()};

    auto javaStart = std::chrono::system_clock::now();

    for (int e = 0; e < iterations; e++) {
        nativeOps.execCustomOp(nullptr, op.getOpHash(), inputBuffers, inputShapes, 1, outputBuffers, outputShapes, 1, nullptr, 0, iArgs, 9, nullptr, 0, false);
    }

    auto javaEnd = std::chrono::system_clock::now();
    auto javaTime = std::chrono::duration_cast<std::chrono::microseconds> (javaEnd - javaStart).count();


    Nd4jPointer outputPermBuffers[] = {outputPermuted.buffer()};
    Nd4jPointer outputPermShapes[] = {outputPermuted.shapeInfo()};

    auto javaPermStart = std::chrono::system_clock::now();


    for (int e = 0; e < iterations; e++) {
        nativeOps.execCustomOp(nullptr, op.getOpHash(), inputBuffers, inputShapes, 1, outputPermBuffers, outputPermShapes, 1, nullptr, 0, iArgs, 9, nullptr, 0, false);
    }

    auto javaPermEnd = std::chrono::system_clock::now();
    auto javaPermTime = std::chrono::duration_cast<std::chrono::microseconds> (javaPermEnd - javaPermStart).count();

    // nd4j_printf("New time: %lld us;\n", outerTime / iterations);
    // nd4j_printf("Permuted time: %lld us;\n", permTime / iterations);
    // nd4j_printf("Legacy time: %lld us;\n", legacyTime / iterations);
    // nd4j_printf("Legacy Permuted time: %lld us;\n", legacyPermTime / iterations);
    // nd4j_printf("Java time: %lld us;\n", javaTime / iterations);
    // nd4j_printf("Java Permuted time: %lld us;\n", javaPermTime / iterations);
}

TEST_F(PlaygroundTests, Test_Im2Col_2) {
    auto input = NDArrayFactory::create<float>('c', {16, 3, 224, 224});
    auto output = NDArrayFactory::create<float>('c', {16, 3, 11, 11, 55, 55});

    auto outputPermuted = NDArrayFactory::create<float>('c', {16, 55, 55, 3, 11, 11});
    outputPermuted.permutei({0, 3, 4, 5, 1, 2});

    nd4j::ops::im2col op;

    Nd4jLong iArgs[] = {11, 11, 4, 4, 2, 2, 1, 1, 0};
    Nd4jPointer inputBuffers[] = {input.buffer()};
    Nd4jPointer inputShapes[] = {input.shapeInfo()};

    Nd4jPointer outputPermBuffers[] = {outputPermuted.buffer()};
    Nd4jPointer outputPermShapes[] = {outputPermuted.shapeInfo()};

    NativeOps nativeOps;

    nativeOps.execCustomOp(nullptr, op.getOpHash(), inputBuffers, inputShapes, 1, outputPermBuffers, outputPermShapes, 1, nullptr, 0, iArgs, 9, nullptr, 0, false);
}

TEST_F(PlaygroundTests, Test_Col2Im_1) {
    
    int bS=16, iH=224,iW=224,  iC=3,oC=3,  kH=11,kW=11,  sH=4,sW=4,  pH=2,pW=2,  dH=1,dW=1;    
    int        oH=55, oW=55;
    int iterations = 1;

    auto input = NDArrayFactory::create<float>('c', {bS, iC, kH, kW, oH, oW});
    auto output = NDArrayFactory::create<float>('c', {bS, iC, iH, iW});
    
    auto inputPermuted = NDArrayFactory::create<float>('c', {bS, oH, oW, iC, kH, kW});
    inputPermuted.permutei({0, 3, 4, 5, 1, 2});
    auto outputPermuted = NDArrayFactory::create<float>('c', {bS, iH, iW, iC});
    outputPermuted.permutei({0, 3, 1, 2});

    input = 10.;
    output = 2.;

    inputPermuted = 10.;
    outputPermuted = 2.;

    nd4j::ops::col2im op;

    auto timeStart = std::chrono::system_clock::now();

    for (int e = 0; e < iterations; e++) {
        auto result = op.execute({&input}, {&output}, {}, {sH, sW, pH, pW, iH, iW, dH, dW, 0}, {});
        ASSERT_EQ(Status::OK(), result);
    }

    auto timeEnd = std::chrono::system_clock::now();
    auto outerTime = std::chrono::duration_cast<std::chrono::microseconds> (timeEnd - timeStart).count();

    auto permStart = std::chrono::system_clock::now();

    for (int e = 0; e < iterations; e++) {
        auto result = op.execute({&inputPermuted}, {&outputPermuted}, {}, {sH, sW, pH, pW, iH, iW, dH, dW, 0}, {});
        ASSERT_EQ(Status::OK(), result);
    }

    auto permEnd = std::chrono::system_clock::now();
    auto permTime = std::chrono::duration_cast<std::chrono::microseconds> (permEnd - permStart).count();

    // nd4j_printf("C-order  time: %lld us;\n", outerTime / iterations);
    // nd4j_printf("Permuted time: %lld us;\n", permTime / iterations);    
}

TEST_F(PlaygroundTests, Test_Im2Col_3) {
    
    int bS=16, iH=224,iW=224,  iC=3,oC=3,  kH=11,kW=11,  sH=4,sW=4,  pH=2,pW=2,  dH=1,dW=1;    
    int        oH=55, oW=55;
    int iterations = 1;

    auto output = NDArrayFactory::create<float>('c', {bS, iC, kH, kW, oH, oW});
    auto input = NDArrayFactory::create<float>('c', {bS, iC, iH, iW});
    
    auto outputPermuted = NDArrayFactory::create<float>('c', {bS, oH, oW, iC, kH, kW});
    outputPermuted.permutei({0, 3, 4, 5, 1, 2});
    auto inputPermuted = NDArrayFactory::create<float>('c', {bS, iH, iW, iC});
    inputPermuted.permutei({0, 3, 1, 2});

    input = 10.;
    output = 2.;

    inputPermuted = 10.;
    outputPermuted = 2.;

    nd4j::ops::im2col op;

    auto timeStart = std::chrono::system_clock::now();

    for (int e = 0; e < iterations; e++) {
        auto result = op.execute({&input}, {&output}, {}, {kH, kW, sH, sW, pH, pW, dH, dW, 0}, {});
        ASSERT_EQ(Status::OK(), result);
    }

    auto timeEnd = std::chrono::system_clock::now();
    auto outerTime = std::chrono::duration_cast<std::chrono::microseconds> (timeEnd - timeStart).count();

    auto permStart = std::chrono::system_clock::now();

    for (int e = 0; e < iterations; e++) {
        auto result = op.execute({&inputPermuted}, {&outputPermuted}, {}, {kH, kW, sH, sW, pH, pW, dH, dW, 0}, {});
        ASSERT_EQ(Status::OK(), result);
    }

    auto permEnd = std::chrono::system_clock::now();
    auto permTime = std::chrono::duration_cast<std::chrono::microseconds> (permEnd - permStart).count();

    // nd4j_printf("C-order  time: %lld us;\n", outerTime / iterations);
    // nd4j_printf("Permuted time: %lld us;\n", permTime / iterations);    
}


TEST_F(PlaygroundTests, loop_test_1) {

    auto f = NDArrayFactory::create<float>('c', {2}, {5000, 10000});
    nd4j::ops::randomuniform op;

    auto result = op.execute({&f}, {-1.0f, 1.0f}, {});
    ASSERT_EQ(Status::OK(), result->status());

    auto array = result->at(0);

    auto buffer = array->buffer();
    int cnt = 0;
    int iterations = 1;

    //nd4j_printf("Array length: %lld\n", array->lengthOf());

    int length = (int) array->lengthOf();
    int span = (int) (array->lengthOf() / 6) + 8;

    NativeOps ops;

    auto t = new int[1000000];




    FloatBits fb;
    float threshold = 0.99f;
    fb.f_ = threshold;
    int le = ops.estimateThreshold(nullptr, reinterpret_cast<void *>(array->buffer()), array->shapeInfo(), static_cast<int>(array->lengthOf()), threshold);

    t[0] = le;
    t[1] = length;
    t[2] = fb.i_;

    //nd4j_printf("number of elements: [%i]\n", le);

    long permTime = 0;

    for (int x = 0; x < iterations; x++) {
        auto permStart = std::chrono::system_clock::now();
        ops.estimateThreshold(nullptr, reinterpret_cast<void *>(array->buffer()), array->shapeInfo(), static_cast<int>(array->lengthOf()), threshold);
        TypeCast::convertToThreshold<float>(nullptr, buffer, array->lengthOf(), t);

        /*
#pragma omp parallel reduction(+:cnt)
        {
            int tid = omp_get_thread_num();
            int start = span * tid;
            int stop = span * (tid + 1);
            if (stop > length)
                stop = length;

#pragma omp simd
            for (int e = start; e < stop; e++) {
                auto v = fabsf(buffer[e]);
                if (v >= 0.995f)
                    cnt++;
            }
        }
         */
/*
#pragma omp parallel for simd reduction(+:cnt)
        for (int e = 0; e < length; e++) {
            auto v = fabsf(buffer[e]);
            if (v >= 0.995f)
                cnt++;
        }
        */

        auto permEnd = std::chrono::system_clock::now();
        permTime += std::chrono::duration_cast<std::chrono::microseconds> (permEnd - permStart).count();
    }



    nd4j_printf("Permuted time: %lld us; Counter: %i;\n", permTime / iterations, cnt);

    delete result;
    delete[] t;
}

//////////////////////////////////////////////////////////////////////
TEST_F(PlaygroundTests, ndarray_tile_test1) {

    auto x = NDArrayFactory::create<float>('c', {20, 30});
    auto exp = NDArrayFactory::create<float>('c', {2,40,60});

    auto timeStart = std::chrono::system_clock::now();
    auto tiled = x.tile({2,2,2});
    auto timeEnd = std::chrono::system_clock::now();
    auto time = std::chrono::duration_cast<std::chrono::microseconds> (timeEnd - timeStart).count();
    // nd4j_printf("c-order time: %d;\n", time);
    
    ASSERT_TRUE(tiled.isSameShape(&exp)); 
}


//////////////////////////////////////////////////////////////////////
TEST_F(PlaygroundTests, ndarray_tile_test2) {

    auto x = NDArrayFactory::create<float>('f', {20, 30});
    auto exp = NDArrayFactory::create<float>('f', {2,40,60});

    auto timeStart = std::chrono::system_clock::now();
    auto tiled = x.tile({2,2,2});
    auto timeEnd = std::chrono::system_clock::now();
    auto time = std::chrono::duration_cast<std::chrono::microseconds> (timeEnd - timeStart).count();
    // nd4j_printf("f-order time: %d;\n", time);
    
    ASSERT_TRUE(tiled.isSameShape(&exp)); 
}

//////////////////////////////////////////////////////////////////////
TEST_F(PlaygroundTests, loopThroughArrs_test1) {
    
    NDArray x('c', {20, 30, 40}, nd4j::DataType::DOUBLE);
    NDArray y('f', {50, 30, 4, 4}, nd4j::DataType::DOUBLE);  

    auto xBuff = x.bufferAsT<double>();
    auto yBuff = y.bufferAsT<double>();

    auto len = x.lengthOf();

    //***********************************
    //***********************************
  
    auto timeStart = std::chrono::system_clock::now();
#pragma omp parallel for schedule(guided) 
    for(Nd4jLong i = 0; i < len; ++i) {
                
        Nd4jLong offset1 = shape::getIndexOffset(i, x.getShapeInfo(), len);
        Nd4jLong offset2 = shape::getIndexOffset(i, y.getShapeInfo(), len);
        
        xBuff[offset1] = yBuff[offset2];
    }
    auto timeEnd = std::chrono::system_clock::now();
    auto myTime = std::chrono::duration_cast<std::chrono::microseconds> (timeEnd - timeStart).count();
    
    //***********************************
    //***********************************
    
    timeStart = std::chrono::system_clock::now();
#pragma omp parallel for schedule(guided)
    for(Nd4jLong i = 0; i < len; ++i) {
        
        Nd4jLong offset1 = shape::getIndexOffset(i, x.getShapeInfo(), len);
        Nd4jLong offset2 = shape::getIndexOffset(i, y.getShapeInfo(), len);
        xBuff[offset1] = yBuff[offset2];
    }
    timeEnd = std::chrono::system_clock::now();
    auto oldTime = std::chrono::duration_cast<std::chrono::microseconds> (timeEnd - timeStart).count();
   
    nd4j_printf("My  time: %lld us;\n", myTime);
    nd4j_printf("Old time: %lld us;\n", oldTime);

    ASSERT_TRUE(1);
}


void loopSpan(float* x, Nd4jLong* xShapeInfo, float* y, Nd4jLong* yShapeInfo, float* z, Nd4jLong* zShapeInfo) {

    auto len = shape::length(xShapeInfo);
    int xEws = shape::elementWiseStride(xShapeInfo);
    int yEws = shape::elementWiseStride(yShapeInfo);
    int zEws = shape::elementWiseStride(zShapeInfo);
            
    BlockInformation info(len, ELEMENT_THRESHOLD);
    #pragma omp parallel num_threads(info.threads) if (info.threads > 1) default(shared)
    {                
        auto i = omp_get_thread_num();            
        Nd4jLong itemsToLoop = (i < info.threads-1) ? info.items : info.items + info.remainder;
        Nd4jLong index = i * info.items;
        auto xi = x + xEws * index;
        auto yi = y + yEws * index;
        auto zi = z + zEws * index;        
        #pragma omp simd
        for (Nd4jLong j = 0; j < itemsToLoop; j++) 
            zi[j * zEws] = simdOps::LogPoissonLoss<float, float, float>::op(xi[j * xEws], yi[j * yEws]);
    }
}

void loopSimple(float* x, Nd4jLong* xShapeInfo, float* y, Nd4jLong* yShapeInfo, float* z, Nd4jLong* zShapeInfo) {

    auto len = shape::length(xShapeInfo);
    int xEws = shape::elementWiseStride(xShapeInfo);
    int yEws = shape::elementWiseStride(yShapeInfo);
    int zEws = shape::elementWiseStride(zShapeInfo);
    int threads = 6;
    int span_size = len / threads + 1;
    
    #pragma omp parallel for simd schedule(static, span_size) if (len > ELEMENT_THRESHOLD) proc_bind(close) default(shared)
    for(Nd4jLong i = 0; i < len; ++i)
        z[i * zEws] = simdOps::LogPoissonLoss<float, float, float>::op(x[i * xEws], y[i * yEws]);

}

//////////////////////////////////////////////////////////////////////
TEST_F(PlaygroundTests, loopThroughArrs_test2) {
    
    NDArray x('c', {40, 25}, nd4j::DataType::FLOAT32);

    const int iterations = 10;
    const int arrays = 10;

    std::vector<NDArray> arrs(arrays);
    for(auto& arr : arrs)
        arr = x;
    
    //***********************************    
    auto timeStart = std::chrono::system_clock::now();
    srand(119);
    for(Nd4jLong i = 0; i < iterations; ++i) {
        int xInd = rand() % arrays;
        int yInd = rand() % arrays;
        int zInd = rand() % arrays;
        auto xBuff = arrs[xInd].bufferAsT<float>();
        auto yBuff = arrs[yInd].bufferAsT<float>();
        auto zBuff = arrs[zInd].bufferAsT<float>();
        auto xShapeInfo = arrs[xInd].getShapeInfo();
        auto yShapeInfo = arrs[yInd].getShapeInfo();
        auto zShapeInfo = arrs[zInd].getShapeInfo();        
    
        loopSimple(xBuff, xShapeInfo, yBuff, yShapeInfo, zBuff, zShapeInfo);
    }
    auto timeEnd = std::chrono::system_clock::now();
    auto simpleTime = std::chrono::duration_cast<std::chrono::microseconds> ((timeEnd - timeStart)/iterations).count();

    //***********************************
    timeStart = std::chrono::system_clock::now();
    for(Nd4jLong i = 0; i < iterations; ++i) {
        int xInd = rand() % arrays;
        int yInd = rand() % arrays;
        int zInd = rand() % arrays;
        auto xBuff = arrs[xInd].bufferAsT<float>();
        auto yBuff = arrs[yInd].bufferAsT<float>();
        auto zBuff = arrs[zInd].bufferAsT<float>();
        auto xShapeInfo = arrs[xInd].getShapeInfo();
        auto yShapeInfo = arrs[yInd].getShapeInfo();
        auto zShapeInfo = arrs[zInd].getShapeInfo();        

        loopSpan(xBuff, xShapeInfo, yBuff, yShapeInfo, zBuff, zShapeInfo);    
    }
    timeEnd = std::chrono::system_clock::now();
    auto spanTime = std::chrono::duration_cast<std::chrono::microseconds> ((timeEnd - timeStart)/iterations).count();

    nd4j_printf("simple time: %lld us;\n", simpleTime);
    nd4j_printf("span   time: %lld us;\n", spanTime);

    ASSERT_TRUE(1);        
}

void loop1(float* x, Nd4jLong* xShapeInfo, float* y, Nd4jLong* yShapeInfo, float* z, Nd4jLong* zShapeInfo) {

    auto len = shape::length(xShapeInfo);
    int xEws = shape::elementWiseStride(xShapeInfo);
    int yEws = shape::elementWiseStride(yShapeInfo);
    int zEws = shape::elementWiseStride(zShapeInfo);
            
    nd4j::OmpLaunchHelper info(len);
    #pragma omp parallel num_threads(info._numThreads) default(shared)
    {                
        auto threadNum = omp_get_thread_num();
        Nd4jLong threadOffset = info.getThreadOffset(threadNum);        
        #pragma omp simd
        for (Nd4jLong j = 0; j < info.getItersPerThread(threadNum); j++)  {
            Nd4jLong xOffset = shape::getIndexOffset(j+threadOffset, xShapeInfo, len);
            Nd4jLong yOffset = shape::getIndexOffset(j+threadOffset, yShapeInfo, len);
            Nd4jLong zOffset = shape::getIndexOffset(j+threadOffset, zShapeInfo, len);
            z[xOffset] = simdOps::LogPoissonLoss<float, float, float>::op(x[xOffset], y[xOffset]);
        }
    }
}

void loop2(float* x, Nd4jLong* xShapeInfo, float* y, Nd4jLong* yShapeInfo, float* z, Nd4jLong* zShapeInfo) {

    auto len = shape::length(xShapeInfo);
    int xEws = shape::elementWiseStride(xShapeInfo);
    int yEws = shape::elementWiseStride(yShapeInfo);
    int zEws = shape::elementWiseStride(zShapeInfo);
    int threads = 6;
    int span_size = len / threads + 1;
    
    #pragma omp parallel for simd schedule(static) default(shared)
    for(Nd4jLong i = 0; i < len; ++i) {
        Nd4jLong xOffset = shape::getIndexOffset(i, xShapeInfo, len);
        Nd4jLong yOffset = shape::getIndexOffset(i, yShapeInfo, len);
        Nd4jLong zOffset = shape::getIndexOffset(i, zShapeInfo, len);
        z[xOffset] = simdOps::LogPoissonLoss<float, float, float>::op(x[xOffset], y[xOffset]);
    }

}

//////////////////////////////////////////////////////////////////////
TEST_F(PlaygroundTests, loopThroughArrs_test3) {
    
    NDArray x('c', {50, 250}, nd4j::DataType::FLOAT32);

    const int iterations = 100;
    const int arrays = 100;

    std::vector<NDArray> arrs(arrays);
    for(auto& arr : arrs)
        arr = x;
    
    //***********************************    
    auto timeStart = std::chrono::system_clock::now();
    srand(119);
    for(Nd4jLong i = 0; i < iterations; ++i) {
        int xInd = rand() % arrays;
        int yInd = rand() % arrays;
        int zInd = rand() % arrays;
        auto xBuff = arrs[xInd].bufferAsT<float>();
        auto yBuff = arrs[yInd].bufferAsT<float>();
        auto zBuff = arrs[zInd].bufferAsT<float>();
        auto xShapeInfo = arrs[xInd].getShapeInfo();
        auto yShapeInfo = arrs[yInd].getShapeInfo();
        auto zShapeInfo = arrs[zInd].getShapeInfo();        
    
        loop2(xBuff, xShapeInfo, yBuff, yShapeInfo, zBuff, zShapeInfo);
    }
    auto timeEnd = std::chrono::system_clock::now();
    auto simpleTime = std::chrono::duration_cast<std::chrono::microseconds> ((timeEnd - timeStart)/iterations).count();

    //***********************************
    timeStart = std::chrono::system_clock::now();
    for(Nd4jLong i = 0; i < iterations; ++i) {
        int xInd = rand() % arrays;
        int yInd = rand() % arrays;
        int zInd = rand() % arrays;
        auto xBuff = arrs[xInd].bufferAsT<float>();
        auto yBuff = arrs[yInd].bufferAsT<float>();
        auto zBuff = arrs[zInd].bufferAsT<float>();
        auto xShapeInfo = arrs[xInd].getShapeInfo();
        auto yShapeInfo = arrs[yInd].getShapeInfo();
        auto zShapeInfo = arrs[zInd].getShapeInfo();        

        loop1(xBuff, xShapeInfo, yBuff, yShapeInfo, zBuff, zShapeInfo);    
    }
    timeEnd = std::chrono::system_clock::now();
    auto spanTime = std::chrono::duration_cast<std::chrono::microseconds> ((timeEnd - timeStart)/iterations).count();

    nd4j_printf("simpleTime time: %lld us;\n", simpleTime);
    nd4j_printf("spanTime   time: %lld us;\n", spanTime);

    ASSERT_TRUE(1);        
}

TEST_F(PlaygroundTests, test_batched_skipgram_1) {
    const int batchSize = 64;
    const int codeLen = 6;
    const int numWords = 244;
    const int vectorLength = 50;

    auto target = NDArrayFactory::create<int>('c', {batchSize});
    auto ngStarter = NDArrayFactory::empty<int>();
    auto indices = NDArrayFactory::create<int>('c', {batchSize, codeLen});
    auto codes = NDArrayFactory::create<int8_t>('c', {batchSize, codeLen});
    auto syn0 = NDArrayFactory::create<float>('c', {numWords, vectorLength});
    auto syn1 = NDArrayFactory::create<float>('c', {numWords, vectorLength});
    auto syn1Neg = NDArrayFactory::empty<float>();
    auto expTable = NDArrayFactory::linspace<float>(0.001, 0.995, 10000);
    auto negTable = NDArrayFactory::empty<float>();

    auto alpha = NDArrayFactory::create<double>('c', {batchSize});
    auto randomValue = NDArrayFactory::create<Nd4jLong>('c', {batchSize});
    auto inferenceVector = NDArrayFactory::empty<float>();

    syn0.assign(0.01);
    syn1.assign(0.02);

    Nd4jLong rv = 2843242345121L;
    auto lr = 0.025;
    for (int e = 0; e < batchSize; e++) {
        target.p(e, e);
        alpha.p(e, lr);
        randomValue.p(e, rv);

        lr -= 0.001;


        for (int s = 1; s < codeLen; s++) {
            indices.p(e, s, nd4j::math::nd4j_abs<Nd4jLong>(rv % numWords));
            codes.p(e, s, s % 2);

            rv = nd4j::math::nd4j_abs<Nd4jLong>(rv * 25214903917L + 11);
        }

        rv = nd4j::math::nd4j_abs<Nd4jLong>(rv * 25214903917L + 11);
    }

    //indices.printIndexedBuffer("indices");
    //codes.printIndexedBuffer("codes");

    auto iterations = 10;

    nd4j::ops::skipgram op;

    auto timeStart = std::chrono::system_clock::now();
    for (int e = 0; e < iterations; e++) {
        auto result = op.execute({&target, &ngStarter, &indices, &codes, &syn0, &syn1, &syn1Neg, expTable, &negTable, &alpha, &randomValue, &inferenceVector}, {}, {}, {false}, true);
        ASSERT_EQ(Status::OK(), result->status());
        delete result;
    }
    auto timeEnd = std::chrono::system_clock::now();
    auto spanTime = std::chrono::duration_cast<std::chrono::microseconds> ((timeEnd - timeStart)/iterations).count();
    auto ttlTime = std::chrono::duration_cast<std::chrono::milliseconds> ((timeEnd - timeStart)).count();


    nd4j_printf("average time: %lld us;\n", spanTime);
    nd4j_printf("total time: %lld ms;\n", ttlTime);


    delete expTable;
}

TEST_F(PlaygroundTests, test_reduce_float) {
     auto array = NDArrayFactory::create<float>('c', {32, 128, 256, 256});
     auto target = NDArrayFactory::create<float>('c', {32, 128});
     std::array<int,2> dims = {2, 3};
     shape::TAD xTad(array.shapeInfo(), dims.data(), dims.size());
     xTad.createTadOnlyShapeInfo();
     xTad.createOffsets();

//     // warm up
     for (int e = 0; e < 5; e++) {
         NativeOpExcutioner::execReduceFloat(reduce::ReduceFloatBenchmarkOp, array.buffer(), array.shapeInfo(), nullptr, target.buffer(),target.shapeInfo(), dims.data(), dims.size(), xTad.tadOnlyShapeInfo,xTad.tadOffsets);
     }

     int iterations = 10;
     auto timeStart = std::chrono::system_clock::now();
     for (int e = 0; e < iterations; e++) {
         NativeOpExcutioner::execReduceFloat(reduce::ReduceFloatBenchmarkOp, array.buffer(), array.shapeInfo(), nullptr, target.buffer(), target.shapeInfo(), dims.data(), dims.size(), xTad.tadOnlyShapeInfo, xTad.tadOffsets);
     }
     auto timeEnd = std::chrono::system_clock::now();
     auto spanTime = std::chrono::duration_cast<std::chrono::microseconds> ((timeEnd - timeStart)/iterations).count();
     auto ttlTime = std::chrono::duration_cast<std::chrono::milliseconds> ((timeEnd - timeStart)).count();

     nd4j_printf("average time: %lld us;\n", spanTime);
     nd4j_printf("total time: %lld ms;\n", ttlTime);
}

TEST_F(PlaygroundTests, test_reduce_same) {
    auto array = NDArrayFactory::create<float>('c', {32, 128, 256, 256});
    auto target = NDArrayFactory::create<float>('c', {32, 128});
    std::array<int,2> dims = {2, 3};
    shape::TAD xTad(array.shapeInfo(), dims.data(), dims.size());
    xTad.createTadOnlyShapeInfo();
    xTad.createOffsets();

//     // warm up
    for (int e = 0; e < 5; e++) {
        NativeOpExcutioner::execReduceSame(reduce::ReduceSameBenchmarkOp, array.buffer(), array.shapeInfo(), nullptr, target.buffer(),target.shapeInfo(), dims.data(), dims.size(), xTad.tadOnlyShapeInfo,xTad.tadOffsets);
    }

    int iterations = 10;
    auto timeStart = std::chrono::system_clock::now();
    for (int e = 0; e < iterations; e++) {
        NativeOpExcutioner::execReduceSame(reduce::ReduceSameBenchmarkOp, array.buffer(), array.shapeInfo(), nullptr, target.buffer(), target.shapeInfo(), dims.data(), dims.size(), xTad.tadOnlyShapeInfo, xTad.tadOffsets);
    }
    auto timeEnd = std::chrono::system_clock::now();
    auto spanTime = std::chrono::duration_cast<std::chrono::microseconds> ((timeEnd - timeStart)/iterations).count();
    auto ttlTime = std::chrono::duration_cast<std::chrono::milliseconds> ((timeEnd - timeStart)).count();

    nd4j_printf("average time: %lld us;\n", spanTime);
    nd4j_printf("total time: %lld ms;\n", ttlTime);
}

TEST_F(PlaygroundTests, test_reduce_float_manual) {
    auto array = NDArrayFactory::create<float>('c', {32, 128, 256, 256});
    auto target = NDArrayFactory::create<float>('c', {32, 128});
    std::array<int, 2> dims = {2, 3};
    shape::TAD xTad(array.shapeInfo(), dims.data(), dims.size());
    xTad.createTadOnlyShapeInfo();
    xTad.createOffsets();

//     // warm up
    for (int e = 0; e < 5; e++) {
        NativeOpExcutioner::execReduceFloat(reduce::ReduceFloatBenchmarkOp, array.buffer(), array.shapeInfo(), nullptr,
                                           target.buffer(), target.shapeInfo(), dims.data(), dims.size(),
                                           xTad.tadOnlyShapeInfo, xTad.tadOffsets);
    }

    int tadLength = 256 * 256;

    int iterations = 10;
    auto timeStart = std::chrono::system_clock::now();
    for (int i = 0; i < iterations; i++) {
        auto z = target.bufferAsT<float>();

#pragma omp parallel for schedule(static) num_threads(6)
        for (int t = 0; t < target.lengthOf(); t++) {

            auto x = array.bufferAsT<float>() + (tadLength * t);
            float local = 0.0f;

            #pragma omp simd reduction(+:local)
            for (int e = 0; e < tadLength; e++) {
                local += x[e];//powf(x[e], 3) * logf(x[e]);
            }

            z[t] = local / tadLength;
        }
    }
    auto timeEnd = std::chrono::system_clock::now();
    auto spanTime = std::chrono::duration_cast<std::chrono::microseconds> ((timeEnd - timeStart)/iterations).count();
    auto ttlTime = std::chrono::duration_cast<std::chrono::milliseconds> ((timeEnd - timeStart)).count();

    nd4j_printf("average time: %lld us;\n", spanTime);
    nd4j_printf("total time: %lld ms;\n", ttlTime);
}

// TEST_F(PlaygroundTests, test_reduce_scalar_float_1) {
//     auto array = NDArrayFactory::create<float>('c', {32, 128, 256, 256});
//     auto target = NDArrayFactory::create<float>(0.0f);

//     // warm up
//     for (int e = 0; e < 5; e++) {
//         NativeOpExcutioner::execReduceFloatScalar(reduce::Mean, array.buffer(), array.shapeInfo(), nullptr, target.buffer(), target.shapeInfo());
//     }

//     int iterations = 10;
//     auto timeStart = std::chrono::system_clock::now();
//     for (int e = 0; e < iterations; e++) {
//         NativeOpExcutioner::execReduceFloatScalar(reduce::Mean, array.buffer(), array.shapeInfo(), nullptr, target.buffer(), target.shapeInfo());
//     }
//     auto timeEnd = std::chrono::system_clock::now();
//     auto spanTime = std::chrono::duration_cast<std::chrono::microseconds> ((timeEnd - timeStart)/iterations).count();
//     auto ttlTime = std::chrono::duration_cast<std::chrono::milliseconds> ((timeEnd - timeStart)).count();

//     nd4j_printf("average time: %lld us;\n", spanTime);
//     nd4j_printf("total time: %lld ms;\n", ttlTime);
// }

TEST_F(PlaygroundTests, test_reduce_scalar_float_2) {
    auto array = NDArrayFactory::create<float>('c', {100000});
    auto target = NDArrayFactory::create<float>(0.0f);

     // warm up
     for (int e = 0; e < 100; e++) {
         NativeOpExcutioner::execReduceFloatScalar(reduce::ReduceFloatBenchmarkOp, array.buffer(), array.shapeInfo(), nullptr, target.buffer(), target.shapeInfo());
     }

     int iterations = 100;
     auto timeStart = std::chrono::system_clock::now();
     for (int e = 0; e < iterations; e++) {
         NativeOpExcutioner::execReduceFloatScalar(reduce::ReduceFloatBenchmarkOp, array.buffer(), array.shapeInfo(), nullptr, target.buffer(), target.shapeInfo());
     }
     auto timeEnd = std::chrono::system_clock::now();
     auto spanTime = std::chrono::duration_cast<std::chrono::nanoseconds> ((timeEnd - timeStart)/iterations).count();
     auto ttlTime = std::chrono::duration_cast<std::chrono::milliseconds> ((timeEnd - timeStart)).count();

     nd4j_printf("average time: %lld ns;\n", spanTime);
     nd4j_printf("total time: %lld ms;\n", ttlTime);
}

TEST_F(PlaygroundTests, test_reduce_scalar_same_2) {
    auto array = NDArrayFactory::create<float>('c', {100000});
    auto target = NDArrayFactory::create<float>(0.0f);

    // warm up
    for (int e = 0; e < 100; e++) {
        NativeOpExcutioner::execReduceSameScalar(reduce::ReduceSameBenchmarkOp, array.buffer(), array.shapeInfo(), nullptr, target.buffer(), target.shapeInfo());
    }

    int iterations = 100;
    auto timeStart = std::chrono::system_clock::now();
    for (int e = 0; e < iterations; e++) {
        NativeOpExcutioner::execReduceSameScalar(reduce::ReduceSameBenchmarkOp, array.buffer(), array.shapeInfo(), nullptr, target.buffer(), target.shapeInfo());
    }
    auto timeEnd = std::chrono::system_clock::now();
    auto spanTime = std::chrono::duration_cast<std::chrono::nanoseconds> ((timeEnd - timeStart)/iterations).count();
    auto ttlTime = std::chrono::duration_cast<std::chrono::milliseconds> ((timeEnd - timeStart)).count();

    nd4j_printf("average time: %lld ns;\n", spanTime);
    nd4j_printf("total time: %lld ms;\n", ttlTime);
}


// TEST_F(PlaygroundTests, test_assign_float) {
//     auto array = NDArrayFactory::create<float>('c', {32, 128, 256, 256});
//     auto target = NDArrayFactory::create<float>('c', {32, 128, 256, 256});

//     array.assign(119);

//     // warm up
//     for (int e = 0; e < 5; e++) {
//         NativeOpExcutioner::execTransformAny(transform::Assign, array.buffer(), array.shapeInfo(), target.buffer(), target.shapeInfo(), nullptr, nullptr, nullptr);
//     }

//     int iterations = 10;
//     auto timeStart = std::chrono::system_clock::now();
//     for (int e = 0; e < iterations; e++) {
//         NativeOpExcutioner::execTransformAny(transform::Assign, array.buffer(), array.shapeInfo(), target.buffer(), target.shapeInfo(), nullptr, nullptr, nullptr);
//     }
//     auto timeEnd = std::chrono::system_clock::now();
//     auto spanTime = std::chrono::duration_cast<std::chrono::microseconds> ((timeEnd - timeStart)/iterations).count();
//     auto ttlTime = std::chrono::duration_cast<std::chrono::milliseconds> ((timeEnd - timeStart)).count();

//     nd4j_printf("average time: %lld us;\n", spanTime);
//     nd4j_printf("total time: %lld ms;\n", ttlTime);
// }

// //////////////////////////////////////////////////////////////////////    
// INLINEDEF unsigned offsetUns(unsigned index, const unsigned *shapeInfo, unsigned arrLen) {
        
//     unsigned offset = 0;        

//     for(unsigned i = 1; i <= shapeInfo[0]; ++i) {
//         arrLen /= shapeInfo[i];
//         if(arrLen > 0 && shapeInfo[i] > 1) {                
//             offset += (index / arrLen) * shapeInfo[i + shapeInfo[0]];
//             index %= arrLen;
//         }
//     }
//     return offset;
// }

// //////////////////////////////////////////////////////////////////////    
// INLINEDEF int offsetInt(int index, const int *shapeInfo, int arrLen) {
        
//     int offset = 0;        

//     for(int i = 1; i <= shapeInfo[0]; ++i) {
//         arrLen /= shapeInfo[i];
//         if(arrLen > 0 && shapeInfo[i] > 1) {                
//             offset += (index / arrLen) * shapeInfo[i + shapeInfo[0]];
//             index %= arrLen;
//         }
//     }
//     return offset;
// }


// //////////////////////////////////////////////////////////////////////
// TEST_F(PlaygroundTests, signed_unsigned_1) {
    
//     const int dim0(32), dim1(128), dim2(256), dim3(256);
//     const unsigned len = dim0*dim1*dim2*dim3;
    
//     unsigned shapeInfoUns[] = {4,  dim0,dim1,dim2,dim3,  dim1*dim2*dim3,dim2*dim3,dim3,1,   0, 1, 99};
//     int      shapeInfoInt[] = {4,  dim0,dim1,dim2,dim3,  dim1*dim2*dim3,dim2*dim3,dim3,1,   0, 1, 99};

//     double* buffer = new double[len];

//     //***********************************    
//     auto timeStart = std::chrono::system_clock::now();
//     for(int i=0; i<(int)len; ++i) {
//         int offset = offsetInt(i, shapeInfoInt, (int)len);
//         buffer[offset] = i;
//     }    

//     auto timeEnd = std::chrono::system_clock::now();
//     auto timeInt = std::chrono::duration_cast<std::chrono::nanoseconds> ((timeEnd - timeStart)/len).count();

//     //***********************************
//     timeStart = std::chrono::system_clock::now();
    
//     for(unsigned i=0; i<len; ++i) {
//         unsigned offset = offsetUns(i, shapeInfoUns, len);
//         buffer[offset] = i;
//     }   
    
//     timeEnd = std::chrono::system_clock::now();
//     auto timeUns = std::chrono::duration_cast<std::chrono::nanoseconds> ((timeEnd - timeStart)/len).count();

//     nd4j_printf("signed   time: %i ns;\n", timeInt);
//     nd4j_printf("unsigned time: %u ns;\n", timeUns);

//     delete []buffer;
// }