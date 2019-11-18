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
// @author Yurii Shyrma (iuriish@yahoo.com)
//

#include <ops/declarable/PlatformHelper.h>
#include <ops/declarable/OpRegistrator.h>
#include <platform_boilerplate.h>

#include <helpers/MKLDNNStream.h>
#include "mkldnnUtils.h"
#include <ops/declarable/helpers/convolutions.h>


namespace nd4j      {
namespace ops       {
namespace platforms {

//////////////////////////////////////////////////////////////////////////
static void deconv2dMKLDNN(const NDArray* input, const NDArray* weights, const NDArray* bias, NDArray* output,
                            const int kH, const int kW, const int sH, const int sW, const int pH, const int pW, const int dH, const int dW,
                            const int isSameMode) {

    // input [bS, iH, iW, iC] nchw, mkl doesn't support format nhwc
    // weights [oC, iC, kH, kW] always, mkl doesn't support weights format [kH, kW, oC, iC]
    // bias [oC], may be nullptr

    // output [bS, oH, oW, oC] nchw, mkl doesn't support format nhwc

    int bS, iC, iH, iW, oC, oH, oW;                             // batch size, input channels, input height/width, output channels, output height/width;
    int indIOioC, indIiH, indWoC, indWiC, indWkH, indOoH;       // corresponding indexes
    ConvolutionUtils::getSizesAndIndexesConv2d(true, *input, *output, bS, iC, iH, iW, oC, oH, oW, indIOioC, indIiH, indWoC, indWiC, indWkH, indOoH);

    int dHmkl(dH), dWmkl(dW), pHmkl(pH), pWmkl(pW);
    ConvolutionUtils::calcPaddingAndDilationForConv2DMKL(oH, oW, iH, iW, kH, kW, sH, sW, isSameMode, pHmkl, pWmkl, dHmkl, dWmkl);

    mkldnn::memory::dims strides   = { sH, sW };
    mkldnn::memory::dims padding   = { pH, pW };
    mkldnn::memory::dims padding_r = { pHmkl, pWmkl };
    mkldnn::memory::dims dilation  = { dHmkl, dWmkl };

    // input type
    mkldnn::memory::data_type xType;
    if(input->dataType() == DataType::FLOAT32)
        xType = mkldnn::memory::data_type::f32;
    else if(input->dataType() == DataType::HALF)
        xType = mkldnn::memory::data_type::f16;
    else if(input->dataType() == DataType::UINT8)
        xType = mkldnn::memory::data_type::u8;
    else
        xType = mkldnn::memory::data_type::s8;

    // weights type
    mkldnn::memory::data_type wType = xType;
    if(xType == mkldnn::memory::data_type::u8)
        wType = mkldnn::memory::data_type::s8;

    // output and bias type (have the same types)
    mkldnn::memory::data_type zType;
    if(output->dataType() == DataType::FLOAT32)
        zType = mkldnn::memory::data_type::f32;
    else if(output->dataType() == DataType::HALF)
        zType = mkldnn::memory::data_type::f16;
    else if(output->dataType() == DataType::UINT8)
        zType = mkldnn::memory::data_type::u8;
    else if(output->dataType() == DataType::INT8)
        zType = mkldnn::memory::data_type::s8;
    else
        zType = mkldnn::memory::data_type::s32;


    mkldnn::memory::format_tag xFormat = mkldnn::memory::format_tag::nchw;   // isNCHW ? mkldnn::memory::format_tag::nchw : mkldnn::memory::format_tag::nhwc;
    mkldnn::memory::format_tag wFormat = mkldnn::memory::format_tag::oihw;

    mkldnn::memory::dims xDims = {bS, iC, iH, iW};
    mkldnn::memory::dims wDims = {oC, iC, kH, kW};
    mkldnn::memory::dims zDims = {bS, oC, oH, oW};

    // memory descriptors for arrays

    // input
    mkldnn::memory::desc x_mkl_md  = mkldnn::memory::desc(xDims, xType, mkldnn::memory::format_tag::any);
    mkldnn::memory::desc x_user_md = mkldnn::memory::desc(xDims, xType, xFormat);
    x_user_md.data.format_kind = mkldnn_blocked;    // overrides format
    x_user_md.data.format_desc.blocking.strides[0] = input->stridesOf()[0];
    x_user_md.data.format_desc.blocking.strides[1] = input->stridesOf()[1];
    x_user_md.data.format_desc.blocking.strides[2] = input->stridesOf()[2];
    x_user_md.data.format_desc.blocking.strides[3] = input->stridesOf()[3];

    // weights
    mkldnn::memory::desc w_mkl_md  = mkldnn::memory::desc(wDims, wType, mkldnn::memory::format_tag::any);
    mkldnn::memory::desc w_user_md = mkldnn::memory::desc(wDims, wType, wFormat);
    w_user_md.data.format_kind = mkldnn_blocked;    // overrides format
    w_user_md.data.format_desc.blocking.strides[0] = weights->stridesOf()[0];
    w_user_md.data.format_desc.blocking.strides[1] = weights->stridesOf()[1];
    w_user_md.data.format_desc.blocking.strides[2] = weights->stridesOf()[2];
    w_user_md.data.format_desc.blocking.strides[3] = weights->stridesOf()[3];

    // bias
    mkldnn::memory::desc b_mkl_md;
    if(bias != nullptr)
        b_mkl_md = mkldnn::memory::desc({oC}, zType, mkldnn::memory::format_tag::x);

    // output
    mkldnn::memory::desc z_mkl_md  = mkldnn::memory::desc(zDims, zType, mkldnn::memory::format_tag::any);
    mkldnn::memory::desc z_user_md = mkldnn::memory::desc(zDims, zType, xFormat);
    z_user_md.data.format_kind = mkldnn_blocked;    // overrides format
    z_user_md.data.format_desc.blocking.strides[0] = output->stridesOf()[0];
    z_user_md.data.format_desc.blocking.strides[1] = output->stridesOf()[1];
    z_user_md.data.format_desc.blocking.strides[2] = output->stridesOf()[2];
    z_user_md.data.format_desc.blocking.strides[3] = output->stridesOf()[3];

    auto engine = mkldnnUtils::getEngine(LaunchContext::defaultContext()->engine());

    // operation primitive description
    mkldnn::deconvolution_forward::desc op_desc(mkldnn::prop_kind::forward_inference, mkldnn::algorithm::deconvolution_direct,
                                                x_mkl_md, w_mkl_md, b_mkl_md, z_mkl_md, strides, dilation, padding, padding_r);
    mkldnn::deconvolution_forward::primitive_desc op_prim_desc(op_desc, engine);

    // arguments (memory buffers) necessary for calculations
    std::unordered_map<int, mkldnn::memory> args;

    mkldnn::stream stream(engine);

    // provide memory buffers and check whether reorder is required

    // input
    auto x_user_mem = mkldnn::memory(x_user_md, engine, input->getBuffer());
    const bool xReorder = op_prim_desc.src_desc() != x_user_mem.get_desc();
    auto x_mkl_mem = xReorder ? mkldnn::memory(op_prim_desc.src_desc(), engine) : x_user_mem;
    if (xReorder)
        mkldnn::reorder(x_user_mem, x_mkl_mem).execute(stream, x_user_mem, x_mkl_mem);
    args[MKLDNN_ARG_SRC] = x_mkl_mem;

    // weights
    auto w_user_mem = mkldnn::memory(w_user_md, engine, weights->getBuffer());
    const bool wReorder = op_prim_desc.weights_desc() != w_user_mem.get_desc();
    auto w_mkl_mem = wReorder ? mkldnn::memory(op_prim_desc.weights_desc(), engine) : w_user_mem;
    if (wReorder)
        mkldnn::reorder(w_user_mem, w_mkl_mem).execute(stream, w_user_mem, w_mkl_mem);
    args[MKLDNN_ARG_WEIGHTS] = w_mkl_mem;

    // bias
    if(bias != nullptr) {
        auto b_mkl_mem = mkldnn::memory(b_mkl_md, engine, bias->getBuffer());
        args[MKLDNN_ARG_BIAS] = b_mkl_mem;
    }

    // output
    auto z_user_mem = mkldnn::memory(z_user_md, engine, output->getBuffer());
    const bool zReorder = op_prim_desc.dst_desc() != z_user_mem.get_desc();
    auto z_mkl_mem = zReorder ? mkldnn::memory(op_prim_desc.dst_desc(), engine) : z_user_mem;
    args[MKLDNN_ARG_DST] = z_mkl_mem;

    // run calculations
    mkldnn::deconvolution_forward(op_prim_desc).execute(stream, args);

    // reorder outputs if necessary
    if (zReorder)
        mkldnn::reorder(z_mkl_mem, z_user_mem).execute(stream, z_mkl_mem, z_user_mem);

    stream.wait();

    // shape::printArray(z_mkl_mem.map_data<float>(),8);
}

//////////////////////////////////////////////////////////////////////////
static void deconv2dBackPropMKLDNN(const NDArray* input, const NDArray* weights, const NDArray* gradO, NDArray* gradI, NDArray* gradW, NDArray* gradB,
                                    const int kH, const int kW, const int sH, const int sW, const int pH, const int pW, const int dH, const int dW,
                                    const int isSameMode) {

    // input and gradI [bS, iH, iW, iC], mkl doesn't support ndhwc format
    // weights and gradW [oC, iC, kH, kW] always, mkl doesn't support weights format [kH, kW, oC, iC]
    // gradB [oC], may be nullptr
    // gradO [bS, oH, oW, oC]

    int bS, iC, iH, iW, oC, oH, oW;                             // batch size, input channels, input height/width, output channels, output height/width;
    int indIOioC, indIiH, indWoC, indWiC, indWkH, indOoH;       // corresponding indexes
    ConvolutionUtils::getSizesAndIndexesConv2d(true, *input, *gradO, bS, iC, iH, iW, oC, oH, oW, indIOioC, indIiH, indWoC, indWiC, indWkH, indOoH);

    int dHmkl(dH), dWmkl(dW), pHmkl(pH), pWmkl(pW);
    ConvolutionUtils::calcPaddingAndDilationForConv2DMKL(oH, oW, iH, iW, kH, kW, sH, sW, isSameMode, pHmkl, pWmkl, dHmkl, dWmkl);

    mkldnn::memory::dims strides   = { sH, sW };
    mkldnn::memory::dims padding   = { pH, pW };
    mkldnn::memory::dims padding_r = { pHmkl, pWmkl };
    mkldnn::memory::dims dilation  = { dHmkl, dWmkl };
    // input type
    mkldnn::memory::data_type xType = input->dataType() == DataType::FLOAT32 ? mkldnn::memory::data_type::f32 : mkldnn::memory::data_type::bf16;
    // weights type
    mkldnn::memory::data_type wType = weights->dataType() == DataType::FLOAT32 ? mkldnn::memory::data_type::f32 : mkldnn::memory::data_type::bf16;
    // gradO type
    mkldnn::memory::data_type gradOType = gradO->dataType() == DataType::FLOAT32 ? mkldnn::memory::data_type::f32 : mkldnn::memory::data_type::bf16;
    // gradI type
    mkldnn::memory::data_type gradIType = gradI->dataType() == DataType::FLOAT32 ? mkldnn::memory::data_type::f32 : mkldnn::memory::data_type::bf16;
    // gradW type
    mkldnn::memory::data_type gradWType = gradW->dataType() == DataType::FLOAT32 ? mkldnn::memory::data_type::f32 : mkldnn::memory::data_type::bf16;
    // gradB type
    mkldnn::memory::data_type gradBType = gradB != nullptr ? (gradB->dataType() == DataType::FLOAT32 ? mkldnn::memory::data_type::f32 : mkldnn::memory::data_type::bf16) : mkldnn::memory::data_type::f32;

    mkldnn::memory::format_tag xFormat = mkldnn::memory::format_tag::nchw;      // isNCHW ? mkldnn::memory::format_tag::nchw : mkldnn::memory::format_tag::nhwc;
    mkldnn::memory::format_tag wFormat = mkldnn::memory::format_tag::oihw;

    mkldnn::memory::dims xDims = {bS, iC, iH, iW};
    mkldnn::memory::dims wDims = {oC, iC, kH, kW};
    mkldnn::memory::dims zDims = {bS, oC, oH, oW};

    // memory descriptors for arrays

    // input
    mkldnn::memory::desc x_mkl_md  = mkldnn::memory::desc(xDims, xType, mkldnn::memory::format_tag::any);
    mkldnn::memory::desc x_user_md = mkldnn::memory::desc(xDims, xType, xFormat);
    x_user_md.data.format_kind = mkldnn_blocked;    // overrides format
    x_user_md.data.format_desc.blocking.strides[0] = input->stridesOf()[0];
    x_user_md.data.format_desc.blocking.strides[1] = input->stridesOf()[1];
    x_user_md.data.format_desc.blocking.strides[2] = input->stridesOf()[2];
    x_user_md.data.format_desc.blocking.strides[3] = input->stridesOf()[3];

    // weights
    mkldnn::memory::desc w_mkl_md  = mkldnn::memory::desc(wDims, wType, mkldnn::memory::format_tag::any);
    mkldnn::memory::desc w_user_md = mkldnn::memory::desc(wDims, wType, wFormat);
    w_user_md.data.format_kind = mkldnn_blocked;    // overrides format
    w_user_md.data.format_desc.blocking.strides[0] = weights->stridesOf()[0];
    w_user_md.data.format_desc.blocking.strides[1] = weights->stridesOf()[1];
    w_user_md.data.format_desc.blocking.strides[2] = weights->stridesOf()[2];
    w_user_md.data.format_desc.blocking.strides[3] = weights->stridesOf()[3];

    // gradO
    mkldnn::memory::desc gradO_mkl_md  = mkldnn::memory::desc(zDims, gradOType, mkldnn::memory::format_tag::any);
    mkldnn::memory::desc gradO_user_md = mkldnn::memory::desc(zDims, gradOType, xFormat);
    gradO_user_md.data.format_kind = mkldnn_blocked;    // overrides format
    gradO_user_md.data.format_desc.blocking.strides[0] = gradO->stridesOf()[0];
    gradO_user_md.data.format_desc.blocking.strides[1] = gradO->stridesOf()[1];
    gradO_user_md.data.format_desc.blocking.strides[2] = gradO->stridesOf()[2];
    gradO_user_md.data.format_desc.blocking.strides[3] = gradO->stridesOf()[3];

    // gradI
    mkldnn::memory::desc gradI_mkl_md  = mkldnn::memory::desc(xDims, gradIType, mkldnn::memory::format_tag::any);
    mkldnn::memory::desc gradI_user_md = mkldnn::memory::desc(xDims, gradIType, xFormat);
    gradI_user_md.data.format_kind = mkldnn_blocked;    // overrides format
    gradI_user_md.data.format_desc.blocking.strides[0] = gradI->stridesOf()[0];
    gradI_user_md.data.format_desc.blocking.strides[1] = gradI->stridesOf()[1];
    gradI_user_md.data.format_desc.blocking.strides[2] = gradI->stridesOf()[2];
    gradI_user_md.data.format_desc.blocking.strides[3] = gradI->stridesOf()[3];

    // gradW
    mkldnn::memory::desc gradW_mkl_md  = mkldnn::memory::desc(wDims, gradWType, mkldnn::memory::format_tag::any);
    mkldnn::memory::desc gradW_user_md = mkldnn::memory::desc(wDims, gradWType, wFormat);
    gradW_user_md.data.format_kind = mkldnn_blocked;    // overrides format
    gradW_user_md.data.format_desc.blocking.strides[0] = gradW->stridesOf()[0];
    gradW_user_md.data.format_desc.blocking.strides[1] = gradW->stridesOf()[1];
    gradW_user_md.data.format_desc.blocking.strides[2] = gradW->stridesOf()[2];
    gradW_user_md.data.format_desc.blocking.strides[3] = gradW->stridesOf()[3];

    // gradB
    mkldnn::memory::desc gradB_mkl_md;
    if(gradB != nullptr)
        gradB_mkl_md = mkldnn::memory::desc({oC}, gradBType, mkldnn::memory::format_tag::x);


    auto engine = mkldnnUtils::getEngine(LaunchContext::defaultContext()->engine());

    // forward primitive description
    mkldnn::deconvolution_forward::desc op_ff_desc(mkldnn::prop_kind::forward_inference, mkldnn::algorithm::deconvolution_direct, x_mkl_md, w_mkl_md, gradB_mkl_md, gradO_mkl_md, strides, dilation, padding, padding_r);
    mkldnn::deconvolution_forward::primitive_desc op_ff_prim_desc(op_ff_desc, engine);

    // backward data primitive description
    mkldnn::deconvolution_backward_data::desc op_data_bp_desc(mkldnn::algorithm::deconvolution_direct, gradI_mkl_md, w_mkl_md, gradO_mkl_md, strides, dilation, padding, padding_r);
    mkldnn::deconvolution_backward_data::primitive_desc op_data_bp_prim_desc(op_data_bp_desc, engine, op_ff_prim_desc);

    // backward weights primitive description
    mkldnn::deconvolution_backward_weights::desc op_weights_bp_desc(mkldnn::algorithm::deconvolution_direct, x_mkl_md, gradW_mkl_md, gradB_mkl_md, gradO_mkl_md, strides, dilation, padding, padding_r);
    mkldnn::deconvolution_backward_weights::primitive_desc op_weights_bp_prim_desc(op_weights_bp_desc, engine, op_ff_prim_desc);

    // arguments (memory buffers) necessary for calculations
    std::unordered_map<int, mkldnn::memory> args;

    mkldnn::stream stream(engine);

    // provide memory buffers and check whether reorder is required

    // input
    auto x_user_mem = mkldnn::memory(x_user_md, engine, input->getBuffer());
    const bool xReorder = op_weights_bp_prim_desc.src_desc() != x_user_mem.get_desc();
    auto x_mkl_mem = xReorder ? mkldnn::memory(op_weights_bp_prim_desc.src_desc(), engine) : x_user_mem;
    if (xReorder)
        mkldnn::reorder(x_user_mem, x_mkl_mem).execute(stream, x_user_mem, x_mkl_mem);
    args[MKLDNN_ARG_SRC] = x_mkl_mem;

    // weights
    auto w_user_mem = mkldnn::memory(w_user_md, engine, weights->getBuffer());
    const bool wReorder = op_data_bp_prim_desc.weights_desc() != w_user_mem.get_desc();
    auto w_mkl_mem = wReorder ? mkldnn::memory(op_data_bp_prim_desc.weights_desc(), engine) : w_user_mem;
    if (wReorder)
        mkldnn::reorder(w_user_mem, w_mkl_mem).execute(stream, w_user_mem, w_mkl_mem);
    args[MKLDNN_ARG_WEIGHTS] = w_mkl_mem;

    // gradO
    auto gradO_user_mem = mkldnn::memory(gradO_user_md, engine, gradO->getBuffer());
    const bool gradOReorder = op_data_bp_prim_desc.diff_dst_desc() != gradO_user_mem.get_desc();
    auto gradO_mkl_mem = gradOReorder ? mkldnn::memory(op_data_bp_prim_desc.diff_dst_desc(), engine) : gradO_user_mem;
    if (gradOReorder)
        mkldnn::reorder(gradO_user_mem, gradO_mkl_mem).execute(stream, gradO_user_mem, gradO_mkl_mem);
    args[MKLDNN_ARG_DIFF_DST] = gradO_mkl_mem;

    // gradI
    auto gradI_user_mem = mkldnn::memory(gradI_user_md, engine, gradI->getBuffer());
    const bool gradIReorder = op_data_bp_prim_desc.diff_src_desc() != gradI_user_mem.get_desc();
    auto gradI_mkl_mem = gradIReorder ? mkldnn::memory(op_data_bp_prim_desc.diff_src_desc(), engine) : gradI_user_mem;
    args[MKLDNN_ARG_DIFF_SRC] = gradI_mkl_mem;

    // gradW
    auto gradW_user_mem = mkldnn::memory(gradW_user_md, engine, gradW->getBuffer());
    const bool gradWReorder = op_weights_bp_prim_desc.diff_weights_desc() != gradW_user_mem.get_desc();
    auto gradW_mkl_mem = gradWReorder ? mkldnn::memory(op_weights_bp_prim_desc.diff_weights_desc(), engine) : gradW_user_mem;
    args[MKLDNN_ARG_DIFF_WEIGHTS] = gradW_mkl_mem;

    // gradB
    if(gradB != nullptr) {
        auto gradB_mkl_mem = mkldnn::memory(gradB_mkl_md, engine, gradB->getBuffer());
        args[MKLDNN_ARG_DIFF_BIAS] = gradB_mkl_mem;
    }

    // run backward data calculations
    mkldnn::deconvolution_backward_data(op_data_bp_prim_desc).execute(stream, args);

    // run backward weights calculations
    mkldnn::deconvolution_backward_weights(op_weights_bp_prim_desc).execute(stream, args);

    // reorder gradI if necessary
    if (gradIReorder)
        mkldnn::reorder(gradI_mkl_mem, gradI_user_mem).execute(stream, gradI_mkl_mem, gradI_user_mem);
    if (gradWReorder)
        mkldnn::reorder(gradW_mkl_mem, gradW_user_mem).execute(stream, gradW_mkl_mem, gradW_user_mem);

    stream.wait();

    // shape::printArray(z_mkl_mem.map_data<float>(),8);
}


//////////////////////////////////////////////////////////////////////////
PLATFORM_IMPL(deconv2d) {

    auto input   = INPUT_VARIABLE(0);                                    // [bS, iH, iW, iC] (NHWC) or [bS, iC, iH, iW] (NCHW)
    auto weights = INPUT_VARIABLE(1);                                    // [kH, kW, oC, iC] always
    auto bias    = block.width() > 2 ? INPUT_VARIABLE(2) : nullptr;      // [oC]

    auto output  = OUTPUT_VARIABLE(0);                                   // [bS, oH, oW, oC] (NHWC) or [bS, oC, oH, oW] (NCHW)

    REQUIRE_TRUE(input->rankOf()   == 4, 0, "CUSTOM DECONV2D_MKLDNN OP: rank of input array must be equal to 4, but got %i instead !", input->rankOf());
    REQUIRE_TRUE(weights->rankOf() == 4, 0, "CUSTOM DECONV2D_MKLDNN OP: rank of weights array must be equal to 4, but got %i instead !", weights->rankOf());

    int kH = INT_ARG(0) > 0 ? INT_ARG(0) : static_cast<int>(weights->sizeAt(0));// filter(kernel) height
    int kW = INT_ARG(1) > 0 ? INT_ARG(1) : static_cast<int>(weights->sizeAt(1));// filter(kernel) width
    int sH = INT_ARG(2);                                                        // strides height
    int sW = INT_ARG(3);                                                        // strides width
    int pH = INT_ARG(4);                                                        // paddings height
    int pW = INT_ARG(5);                                                        // paddings width
    int dH = INT_ARG(6);                                                        // dilations height
    int dW = INT_ARG(7);                                                        // dilations width
    int isSameMode = INT_ARG(8);                                                // 0-VALID, 1-SAME
    int isNCHW     = block.getIArguments()->size() > 9 ? !INT_ARG(9) : 1;       // INT_ARG(9): 0-NCHW,  1-NHWC

    int bS, iC, iH, iW, oC, oH, oW;                             // batch size, input channels, input height/width, output channels, output height/width;
    int indIOioC, indIiH, indWoC, indWiC, indWkH, indOoH;       // corresponding indexes
    ConvolutionUtils::getSizesAndIndexesConv2d(isNCHW, *input, *output, bS, iC, iH, iW, oC, oH, oW, indIOioC, indIiH, indWoC, indWiC, indWkH, indOoH);

    std::vector<Nd4jLong>  expectedWeightsShape = {kH, kW, oC, iC};
    REQUIRE_TRUE(weights->isSameShape(expectedWeightsShape), 0, "CUSTOM DECONV2D_MKLDNN OP: wrong shape of weights array, expected is %s, but got %s instead !", ShapeUtils::shapeAsString(expectedWeightsShape).c_str(), ShapeUtils::shapeAsString(weights).c_str());
    if (bias)
        REQUIRE_TRUE(bias->rankOf() <= 2 && oC == bias->lengthOf(), 0, "CUSTOM DECONV2D_MKLDNN OP: wrong shape of array with biases, expected rank, length: <=2, %i, but got %i, %i instead !", oC, bias->rankOf(), bias->lengthOf());

    if(isSameMode){                       // SAME
        //Note: we're intentionally swapping iH and oH, to calculated the padding for a"normal" conv (not deconv) forward pass
        ConvolutionUtils::calcPadding2D(pH, pW, iH, iW, oH, oW, kH, kW, sH, sW, dH, dW);
    }

    // mkl supports only [oC, iC, kH, kW] format for weights
    weights = new NDArray(weights->permute({2,3,0,1}));        // [kH, kW, oC, iC] -> [oC, iC, kH, kW]

    // mkl supports only NCHW
    if(!isNCHW) {
        input = new NDArray(input->permute({0,3,1,2}));       // [bS, iH, iW, iC] -> [bS, iC, iH, iW]
        output = new NDArray(output->permute({0,3,1,2}));     // [bS, oH, oW, oC] -> [bS, oC, oH, oW]
    }

    deconv2dMKLDNN(input, weights, bias, output, kH, kW, sH, sW, pH, pW, dH, dW, isSameMode);

    delete weights;

    if(!isNCHW) {
        delete input;
        delete output;
    }

    return Status::OK();
}

PLATFORM_CHECK(deconv2d) {
    // we don't want to use mkldnn if cpu doesn't support avx/avx2
    // if (::optimalLevel() < 2)
    //     return false;

    auto input   = INPUT_VARIABLE(0);
    auto weights = INPUT_VARIABLE(1);
    auto bias    = block.width() > 2 ? INPUT_VARIABLE(2) : nullptr;

    auto output  = INPUT_VARIABLE(0);

    const DataType xType = input->dataType();
    const DataType wType = weights->dataType();
    const DataType zType = output->dataType();
    const DataType bType = bias != nullptr ? bias->dataType() : zType;

    return block.isUseMKLDNN() && (
            (xType==DataType::FLOAT32 && wType==DataType::FLOAT32 && bType==DataType::FLOAT32 && zType==DataType::FLOAT32) ||
            ((xType==DataType::UINT8 || xType==DataType::INT8) && wType==DataType::INT8 && (zType==DataType::UINT8 || zType==DataType::INT8 || zType==DataType::INT32 || zType==DataType::FLOAT32) && bType == zType)
          );
}


//////////////////////////////////////////////////////////////////////////
PLATFORM_IMPL(deconv2d_bp) {

    auto input   = INPUT_VARIABLE(0);                                                // [bS, iH, iW, iC] (NHWC) or [bS, iC, iH, iW] (NCDHW)
    auto weights = INPUT_VARIABLE(1);                                                // [kH, kW, oC, iC] always
    auto bias    = block.width() > 3 ? INPUT_VARIABLE(2) : nullptr;                  // [oC]
    auto gradO   = block.width() > 3 ? INPUT_VARIABLE(3) : INPUT_VARIABLE(2);        // [bS, oH, oW, oC] (NHWC) or [bS, oC, oH, oW] (NCDHW), epsilon_next

    auto gradI = OUTPUT_VARIABLE(0);                                                 // [bS, iH, iW, iC] (NHWC) or [bS, iC, iH, iW] (NCDHW), gradI
    auto gradW = OUTPUT_VARIABLE(1);                                                 // [kH, kW, oC, iC] always
    auto gradB = block.width() > 3 ? OUTPUT_VARIABLE(2) : nullptr;                   // [oC]

    REQUIRE_TRUE(input->rankOf()   == 4, 0, "CUSTOM DECONV2D_MKLDNN_BP OP: rank of input array must be equal to 4, but got %i instead !", input->rankOf());
    REQUIRE_TRUE(weights->rankOf() == 4, 0, "CUSTOM DECONV2D_MKLDNN_BP OP: rank of weights array must be equal to 4 , but got %i instead !", weights->rankOf());
    REQUIRE_TRUE(gradO->rankOf()   == 4, 0, "CUSTOM DECONV2D_MKLDNN_BP OP: rank of output gradients (next epsilon) array must be equal to 4, but got %i instead !", gradO->rankOf());


    int kH = INT_ARG(0) > 0 ? INT_ARG(0) : static_cast<int>(weights->sizeAt(0));// filter(kernel) height
    int kW = INT_ARG(1) > 0 ? INT_ARG(1) : static_cast<int>(weights->sizeAt(1));// filter(kernel) width
    int sH = INT_ARG(2);                                                        // strides height
    int sW = INT_ARG(3);                                                        // strides width
    int pH = INT_ARG(4);                                                        // paddings height
    int pW = INT_ARG(5);                                                        // paddings width
    int dH = INT_ARG(6);                                                        // dilations height
    int dW = INT_ARG(7);                                                        // dilations width
    int isSameMode = INT_ARG(8);                                                // 0-VALID, 1-SAME
    int isNCHW  = block.getIArguments()->size() > 9 ? !INT_ARG(9) : 1;          // INT_ARG(9): 1-NHWC, 0-NCHW

    int bS, iC, iH, iW, oC, oH, oW;                             // batch size, input channels, input height/width, output channels, output height/width;
    int indIOioC, indIiH, indWoC, indWiC, indWkH, indOoH;       // corresponding indexes
    ConvolutionUtils::getSizesAndIndexesConv2d(isNCHW, *input, *gradO, bS, iC, iH, iW, oC, oH, oW, indIOioC, indIiH, indWoC, indWiC, indWkH, indOoH);

    int trueoH, trueoW;          // true output height, width
    ConvolutionUtils::calcOutSizeDeconv2D(trueoH, trueoW, kH, kW, sH, sW, pH, pW, dH, dW, iH, iW, isSameMode);

    std::vector<Nd4jLong> expectedGradOShape  = ShapeUtils::composeShapeUsingDimsAndIdx({bS,oC,trueoH,trueoW,  0,indIOioC,indOoH,indOoH+1});
    std::vector<Nd4jLong> expectedWeightsShape = {kH, kW, oC, iC};
    REQUIRE_TRUE(gradO->isSameShape(expectedGradOShape), 0,  "CUSTOM DECONV2D_MKLDNN_BP OP: wrong shape of output gradients (next epsilon) array, expected is %s, but got %s instead !", ShapeUtils::shapeAsString(expectedGradOShape).c_str(), ShapeUtils::shapeAsString(gradO).c_str());
    REQUIRE_TRUE(weights->isSameShape(expectedWeightsShape), 0, "CUSTOM DECONV2D_MKLDNN_BP OP: wrong shape of weights array, expected is %s, but got %s instead !", ShapeUtils::shapeAsString(expectedWeightsShape).c_str(), ShapeUtils::shapeAsString(weights).c_str());
    if(bias)
        REQUIRE_TRUE(bias->rankOf() <= 2 && oC == bias->lengthOf(), 0, "CUSTOM DECONV2D_MKLDNN_BP OP: wrong shape of array with biases, expected rank, length: <=2, %i, but got %i, %i instead !", oC, bias->rankOf(), bias->lengthOf());

    if(isSameMode){                       // SAME
        //Note: we're intentionally swapping iH and oH, to calculated the padding for a"normal" conv (not deconv) forward pass
        ConvolutionUtils::calcPadding2D(pH, pW, iH, iW, oH, oW, kH, kW, sH, sW, dH, dW);
    }

    // mkl supports only [oC, iC, kH, kW] for weights
    weights = new NDArray(weights->permute({2,3,0,1}));        // [kH, kW, oC, iC] -> [oC, iC, kH, kW]
    gradW   = new NDArray(gradW->permute({2,3,0,1}));          // [kH, kW, oC, iC] -> [oC, iC, kH, kW]

    // mkl supports NCHW format only
    if(!isNCHW) {
        input = new NDArray(input->permute({0,3,1,2}));    // [bS, iH, iW, iC] -> [bS, iC, iH, iW]
        gradI = new NDArray(gradI->permute({0,3,1,2}));    // [bS, iH, iW, iC] -> [bS, iC, iH, iW]
        gradO = new NDArray(gradO->permute({0,3,1,2}));    // [bS, oH, oW, oC] -> [bS, oC, oH, oW]
    }

    deconv2dBackPropMKLDNN(input, weights, gradO, gradI, gradW, gradB, kH, kW, sH, sW, pH, pW, dH, dW, isSameMode);

    delete weights;
    delete gradW;

    if(!isNCHW) {
        delete input;
        delete gradI;
        delete gradO;
    }

    return Status::OK();
}

PLATFORM_CHECK(deconv2d_bp) {
    // we don't want to use mkldnn if cpu doesn't support avx/avx2
    // if (::optimalLevel() < 2)
    //     return false;

    auto input   = INPUT_VARIABLE(0);                                                // [bS, iH, iW, iC] (NHWC) or [bS, iC, iH, iW] (NCDHW)
    auto weights = INPUT_VARIABLE(1);                                                // [kH, kW, oC, iC] always
    auto bias    = block.width() > 3 ? INPUT_VARIABLE(2) : nullptr;                  // [oC]
    auto gradO   = block.width() > 3 ? INPUT_VARIABLE(3) : INPUT_VARIABLE(2);        // [bS, oH, oW, oC] (NHWC) or [bS, oC, oH, oW] (NCDHW), epsilon_next

    auto gradI = OUTPUT_VARIABLE(0);                                                 // [bS, iH, iW, iC] (NHWC) or [bS, iC, iH, iW] (NCDHW), gradI
    auto gradW = OUTPUT_VARIABLE(1);                                                 // [kH, kW, oC, iC] always
    auto gradB = block.width() > 3 ? OUTPUT_VARIABLE(2) : nullptr;                   // [oC]


    const DataType xType = input->dataType();
    const DataType wType = weights->dataType();
    const DataType gradOType = gradO->dataType();

    const DataType gradIType = gradI->dataType();
    const DataType gradWType = gradW->dataType();
    const DataType gradBType = gradB != nullptr ? gradB->dataType() : DataType::FLOAT32;

    return block.isUseMKLDNN() && ((xType==DataType::FLOAT32 || xType==DataType::BFLOAT16) && (wType==DataType::FLOAT32 || wType==DataType::BFLOAT16) && (gradOType==DataType::FLOAT32 || gradOType==DataType::BFLOAT16) && (gradIType==DataType::FLOAT32 || gradIType==DataType::BFLOAT16) && (gradWType==DataType::FLOAT32 || gradWType==DataType::BFLOAT16) && (gradBType==DataType::FLOAT32 || gradBType==DataType::BFLOAT16) );
}


}
}
}
