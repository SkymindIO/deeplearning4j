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
package org.nd4j.linalg.profiler;

import org.nd4j.linalg.api.memory.AllocationsTracker;
import org.nd4j.linalg.api.memory.enums.AllocationKind;
import org.nd4j.linalg.api.ndarray.INDArrayStatistics;
import org.nd4j.linalg.factory.Nd4j;

import java.util.List;

/**
 * A unified profiler covering both memory and op execution timing.
 * This provides a single entry point in to profiling anything about running nd4j calculations
 *
 * @author Adam Gibson
 */
public class UnifiedProfiler {


    private static final UnifiedProfiler INSTANCE = new UnifiedProfiler();

    protected UnifiedProfiler() {

    }



    public List<String> currentWorkspacesForThread() {
        return Nd4j.getWorkspaceManager().getAllWorkspacesIdsForCurrentThread();
    }

    public long executionTimeForOp(String opName) {
        return OpProfiler.getInstance().getOpCounter().getCount(opName);
    }

    public void start() {
        ProfilerConfig profilerConfig = ProfilerConfig.builder()
                .nativeStatistics(true)
                .checkForINF(true)
                .checkForNAN(true)
                .checkElapsedTime(true)
                .checkWorkspaces(true)
                .checkLocality(true)
                .build();
        Nd4j.getExecutioner().setProfilingConfig(profilerConfig);
        OpProfiler.getInstance().setConfig(profilerConfig);

    }

    public void stop() {
        ProfilerConfig profilerConfig = ProfilerConfig.builder()
                .nativeStatistics(false)
                .checkForINF(false)
                .checkForNAN(false)
                .checkElapsedTime(false)
                .checkWorkspaces(false)
                .checkLocality(false)
                .build();
        Nd4j.getExecutioner().setProfilingConfig(profilerConfig);
        OpProfiler.getInstance().setConfig(profilerConfig);

    }


    /**
     * Prints full current state including workspace info,
     * memory stats, op execution status
     * @return
     */
    public String printCurrentStats() {
        StringBuilder stringBuilder = new StringBuilder();
        stringBuilder.append(workspaceInfo());
        stringBuilder.append(memoryStats());
        stringBuilder.append(executionStats());
        stringBuilder.append(AllocationsTracker.getInstance().memoryInfo());
        return stringBuilder.toString();
    }

    /**
     * Workspace allocation information
     * @return
     */
    public String workspaceInfo() {
        return AllocationsTracker.getInstance().memoryPerWorkspace();
    }

    /**
     * Op execution information
     * @return
     */
    public String executionStats() {
        return OpProfiler.getInstance().statsAsString();
    }

    /**
     * Memory stats per device
     * @return
     */
    public String memoryStats() {
        return AllocationsTracker.getInstance().memoryPerDevice();
    }


    public static UnifiedProfiler getInstance() {
        return INSTANCE;
    }


    public static INDArrayStatistics statistics() {
        return OpProfiler.getInstance().getStatistics();
    }

    public long getBytesForAllocationKind(AllocationKind allocationKind,int deviceId) {
        return AllocationsTracker.getInstance().bytesOnDevice(allocationKind,deviceId);
    }

    public long getBytesAllocatedForDevice(int deviceId) {
        return AllocationsTracker.getInstance().bytesOnDevice(deviceId);
    }


}
