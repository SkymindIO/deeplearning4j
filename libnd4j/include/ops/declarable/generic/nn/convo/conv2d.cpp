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
// @author Yurii Shyrma, created on 06.03.2018
//

#ifndef LIBND4J_CONVO_OPS_H
#define LIBND4J_CONVO_OPS_H

#include <system/op_boilerplate.h>
#if NOT_EXCLUDED(OP_conv2d)

#include <ops/declarable/CustomOperations.h>
#include <ops/declarable/OpRegistrator.h>
#include <ops/declarable/helpers/convolutions.h>
#include <system/op_boilerplate.h>

#include <memory>

namespace sd {
namespace ops {

CUSTOM_OP_IMPL(conv2d, 2, 1, false, 0, 9) {
  auto input = INPUT_VARIABLE(0);                               // [bS, iH, iW, iC] (NHWC) or [bS, iC, iH, iW] (NCHW)
  auto weights = INPUT_VARIABLE(1);                             // [kH, kW, iC, oC], [oC, iC, kH, kW], [oC, kH, kW, iC]
  auto bias = block.width() > 2 ? INPUT_VARIABLE(2) : nullptr;  // [oC]

  auto output = OUTPUT_NULLIFIED(0);  // [bS, oH, oW, oC] (NHWC) or [bS, oC, oH, oW] (NCHW)

  LongType sH = INT_ARG(2);                                               // strides height
  LongType sW = INT_ARG(3);                                               // strides width
  LongType pH = INT_ARG(4);                                               // paddings height
  LongType pW = INT_ARG(5);                                               // paddings width
  LongType dH = INT_ARG(6);                                               // dilations height
  LongType dW = INT_ARG(7);                                               // dilations width
  int isSameMode = INT_ARG(8);                                       // 0-VALID, 1-SAME
  int isNCHW = block.getIArguments()->size() > 9 ? INT_ARG(9) : 1;  // INT_ARG(9): 0-NCHW,  1-NHWC
  int wFormat = block.getIArguments()->size() > 10
                ? INT_ARG(10)
                : 0;  // 0 - [kH, kW, iC, oC], 1 - [oC, iC, kH, kW], 2 - [oC, kH, kW, iC]

  //normally nchw is 0 and 1 being passed in, we're using it as a boolean here
  //so we want it to be whether nchw is 0 or not.
  isNCHW = isNCHW == 0;
  LongType kH = INT_ARG(0) > 0 ? INT_ARG(0) : static_cast<LongType>(weights->sizeAt(0));  // filter(kernel) height
  LongType kW = INT_ARG(1) > 0 ? INT_ARG(1) : static_cast<LongType>(weights->sizeAt(1));  // filter(kernel) width
  ConvolutionUtils::conv2d(block, input, weights, bias, output, kH, kW, sH, sW, pH, pW, dH, dW, isSameMode, isNCHW,
                           wFormat);

  return Status::OK;
}

DECLARE_SHAPE_FN(conv2d) {
  auto inputShapeInfo = inputShape->at(0);    // [bS, iH, iW, iC] (NHWC) or [bS, iC, iH, iW] (NCHW)
  auto weightsShapeInfo = inputShape->at(1);  // [kH, kW, iC, oC], [oC, iC, kH, kW], [oC, kH, kW, iC]
  auto biasShapeInfo = block.width() > 2 ? inputShape->at(2) : nullptr;  // [oC]

  // output [bS, oH, oW, oC] (NHWC) or [bS, oC, oH, oW] (NCHW)

  LongType sH = INT_ARG(2);                                               // strides height
  LongType sW = INT_ARG(3);                                               // strides width
  LongType pH = INT_ARG(4);                                               // paddings height
  LongType pW = INT_ARG(5);                                               // paddings width
  LongType dH = INT_ARG(6);                                               // dilations height
  LongType dW = INT_ARG(7);                                               // dilations width
  int paddingMode = INT_ARG(8);                                       // 0-VALID, 1-SAME
  int isNCHW = block.getIArguments()->size() > 9 ? INT_ARG(9) : 0;  // INT_ARG(9): 0-NCHW, 1-NHWC
  LongType wFormat = block.getIArguments()->size() > 10
                     ? INT_ARG(10)
                     : 0;  // 0 - [kH, kW, iC, oC], 1 - [oC, iC, kH, kW], 2 - [oC, kH, kW, iC]
  //normally nchw is 0 and 1 being passed in, we're using it as a boolean here
  //so we want it to be whether nchw is 0 or not.
  isNCHW = isNCHW == 0;

  LongType kH = INT_ARG(0) > 0 ? INT_ARG(0) : static_cast<LongType>(ConvolutionUtils::sizeOfKh(weightsShapeInfo,wFormat));  // filter(kernel) height
  LongType kW = INT_ARG(1) > 0 ? INT_ARG(1) : static_cast<LongType>(ConvolutionUtils::sizeOfKw(weightsShapeInfo,wFormat));  // filter(kernel) width
  const int rank = 4;  // 4

  REQUIRE_TRUE(inputShapeInfo[0] == rank, 0,
               "CUSTOM CONV2D OP: rank of input array must be equal to %i, but got %i instead !", rank,
               inputShapeInfo[0]);
  REQUIRE_TRUE(weightsShapeInfo[0] == rank, 0,
               "CUSTOM CONV2D OP: rank of weights array must be equal to %i, but got %i instead !", rank,
               weightsShapeInfo[0]);



  LongType bS = shape::sizeAt(inputShapeInfo, 0);             // batch size
  LongType   iC = ConvolutionUtils::inChannels(weightsShapeInfo, wFormat);
  LongType   iH = ConvolutionUtils::inputHeight(inputShapeInfo, isNCHW);
  LongType    iW = ConvolutionUtils::inputWidth(inputShapeInfo, isNCHW);
  LongType    oC = ConvolutionUtils::outChannels(weightsShapeInfo, wFormat);
  std::vector<LongType> expectedWeightsShape = ConvolutionUtils::expectWeightsShape(wFormat, kH, kW, iC, oC);
  if(!ShapeUtils::areShapesEqual(weightsShapeInfo, expectedWeightsShape)) {
    std::string errorMessage;
    errorMessage += "CUSTOM CONV2D OP: wrong shape of weights array, expected is ";
    errorMessage += ShapeUtils::shapeAsString(expectedWeightsShape);
    errorMessage += ", but got ";
    errorMessage += ShapeUtils::shapeAsString(weightsShapeInfo);
    errorMessage += " instead !";
    THROW_EXCEPTION(errorMessage.c_str());
  }


  if (biasShapeInfo) {
    if(biasShapeInfo[0] > 2 || oC != shape::length(biasShapeInfo)) {
      std::string errorMessage;
      errorMessage += "CUSTOM CONV2D OP: wrong shape of array with biases, expected rank, length: <=2, ";
      errorMessage += std::to_string(oC);
      errorMessage += ", but got ";
      errorMessage += std::to_string(biasShapeInfo[0]);
      errorMessage += ", ";
      errorMessage += std::to_string(shape::length(biasShapeInfo));
      errorMessage += " instead !";
      THROW_EXCEPTION(errorMessage.c_str());
    }

  }

  LongType* outputShapeInfo = new LongType[shape::shapeInfoLength(rank)];

  outputShapeInfo[0] = 4;
  LongType    oH = ConvolutionUtils::calcOutDimConv(iH, kH, sH, pH, dH, paddingMode);
  LongType   oW = ConvolutionUtils::calcOutDimConv(iW,kW,sW,pW,dW,paddingMode);  // batch size, input channels, input height/width, output channels, output height/width;


  /**
   * NOTE: THIS BLOCK OF LOGIC PROBABLY LOOKS STRANGE.
   * THIS IS FOR COMPATIBILITY WITH THE CONV2D implementation in dl4j.
   */
  sd::LongType strideCalcShape[4];
  strideCalcShape[0] = oW;
  strideCalcShape[1] = oH;
  strideCalcShape[2] = bS;
  strideCalcShape[3] = oC;

  sd::LongType *permute = new sd::LongType[4];
  permute[0] = 2;
  permute[1] = 3;
  permute[2] = 1;
  permute[3] = 0;

  sd::LongType * second = shape::calcStridesFortran(strideCalcShape,shape::rank(outputShapeInfo));
  shape::doPermuteSwap(4, second,permute);
  shape::doPermuteSwap(4, strideCalcShape,permute);


  if(!isNCHW) {
    permute[0] = 0;
    permute[1] = 2;
    permute[2] = 3;
    permute[3] = 1;
    shape::doPermuteSwap(4, strideCalcShape,permute);
    shape::doPermuteSwap(4, second,permute);


    sd::LongType * second2 = shape::calcStridesFortran(strideCalcShape,shape::rank(outputShapeInfo));
    shape::doPermuteSwap(4, second2,permute);

    shape::setShape(outputShapeInfo, strideCalcShape);
    shape::setStride(outputShapeInfo,second);
    shape::setOrder(outputShapeInfo, 'f');
    ArrayOptions::setExtra(outputShapeInfo,ArrayOptions::defaultFlag());
    ArrayOptions::setDataType(outputShapeInfo,ArrayOptions::dataType(inputShapeInfo));
    delete[] second2;

  } else {
    shape::setShape(outputShapeInfo, strideCalcShape);
    shape::setStride(outputShapeInfo,second);
    shape::setOrder(outputShapeInfo, 'f');
    ArrayOptions::setExtra(outputShapeInfo,ArrayOptions::defaultFlag());
    ArrayOptions::setDataType(outputShapeInfo,ArrayOptions::dataType(inputShapeInfo));
  }



  delete[] second;
  delete[] permute;
  auto ret = ConstantShapeHelper::getInstance().createFromExisting(outputShapeInfo, block.workspace());
  return SHAPELIST(ret);
}


DECLARE_TYPES(conv2d) {
  getOpDescriptor()
      ->setAllowedInputTypes(0, ANY)
      ->setAllowedInputTypes(1, {ALL_FLOATS})
      ->setAllowedInputTypes(2, {ALL_FLOATS})
      ->setAllowedOutputTypes({ALL_FLOATS});
}

DECLARE_TYPES(conv2d_bp) {
  getOpDescriptor()->setAllowedInputTypes(ANY)->setAllowedOutputTypes({ALL_FLOATS});
}

//////////////////////////////////////////////////////////////////////////
CUSTOM_OP_IMPL(conv2d_bp, 3, 2, false, 0, 9) {
  auto input = INPUT_VARIABLE(0);                               // [bS, iH, iW, iC] (NHWC) or [bS, iC, iH, iW] (NCHW)
  auto weights = INPUT_VARIABLE(1);                             // [kH, kW, iC, oC], [oC, iC, kH, kW], [oC, kH, kW, iC]
  auto bias = block.width() > 3 ? INPUT_VARIABLE(2) : nullptr;  // [oC]
  auto gradO = block.width() > 3
               ? INPUT_VARIABLE(3)
               : INPUT_VARIABLE(2);  // [bS, oH, oW, oC] (NHWC) or [bS, oC, oH, oW] (NCHW), epsilon_next

  auto gradI = OUTPUT_VARIABLE(0);  // [bS, iH, iW, iC] (NHWC) or [bS, iC, iH, iW] (NCHW), epsilon
  auto gradW = OUTPUT_VARIABLE(1);  // [kH, kW, iC, oC], [oC, iC, kH, kW], [oC, kH, kW, iC]
  auto gradB = block.width() > 3 ? OUTPUT_VARIABLE(2) : nullptr;  // [oC]

  LongType kH = INT_ARG(0);                                               // filter(kernel) height
  LongType kW = INT_ARG(1);                                               // filter(kernel) width
  LongType sH = INT_ARG(2);                                               // strides height
  LongType sW = INT_ARG(3);                                               // strides width
  LongType pH = INT_ARG(4);                                               // paddings height
  LongType pW = INT_ARG(5);                                               // paddings width
  LongType dH = INT_ARG(6);                                               // dilations height
  LongType dW = INT_ARG(7);                                               // dilations width
  int isSameMode = INT_ARG(8);                                       // 0-VALID, 1-SAME
  int isNCHW = block.getIArguments()->size() > 9 ? INT_ARG(9) : 1;  // INT_ARG(9): 0-NCHW, 1-NHWC
  int wFormat = block.getIArguments()->size() > 10
                ? INT_ARG(10)
                : 0;  // 0 - [kH, kW, iC, oC], 1 - [oC, iC, kH, kW], 2 - [oC, kH, kW, iC]

  isNCHW = isNCHW == 0;
  REQUIRE_TRUE(input->rankOf() == 4, 0,
               "CUSTOM CONV2D_BP OP: rank of input array must be equal to 4, but got %i instead !", input->rankOf());
  REQUIRE_TRUE(weights->rankOf() == 4, 0,
               "CUSTOM CONV2D_BP OP: rank of weights array must be equal to 4, but got %i instead !",
               weights->rankOf());
  REQUIRE_TRUE(
      gradO->rankOf() == 4, 0,
      "CUSTOM CONV2D_BP OP: rank of output's gradients (next epsilon) array must be equal to 4, but got %i instead !",
      gradO->rankOf());
  int paddingMode = INT_ARG(8);                                       // 0-VALID, 1-SAME


  ConvolutionUtils::conv2dBP(block, input, weights, bias, gradO, gradI, gradW, gradB, kH, kW, sH, sW, pH, pW, dH, dW,
                             isSameMode, isNCHW, wFormat);

  return Status::OK;
}

DECLARE_SHAPE_FN(conv2d_bp) {
  auto inputShapeInfo = inputShape->at(0);    // [bS, iH, iW, iC] (NHWC) or [bS, iC, iH, iW] (NCHW)
  auto weightsShapeInfo = inputShape->at(1);  // [kH, kW, iC, oC], [oC, iC, kH, kW], [oC, kH, kW, iC]
  auto biasShapeInfo = block.width() > 3 ? inputShape->at(2) : nullptr;  // [oC]
  auto gradOShapeInfo = block.width() > 3
                        ? inputShape->at(3)
                        : inputShape->at(2);  // [bS, oH, oW, oC] (NHWC) or [bS, oC, oH, oW] (NCHW), epsilon_next


  LongType sH = INT_ARG(2);                                               // strides height
  LongType sW = INT_ARG(3);                                               // strides width
  LongType pH = INT_ARG(4);                                               // paddings height
  LongType pW = INT_ARG(5);                                               // paddings width
  LongType dH = INT_ARG(6);                                               // dilations height
  LongType dW = INT_ARG(7);                                               // dilations width
  int paddingMode = INT_ARG(8);                                       // 0-VALID, 1-SAME
  int isNCHW = block.getIArguments()->size() > 9 ? INT_ARG(9) : 0;  // INT_ARG(9): 0-NCHW, 1-NHWC
  LongType wFormat = block.getIArguments()->size() > 10
                     ? INT_ARG(10)
                     : 0;  // 0 - [kH, kW, iC, oC], 1 - [oC, iC, kH, kW], 2 - [oC, kH, kW, iC]
  //normally nchw is 0 and 1 being passed in, we're using it as a boolean here
  //so we want it to be whether nchw is 0 or not.
  isNCHW = isNCHW == 0;
  // output [bS, oH, oW, oC] (NHWC) or [bS, oC, oH, oW] (NCHW)
  LongType kH = INT_ARG(0) > 0 ? INT_ARG(0) : static_cast<LongType>(ConvolutionUtils::sizeOfKh(weightsShapeInfo,wFormat));  // filter(kernel) height
  LongType kW = INT_ARG(1) > 0 ? INT_ARG(1) : static_cast<LongType>(ConvolutionUtils::sizeOfKw(weightsShapeInfo,wFormat));  // filter(kernel) width

  const LongType rank = 4;

  LongType bS = shape::sizeAt(inputShapeInfo, 0);             // batch size
  LongType   iC = ConvolutionUtils::inChannels(weightsShapeInfo, wFormat);
  LongType   iH = ConvolutionUtils::inputHeight(inputShapeInfo, isNCHW);
  LongType    iW = ConvolutionUtils::inputWidth(inputShapeInfo, isNCHW);
  LongType    oC = ConvolutionUtils::outChannels(weightsShapeInfo, wFormat);
  std::vector<LongType> expectedWeightsShape = ConvolutionUtils::expectWeightsShape(wFormat, kH, kW, iC, oC);
  if(!ShapeUtils::areShapesEqual(weightsShapeInfo, expectedWeightsShape)) {
    std::string errorMessage;
    errorMessage += "CUSTOM CONV2D OP: wrong shape of weights array, expected is ";
    errorMessage += ShapeUtils::shapeAsString(expectedWeightsShape);
    errorMessage += ", but got ";
    errorMessage += ShapeUtils::shapeAsString(weightsShapeInfo);
    errorMessage += " instead !";
    THROW_EXCEPTION(errorMessage.c_str());
  }


  if (biasShapeInfo) {
    if(biasShapeInfo[0] > 2 || oC != shape::length(biasShapeInfo)) {
      std::string errorMessage;
      errorMessage += "CUSTOM CONV2D OP: wrong shape of array with biases, expected rank, length: <=2, ";
      errorMessage += std::to_string(oC);
      errorMessage += ", but got ";
      errorMessage += std::to_string(biasShapeInfo[0]);
      errorMessage += ", ";
      errorMessage += std::to_string(shape::length(biasShapeInfo));
      errorMessage += " instead !";
      THROW_EXCEPTION(errorMessage.c_str());
    }

  }

  sd::LongType * strideCalcShapeGradI = new sd::LongType[shape::rank(inputShapeInfo)];
  strideCalcShapeGradI[0] = iC;
  strideCalcShapeGradI[1] = bS;
  strideCalcShapeGradI[2] = iH;
  strideCalcShapeGradI[3] = iW;

  sd::LongType *strides = new sd::LongType[4];
  sd::LongType *permute = new sd::LongType[4];
  permute[0] = 1;
  permute[1] = isNCHW ? 0 : 2;
  permute[2] = isNCHW ? 2 : 3;
  permute[3] = isNCHW ? 3 : 0;
  shape::calcStrides(strideCalcShapeGradI,shape::rank(inputShapeInfo),strides);
  shape::doPermuteSwap(4, strideCalcShapeGradI, permute);
  shape::doPermuteSwap(4, strides, permute);
  auto shapeDesc = ShapeBuilders::createShapeInfo(ArrayOptions::dataType(inputShapeInfo),
                                                  'c',
                                                  4,
                                                  strideCalcShapeGradI,
                                                  block.getWorkspace(),
                                                  false);
  shape::setStride(shapeDesc,strides);
  auto gradIshapeInfo = ConstantShapeHelper::getInstance().createFromExisting(shapeDesc, true);
  RELEASE(strides,block.getWorkspace());
  RELEASE(strideCalcShapeGradI,block.getWorkspace());
  RELEASE(permute,block.getWorkspace());
  auto gradWshapeInfo =
      ShapeBuilders::copyShapeInfoAndType(weightsShapeInfo, gradOShapeInfo, false, block.getWorkspace());
  if (biasShapeInfo) {
    auto gradBshapeInfo =
        ShapeBuilders::copyShapeInfoAndType(biasShapeInfo, gradOShapeInfo, false, block.getWorkspace());
    return SHAPELIST(gradIshapeInfo, CONSTANT(gradWshapeInfo), CONSTANT(gradBshapeInfo));
  }

  return SHAPELIST(gradIshapeInfo, CONSTANT(gradWshapeInfo));
}

//////////////////////////////////////////////////////////////////////////
CUSTOM_OP_IMPL(conv2d_input_bp, 3, 1, false, 0, 9) {
  auto gradIShape = INPUT_VARIABLE(0);  // [4]
  auto weights = INPUT_VARIABLE(1);     // [kH, kW, iC, oC], [oC, iC, kH, kW], [oC, kH, kW, iC]
  auto gradO = INPUT_VARIABLE(2);       // [bS, oH, oW, oC] (NHWC) or [bS, oC, oH, oW] (NCHW), epsilon_next

  auto gradI = OUTPUT_NULLIFIED(0);  // [bS, iH, iW, iC] (NHWC) or [bS, iC, iH, iW] (NCHW), epsilon

  LongType kH = INT_ARG(0);                                               // filter(kernel) height
  LongType kW = INT_ARG(1);                                               // filter(kernel) width
  LongType sH = INT_ARG(2);                                               // strides height
  LongType sW = INT_ARG(3);                                               // strides width
  LongType pH = INT_ARG(4);                                               // paddings height
  LongType pW = INT_ARG(5);                                               // paddings width
  LongType dH = INT_ARG(6);                                               // dilations height
  LongType dW = INT_ARG(7);                                               // dilations width
  int isSameMode = INT_ARG(8);                                       // 0-VALID, 1-SAME
  int isNCHW = block.getIArguments()->size() > 9 ? !INT_ARG(9) : 1;  // INT_ARG(9): 0-NCHW, 1-NHWC
  int wFormat = block.getIArguments()->size() > 10
                ? INT_ARG(10)
                : 0;  // 0 - [kH, kW, iC, oC], 1 - [oC, iC, kH, kW], 2 - [oC, kH, kW, iC]

  const int rank = gradO->rankOf();

  REQUIRE_TRUE(weights->rankOf() == rank, 0,
               "CUSTOM CONV2D_INPUT_BP OP: rank of weights array must be equal to 4, but got %i instead !",
               weights->rankOf());
  REQUIRE_TRUE(gradIShape->rankOf() == 1, 0,
               "CUSTOM CONV2D_INPUT_BP OP: rank of array with output shape must be equal to 1, but got %i instead !",
               gradIShape->rankOf());
  REQUIRE_TRUE(gradIShape->lengthOf() == rank, 0,
               "CUSTOM CONV2D_INPUT_BP OP: length of array with output shape must be equal to 4, but got %i instead !",
               gradIShape->lengthOf());

  // create empty conv2d input array
  std::vector<LongType> gradIShapeAsVector(rank);
  for (int i = 0; i < rank; ++i) gradIShapeAsVector[i] = gradIShape->e<LongType>(i);
  NDArray input(gradO->ordering(), gradIShapeAsVector, gradO->dataType(), block.launchContext());

  LongType bS, iC, iH, iW, oC, oH,
      oW;  // batch size, input channels, input height/width, output channels, output height/width;
  LongType indIOioC, indIiH, indWoC, indWiC, indWkH, indOoH;  // corresponding indexes
  ConvolutionUtils::getSizesAndIndexesConv2d(isNCHW, wFormat, input, *gradO, bS, iC, iH, iW, oC, oH, oW, indIOioC,
                                             indIiH, indWiC, indWoC, indWkH, indOoH);

  LongType trueoH, trueoW;  // true output height, width
  ConvolutionUtils::calcOutSizePool2D(trueoH, trueoW, kH, kW, sH, sW, pH, pW, dH, dW, iH, iW, isSameMode);

  std::vector<LongType> expectedGradOShape =
      ShapeUtils::composeShapeUsingDimsAndIdx({bS, oC, trueoH, trueoW, 0, indIOioC, indOoH, indOoH + 1});
  std::vector<LongType> expectedWeightsShape = ConvolutionUtils::expectWeightsShape(wFormat, kH, kW, iC, oC);
  REQUIRE_TRUE(gradO->isSameShape(expectedGradOShape), 0,
               "CUSTOM CONV2D_INPUT_BP OP: wrong shape of output gradients (next epsilon) array, expected is %s, but "
               "got %s instead !",
               ShapeUtils::shapeAsString(expectedGradOShape).c_str(), ShapeUtils::shapeAsString(gradO).c_str());
  REQUIRE_TRUE(weights->isSameShape(expectedWeightsShape), 0,
               "CUSTOM CONV2D_INPUT_BP OP: wrong shape of weights array, expected is %s, but got %s instead !",
               ShapeUtils::shapeAsString(expectedWeightsShape).c_str(), ShapeUtils::shapeAsString(weights).c_str());

  ConvolutionUtils::conv2dBP(block, &input, weights, nullptr, gradO, gradI, nullptr, nullptr, kH, kW, sH, sW, pH, pW,
                             dH, dW, isSameMode, isNCHW, wFormat);

  return Status::OK;
}

DECLARE_TYPES(conv2d_input_bp) {
  getOpDescriptor()->setAllowedInputTypes(ANY)->setAllowedOutputTypes({ALL_FLOATS});
}

DECLARE_SHAPE_FN(conv2d_input_bp) {
  auto gradIShapeShapeInfo = inputShape->at(0);  // [4]
  auto weightsShapeInfo = inputShape->at(1);     // [kH, kW, iC, oC], [oC, iC, kH, kW], [oC, kH, kW, iC]
  auto gradOShapeInfo = inputShape->at(2);       // [bS, oH, oW, oC] (NHWC) or [bS, oC, oH, oW] (NCHW), epsilon_next

  const LongType rank = 4;

  REQUIRE_TRUE(gradIShapeShapeInfo[0] == 1, 0,
               "CUSTOM CONV2D_INPUT_BP OP: rank of array with output shape must be equal to %i, but got %i instead !",
               1, gradIShapeShapeInfo[0]);
  REQUIRE_TRUE(weightsShapeInfo[0] == rank, 0,
               "CUSTOM CONV2D_INPUT_BP OP: rank of weights array must be equal to %i, but got %i instead !", rank,
               weightsShapeInfo[0]);
  REQUIRE_TRUE(gradOShapeInfo[0] == rank, 0,
               "CUSTOM CONV2D_INPUT_BP OP: rank of output gradients (next epsilon) array must be equal to %i, but got "
               "%i instead !",
               rank, gradOShapeInfo[0]);

  const LongType kH = INT_ARG(0);                                               // filter(kernel) height
  const LongType kW = INT_ARG(1);                                               // filter(kernel) width
  const LongType sH = INT_ARG(2);                                               // strides height
  const LongType sW = INT_ARG(3);                                               // strides width
  const LongType pH = INT_ARG(4);                                               // paddings height
  const LongType pW = INT_ARG(5);                                               // paddings width
  const LongType dH = INT_ARG(6);                                               // dilations height
  const LongType dW = INT_ARG(7);                                               // dilations width
  const int isSameMode = INT_ARG(8);                                       // 0-VALID, 1-SAME
  const int isNCHW = block.getIArguments()->size() > 9 ? !INT_ARG(9) : 1;  // INT_ARG(9): 0-NCHW, 1-NHWC
  const int wFormat = block.getIArguments()->size() > 10
                      ? INT_ARG(10)
                      : 0;  // 0 - [kH, kW, iC, oC], 1 - [oC, iC, kH, kW], 2 - [oC, kH, kW, iC]

  int indIOioC, indIiH, indWoC(0 == wFormat ? 3 : 0), indOoH;
  if (!isNCHW) {
    indIOioC = 3;
    indIiH = 1;
    indOoH = 1;
  } else {
    indIOioC = 1;
    indIiH = 2;
    indOoH = 2;
  }

  std::vector<LongType> gradIShape = INPUT_VARIABLE(0)->template asVectorT<LongType>();

  const LongType bS = gradIShape[0];                 // batch size
  const LongType iH = gradIShape[indIiH];            // input height
  const LongType iW = gradIShape[indIiH + 1];        // input width
  const LongType iC = gradIShape[indIOioC];          // input channels
  const LongType oC = weightsShapeInfo[indWoC + 1];  // output channels

  LongType trueoH, trueoW;  // true output height, width
  ConvolutionUtils::calcOutSizePool2D(trueoH, trueoW, kH, kW, sH, sW, pH, pW, dH, dW, iH, iW, isSameMode);

  std::vector<LongType> expectedGradOShape =
      ShapeUtils::composeShapeUsingDimsAndIdx({bS, oC, trueoH, trueoW, 0, indIOioC, indOoH, indOoH + 1});
  std::vector<LongType> expectedWeightsShape = ConvolutionUtils::expectWeightsShape(wFormat, kH, kW, iC, oC);
  REQUIRE_TRUE(ShapeUtils::areShapesEqual(gradOShapeInfo, expectedGradOShape), 0,
               "CUSTOM CONV2D_INPUT_BP OP: wrong shape of output gradients (next epsilon) array, expected is %s, but "
               "got %s instead !",
               ShapeUtils::shapeAsString(expectedGradOShape).c_str(),
               ShapeUtils::shapeAsString(gradOShapeInfo).c_str());
  REQUIRE_TRUE(ShapeUtils::areShapesEqual(weightsShapeInfo, expectedWeightsShape), 0,
               "CUSTOM CONV2D_INPUT_BP OP: wrong shape of weights array, expected is %s, but got %s instead !",
               ShapeUtils::shapeAsString(expectedWeightsShape).c_str(),
               ShapeUtils::shapeAsString(weightsShapeInfo).c_str());

  LongType* gradIshapeInfo(nullptr);
  ALLOCATE(gradIshapeInfo, block.getWorkspace(), shape::shapeInfoLength(rank), sd::LongType);

  gradIshapeInfo[0] = rank;
  gradIshapeInfo[1] = bS;

  if (isNCHW) {
    gradIshapeInfo[2] = iC;
    gradIshapeInfo[3] = iH;
    gradIshapeInfo[4] = iW;
  } else {
    gradIshapeInfo[2] = iH;
    gradIshapeInfo[3] = iW;
    gradIshapeInfo[4] = iC;
  }

  ShapeUtils::updateStridesAndType(gradIshapeInfo, gradOShapeInfo, shape::order(gradOShapeInfo));

  return SHAPELIST(CONSTANT(gradIshapeInfo));
}

}  // namespace ops
}  // namespace sd

#endif

#endif  // LIBND4J_CONVO_OPS_H
