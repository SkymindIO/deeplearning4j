/*
 *  ******************************************************************************
 *  *
 *  *
 *  * This program and the accompanying materials are made available under the
 *  * terms of the Apache License, Version 2.0 which is available at
 *  * https://www.apache.org/licenses/LICENSE-2.0.
 *  *
 *  *  See the NOTICE file distributed with this work for additional
 *  *  information regarding copyright ownership.
 *  * Unless required by applicable law or agreed to in writing, software
 *  * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 *  * License for the specific language governing permissions and limitations
 *  * under the License.
 *  *
 *  * SPDX-License-Identifier: Apache-2.0
 *  *****************************************************************************
 */
package org.nd4j.samediff.frameworkimport.onnx.processing

import org.nd4j.autodiff.functions.DifferentialFunction
import org.nd4j.autodiff.samediff.SDVariable
import org.nd4j.autodiff.samediff.SameDiff
import org.nd4j.autodiff.samediff.internal.SameDiffOp
import org.nd4j.enums.DataFormat
import org.nd4j.enums.WeightsFormat
import org.nd4j.ir.OpNamespace
import org.nd4j.linalg.api.ops.impl.layers.convolution.config.Conv2DConfig
import org.nd4j.linalg.api.ops.impl.layers.convolution.config.PaddingMode
import org.nd4j.samediff.frameworkimport.ImportGraph
import org.nd4j.samediff.frameworkimport.hooks.PreImportHook
import org.nd4j.samediff.frameworkimport.hooks.annotations.PreHookRule
import org.nd4j.samediff.frameworkimport.registry.OpMappingRegistry
import org.nd4j.shade.protobuf.GeneratedMessageV3
import org.nd4j.shade.protobuf.ProtocolMessageEnum
import java.lang.IllegalArgumentException

@PreHookRule(nodeNames = ["conv2_1"],opNames = [],frameworkName = "onnx")
class GroupConvPreProcessingRule: PreImportHook {

    override fun doImport(
        sd: SameDiff,
        attributes: Map<String, Any>,
        outputNames: List<String>,
        op: SameDiffOp,
        mappingRegistry: OpMappingRegistry<GeneratedMessageV3, GeneratedMessageV3, GeneratedMessageV3, GeneratedMessageV3, ProtocolMessageEnum, GeneratedMessageV3, GeneratedMessageV3>,
        importGraph: ImportGraph<GeneratedMessageV3, GeneratedMessageV3, GeneratedMessageV3, GeneratedMessageV3, GeneratedMessageV3, GeneratedMessageV3, ProtocolMessageEnum>
    ): Map<String, List<SDVariable>> {
        if(op.op.opName() != "conv2d") {
            throw IllegalArgumentException("Illegal op being processed of type ${op.op.opName()} with node name ${op.op.ownName}")
        }

        val numSizeSplits = attributes.getOrDefault("group",1) as Long
        if(numSizeSplits.toInt() == 1) {
            //no need to split, just perform 1 convolution op
            return emptyMap()
        }

        val descriptor = mappingRegistry.nd4jOpDefs[op.op]!!
        val intArgs = descriptor.argDescriptorList.filter { input -> input.argType == OpNamespace.ArgDescriptor.ArgType.INT64 }.sortedBy { input -> input.argIndex }
        val config = Conv2DConfig.builder()
            .sH(intArgs[2].int64Value)
            .sW(intArgs[3].int64Value)
            .kH(intArgs[0].int64Value)
            .kW(intArgs[1].int64Value)
            .pH(intArgs[4].int64Value)
            .pW(intArgs[5].int64Value)
            .dH(intArgs[6].int64Value)
            .dW(intArgs[7].int64Value)
            .paddingMode(PaddingMode.fromNumber(intArgs[8].int64Value.toInt()))
            .weightsFormat(WeightsFormat.values()[intArgs[10].int64Value.toInt()])
            .dataFormat(DataFormat.values()[intArgs[9].int64Value.toInt()].name)
            .build()

        val listOfFunctions = ArrayList<DifferentialFunction>()
        val weights = sd.getVariable(op.inputsToOp[1])
        //for onnx, this is the number of ops
        val split = sd.split(listOf(op.name + "_split").toTypedArray(),weights,numSizeSplits.toInt(),1)
        val resultMap = HashMap<String,List<SDVariable>>()
        /**
         * NOTE: Need to look in to how to wire up inputs and outputs properly.
         * May need HookResult to return an indicator of variables and ops to remove.
         */
        /**
         * TODO: figure out how to change weights to be divided by 2 in size.
         * Run in to : CUSTOM CONV2D OP: wrong shape of weights array, expected is [256, 24, 5, 5], but got [256, 48, 5, 5] instead !
         * We could also try getting rid of validation. Either way we may need to split up the weights by the number of group occurrences.
         * We technically seem to be doing that down below but that doesn't seem to be enough.
         *
         * The input/output variable weight shape may also not be properly updated. Need to verify.
         */
        val outputVars = ArrayList<SDVariable>()
        split.forEachIndexed { index,input ->
            val varName = "${op.name}_split/$index"
            if(sd.hasVariable(varName))
                sd.variables.remove(varName)
            val outputVariable = sd.cnn().conv2d(varName,input,weights,config)
            resultMap[outputVariable.name()] = listOf(outputVariable)
            outputVars.add(outputVariable)
        }

        /**
         * TODO: Fix output names and potentially look for other inputs
         * in graph where we need to redirect the input/output names
         */
        val toTypedArray = outputVars.toTypedArray()
        val concat = sd.concat(op.name,0,*toTypedArray)
        resultMap[op.name] = listOf(concat)
        return resultMap
    }
}