/*******************************************************************************
 * Copyright (c) 2015-2018 Skymind, Inc.
 * Copyright (c) 2019 Konduit K.K.
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
// @author raver119@gmail.com, created on 29/10/17.
// @author Yurii Shyrma (iuriish@yahoo.com)
//

#include <op_boilerplate.h>
#if NOT_EXCLUDED(OP_batchnorm)

#include <ops/declarable/CustomOperations.h>
#include<ops/declarable/helpers/batchnorm.h>

namespace nd4j {
namespace ops {


//////////////////////////////////////////////////////////////////////////
CUSTOM_OP_IMPL(batchnorm, 3, 1, false, 1, 2) {

    auto input    = INPUT_VARIABLE(0);
    auto mean     = INPUT_VARIABLE(1);
    auto variance = INPUT_VARIABLE(2);
    NDArray* gamma    = nullptr;
    NDArray* beta     = nullptr;

    auto output   = OUTPUT_VARIABLE(0);

    const bool   applyScale  = (bool)INT_ARG(0);
    const bool   applyOffset = (bool)INT_ARG(1);
    const double epsilon     = T_ARG(0);

    if(applyScale)
        gamma = INPUT_VARIABLE(3);
    if(applyOffset)
        beta = INPUT_VARIABLE(3 + (int)applyScale);

    const int numOfIntArgs = block.getIArguments()->size();
    const int inRank = input->rankOf();

    // get axes args to normalize input array over
    std::vector<int> axes;
    if(numOfIntArgs > 2)
        for(int i = 2; i < numOfIntArgs; ++i)
            axes.push_back(INT_ARG(i));
    else
        axes.push_back(inRank-1);               // default dimension to reduce along is last dimension

    const int numOfAxes = axes.size();
    REQUIRE_TRUE(numOfAxes <= inRank, 0, "BATCHNORM op: too big number of input axes to normalize over, expected number should be less or equal to rank of input array, but got %i and %i correspondingly !", numOfAxes, inRank);

    // evaluate expected shape for mean, variance and gamma. These 3 arrays should have identical shapes
    // for example if input shape is {2,3,4,5,6} and axes = {1,3}, then expected shape would be {1,3,1,5,1}, and if axes = {3}, then expected shape would be {5}
    std::vector<Nd4jLong> expShape;
    if(numOfAxes == 1)
        expShape.push_back(input->sizeAt(axes[0]));
    else {      // get, for example, something like {1, inputDim1, 1, inputDim3, 1} if axes = {1, 3}
        expShape = std::vector<Nd4jLong>(inRank, 1);
        for(uint i = 0; i < numOfAxes; ++i)
            expShape[axes[i]] = input->sizeAt(axes[i]);
    }

    REQUIRE_TRUE(mean->isSameShape(expShape) , 0, "BATCHNORM op: wrong shape of mean array, expected is %s, but got %s instead !", ShapeUtils::shapeAsString(expShape).c_str(), ShapeUtils::shapeAsString(mean).c_str());
    REQUIRE_TRUE(variance->isSameShape(expShape), 0, "BATCHNORM op: wrong shape of variance array, expected is %s, but got %s instead !", ShapeUtils::shapeAsString(expShape).c_str(), ShapeUtils::shapeAsString(variance).c_str());
    if(gamma)
        REQUIRE_TRUE(gamma->isSameShape(expShape), 0, "BATCHNORM op: wrong shape of gamma array, expected is %s, but got %s instead !", ShapeUtils::shapeAsString(expShape).c_str(), ShapeUtils::shapeAsString(gamma).c_str());
    if(beta)
        REQUIRE_TRUE(beta->isSameShape(expShape), 0, "BATCHNORM op: wrong shape of beta array, expected is %s, but got %s instead !", ShapeUtils::shapeAsString(expShape).c_str(), ShapeUtils::shapeAsString(beta).c_str());

    // types of all input arrays should be the same
    for(int i = 1; i < block.width(); ++i)
        REQUIRE_TRUE(INPUT_VARIABLE(0)->dataType() == INPUT_VARIABLE(i)->dataType(), 0, "BATCHNORM op: types of all input arrays should be the same !");

    nd4j_debug("MKL-DNN is not used for batchnorm!\n", 0);

    // formula: output = gamma * ((input - mean) / sqrt(variance + epsilon)) + beta
    auto v = input->varianceAlongDimension(variance::SummaryStatsVariance, false, ShapeUtils::evalDimsToExclude(input->rankOf(), axes));
    auto m = input->reduceAlongDimension(nd4j::reduce::Mean, ShapeUtils::evalDimsToExclude(input->rankOf(), axes));

    // helpers::batchnorm(input, mean, variance, gamma, beta, output, axes, epsilon);
    helpers::batchnorm(input, m, v, gamma, beta, output, axes, epsilon);

    // NDArray stdInv = *v + epsilon;
    // stdInv.applyTransform(transform::Reciprocal);               // 1 / (variance + epsilon)
    // stdInv.applyTransform(transform::Sqrt);                     // 1 / (variance + epsilon)^0.5
    // if(applyScale)
    //     stdInv *= *gamma;

    //  // empty array with same shape as input
    // input->applyBroadcast(nd4j::broadcast::Subtract, axes, m, output);
    // output->applyBroadcast(nd4j::broadcast::Multiply, axes, &stdInv);

    // if(applyOffset)
    //     output->applyBroadcast(nd4j::broadcast::Add, axes, beta);

    delete v;
    delete m;

    return Status::OK();
}

DECLARE_TYPES(batchnorm) {
    getOpDescriptor()->setAllowedInputTypes({ALL_FLOATS})->setSameMode(true);
}

DECLARE_SHAPE_FN(batchnorm) {

    auto inShapeInfo = inputShape->at(0);
    DataType outType = DataTypeUtils::pickFloatingType(ArrayOptions::dataType(inShapeInfo));

    auto outShapeInfo = ShapeBuilders::copyShapeInfoAndType(inShapeInfo, outType, false, block.getWorkspace());    // output shape is identical to input shape

    return SHAPELIST(CONSTANT(outShapeInfo));
}

//////////////////////////////////////////////////////////////////////////
CUSTOM_OP_IMPL(batchnorm_bp, 4, 3, false, 1, 2) {

    NDArray* input    = INPUT_VARIABLE(0);
    NDArray* mean     = INPUT_VARIABLE(1);
    NDArray* variance = INPUT_VARIABLE(2);
    NDArray* gamma    = nullptr;
    NDArray* beta     = nullptr;
    NDArray* dLdO     = INPUT_VARIABLE(block.width() - 1);    // next epsilon

    NDArray* dLdI = OUTPUT_VARIABLE(0);
    NDArray* dLdM = OUTPUT_VARIABLE(1);
    NDArray* dLdV = OUTPUT_VARIABLE(2);
    NDArray* dLdG = nullptr;
    NDArray* dLdB = nullptr;

    const bool   applyScale  = (bool)INT_ARG(0);
    const bool   applyOffset = (bool)INT_ARG(1);
    const float  epsilon     = T_ARG(0);

    if(applyScale) {
        gamma = INPUT_VARIABLE(3);
        dLdG  = OUTPUT_VARIABLE(3);
    }
    if(applyOffset) {
        beta = INPUT_VARIABLE(3 + (int)applyScale);
        dLdB = OUTPUT_VARIABLE(3 + (int)applyScale);
    }

    const int numOfIntArgs = block.getIArguments()->size();
    const int inRank = input->rankOf();

    // get axes args to normalize input array over
    std::vector<int> axes;
    if(numOfIntArgs > 2)
        for(int i = 2; i < numOfIntArgs; ++i)
            axes.push_back(INT_ARG(i));
    else
        axes.push_back(inRank-1);               // default dimension to reduce along is last dimension

    const int numOfAxes = axes.size();
    REQUIRE_TRUE(numOfAxes <= inRank, 0, "BATCHNORM_BP op: too big number of input axes to normalize over, expected number should be less or equal to rank of input array, but got %i and %i correspondingly !", numOfAxes, inRank);

    // evaluate expected shape for mean, variance and gamma. These 3 arrays should have identical shapes
    // for example if input shape is {2,3,4,5,6} and axes = {1,3}, then expected shape would be {1,3,1,5,1}, and if axes = {3}, then expected shape would be {5}
    std::vector<Nd4jLong> expShape;
    if(numOfAxes == 1)
        expShape.push_back(input->sizeAt(axes[0]));
    else {      // get, for example, something like {1, inputDim1, 1, inputDim3, 1} if axes = {1, 3}
        expShape = std::vector<Nd4jLong>(inRank, 1);
        for(uint i = 0; i < numOfAxes; ++i)
            expShape[axes[i]] = input->sizeAt(axes[i]);
    }

    REQUIRE_TRUE(mean->isSameShape(expShape), 0, "BATCHNORM_BP op: wrong shape of mean array, expected is %s, but got %s instead !", ShapeUtils::shapeAsString(expShape).c_str(), ShapeUtils::shapeAsString(mean).c_str());
    REQUIRE_TRUE(variance->isSameShape(expShape), 0, "BATCHNORM_BP op: wrong shape of variance array, expected is %s, but got %s instead !", ShapeUtils::shapeAsString(expShape).c_str(), ShapeUtils::shapeAsString(variance).c_str());
    if(gamma)
        REQUIRE_TRUE(gamma->isSameShape(expShape), 0, "BATCHNORM_BP op: wrong shape of gamma array, expected is %s, but got %s instead !", ShapeUtils::shapeAsString(expShape).c_str(), ShapeUtils::shapeAsString(gamma).c_str());
    if(beta)
        REQUIRE_TRUE(beta->isSameShape(expShape), 0, "BATCHNORM_BP op: wrong shape of beta array, expected is %s, but got %s instead !", ShapeUtils::shapeAsString(expShape).c_str(), ShapeUtils::shapeAsString(beta).c_str());

    REQUIRE_TRUE(input->isSameShape(dLdO), 0, "BATCHNORM_BP op: wrong shape of output gradients array, expected is %s, but got %s instead !", ShapeUtils::shapeAsString(input).c_str(), ShapeUtils::shapeAsString(dLdO).c_str());

    // types of all input arrays should be the same (except dLdO)
    for(int i = 1; i < block.width() - 2; ++i)
        REQUIRE_TRUE(INPUT_VARIABLE(0)->dataType() == INPUT_VARIABLE(i)->dataType(), 0, "BATCHNORM_BP op: types of arrays (input, mean, variance, gamma, beta) should be the same !");

    // ***** calculations ***** //

    // formula for forward step: f = gamma * ((x - m) / (v + eps)^0.5) + beta

    // notations:
    // stdInv = 1 / (v + eps)^0.5
    // N - batch size (product of spatial dimensions)

    // derivatives:
    // dfdx = dfdx_ + dfdm*dmdx + dfdv*dvdx
    // dfdm = dfdm_ + dfdv*dvdm
    // dvdx = dvdx_ + dvdm*dmdx

    // dfdx = dfdx_ + (dfdm_ + dfdv*dvdm)*dmdx + dfdv*(dvdx_ + dvdm*dmdx) =  (dfdx_ + dfdm_*dmdx) + dfdv*(2*dvdm*dmdx + dvdx_)
    // term (dfdx_ + dfdm_*dmdx) = 0, since:
    //                                       1) dfdx_ = gamma * stdInv
    //                                       2) dfdm_ = - N * gamma * stdInv
    //                                       3) dmdx  = 1 / N
    // so:
    // dfdx = dfdv*(2*dvdm / N + dvdx_)

    // dvdx_ = 2 * (x - m) / N
    // dvdm  = -2 * (x - m)_sum / N
    // dfdv  = -0.5 * gamma * (x - m)_sum * stdInv^3

    // finally:
    // dfdx = (2/N) * dfdv * (dvdm  + (x - m))

    // dLdI = dLdO * dfdx
    // dLdV = (dLdO * dfdv)_sum
    // dLdM = (dLdO * dfdm)_sum = (dLdO * (dfdm_ + dfdv*dvdm))_sum

    // dLdG = (dLdO * (x - m))_sum * stdInv
    // dLdB = dLdO_sum

    // variance = input->varianceAlongDimension(variance::SummaryStatsVariance, false, ShapeUtils::evalDimsToExclude(input->rankOf(), axes));
    // mean = input->reduceAlongDimension(nd4j::reduce::Mean, ShapeUtils::evalDimsToExclude(input->rankOf(), axes));

    const auto excludedAxes = ShapeUtils::evalDimsToExclude(inRank, axes);
    const bool keepUnitiesInShape = inRank == mean->rankOf();

    // batch size N
    const Nd4jLong N = input->lengthOf() / shape::tadLength(input->getShapeInfo(), axes.data(), axes.size());

    // input - mean
    NDArray xMinusMean(input); // empty array with same shape as input
    input->applyBroadcast(nd4j::broadcast::Subtract, axes, mean, &xMinusMean);

    // stdInv
    NDArray stdInv = *variance + epsilon;
    stdInv.applyTransform(transform::Reciprocal);                           // 1 / (variance + epsilon)
    stdInv.applyTransform(transform::Sqrt);                                 // 1 / (variance + epsilon)^0.5
    auto stdInv3 = stdInv.transform(nd4j::transform::Cube);
    if(applyScale)
        stdInv3.applyPairwiseTransform(nd4j::pairwise::Multiply, *gamma);   // gamma / (variance + epsilon)^1.5

    // dfdv
    auto dfdv = xMinusMean.reduceAlongDims(nd4j::reduce::Sum, excludedAxes, keepUnitiesInShape);

    // dvdm
    auto dvdm = dfdv * (-2.f / N);

    // dfdv
    dfdv *= stdInv3;
    dfdv *= -0.5;

    // dLdI
    xMinusMean.applyBroadcast(nd4j::broadcast::Add, axes, &dvdm, dLdI);
    dLdI->applyBroadcast(nd4j::broadcast::Multiply, axes, &dfdv);
    *dLdI *= 2.f / N;
    *dLdI *= *dLdO;

    *dLdM = 0;      // put zeros so far
    *dLdV = 0;      // put zeros so far

    // dLdG
    if(applyScale) {
        xMinusMean *= *dLdO;
        xMinusMean.reduceAlongDimension(reduce::Sum, dLdG, excludedAxes, keepUnitiesInShape);
        *dLdG *= stdInv;
    }

    // dLdB
    if(applyOffset)
        dLdO->reduceAlongDimension(reduce::Sum, dLdB, excludedAxes, keepUnitiesInShape);


    // java code
    NDArray xMu(input);
    input->applyBroadcast(nd4j::broadcast::Subtract, axes, mean, &xMu);
    NDArray xHat(input);
    input->applyBroadcast(nd4j::broadcast::Multiply, axes, &stdInv, &xHat);
    NDArray dxhat(input);
    dLdO->applyBroadcast(nd4j::broadcast::Multiply, axes, gamma, &dxhat);
    NDArray temp = dxhat*xMu;
    temp.reduceAlongDimension(reduce::Sum, dLdV, excludedAxes, keepUnitiesInShape);
    *dLdV *= -0.5f * stdInv*stdInv*stdInv;
    dLdV->printBuffer();
    NDArray* dxmu1 = dxhat.reduceAlongDimension(reduce::Sum, excludedAxes, keepUnitiesInShape);
    *dxmu1 *= -stdInv;
    NDArray* dxmu2 = xMu.reduceAlongDimension(reduce::Sum, excludedAxes, keepUnitiesInShape);
    *dxmu2 *=  *dLdV * (-2.f/N);
    NDArray dLdmu = *dxmu1 + *dxmu2;
    dLdmu *= (1.f /N);
    *dLdV *= (2.f/N);
    dxhat.applyBroadcast(nd4j::broadcast::Multiply, axes, &stdInv);
    xMu.applyBroadcast(nd4j::broadcast::Multiply, axes, dLdV);
    dxhat += xMu;
    dxhat.applyBroadcast(nd4j::broadcast::Add, axes, &dLdmu, dLdI);
    delete  dxmu1;
    delete  dxmu2;
    xHat *= *dLdO;
    xHat.reduceAlongDimension(reduce::Sum, dLdG, excludedAxes, keepUnitiesInShape);

    // dLdI->printBuffer();
    // printf("------------\n");
    // dLdG->printBuffer();
    // printf("------------\n");
    // dLdB->printBuffer();


    return Status::OK();
}

DECLARE_TYPES(batchnorm_bp) {
    getOpDescriptor()
            ->setAllowedInputTypes(0, nd4j::DataType::ANY)
            ->setAllowedInputTypes(1, nd4j::DataType::ANY)
            ->setAllowedInputTypes(2, nd4j::DataType::ANY)
            ->setAllowedInputTypes(3, {ALL_FLOATS})
            ->setAllowedInputTypes(4, nd4j::DataType::ANY)
            ->setAllowedInputTypes(5, nd4j::DataType::ANY)
            ->setAllowedOutputTypes({ALL_FLOATS});
}

//////////////////////////////////////////////////////////////////////////

DECLARE_SHAPE_FN(batchnorm_bp) {

    Nd4jLong* inShapeInfo   = inputShape->at(0);
    Nd4jLong* meanShapeInfo = inputShape->at(1);

    const bool applyScale  = (bool)INT_ARG(0);
    const bool applyOffset = (bool)INT_ARG(1);

    DataType outType = DataTypeUtils::pickFloatingType(ArrayOptions::dataType(inShapeInfo));

    auto shapes = SHAPELIST();

    // dLdI shapeInfo
    shapes->push_back(ConstantShapeHelper::getInstance()->createShapeInfo(outType, inShapeInfo));

    // dLdM shapeInfo
    shapes->push_back(ConstantShapeHelper::getInstance()->createShapeInfo(outType, meanShapeInfo));

    // dLdV shapeInfo (same as dLdM)
    shapes->push_back(shapes->at(shapes->size()-1));

    // dLdG shapeInfo (same as dLdM)
    if(applyScale)
        shapes->push_back(shapes->at(shapes->size()-1));

    // dLdB shapeInfo (same as dLdM)
    if(applyOffset)
        shapes->push_back(shapes->at(shapes->size()-1));

    return shapes;
}


}
}

#endif
