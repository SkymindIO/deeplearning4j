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
package org.nd4j.linalg.api.memory;

import org.nd4j.linalg.api.buffer.DataType;
import org.nd4j.linalg.api.memory.enums.MemoryKind;

import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.atomic.AtomicLong;

public class WorkspaceAllocationsTracker {

    private Map<MemoryKind,AtomicLong> bytesTracked = new HashMap<>();
    private Map<DataType,AtomicLong> dataTypeCounts = new HashMap<>();

    public WorkspaceAllocationsTracker() {
        Arrays.stream(DataType.values()).forEach(dataType -> {
            dataTypeCounts.put(dataType,new AtomicLong(0));
        });

        Arrays.stream(MemoryKind.values()).forEach(memoryKind -> {
            bytesTracked.put(memoryKind,new AtomicLong(0));
        });

    }



    public long currentBytes(MemoryKind memoryKind) {
        return bytesTracked.get(memoryKind).get();
    }

    public long currentDataTypeCount(DataType toCount) {
        return dataTypeCounts.get(toCount).get();
    }


    /**
     * Allocate bytes in the workspace tracking
     * @param dataType the data type to add
     * @param memoryKind  the kind of memory to add allocation for
     * @param bytes the bytes to add to the workspace
     */
    public void allocate(DataType dataType, MemoryKind memoryKind, long bytes) {
        dataTypeCounts.get(dataType).incrementAndGet();
        bytesTracked.get(memoryKind).addAndGet(bytes);
    }

    /**
     * DeAllocate bytes in the workspace tracking
     * @param dataType the data type to add
     * @param memoryKind the kind of memory to de allocate
     * @param bytes the bytes to add to the workspace
     */
    public void deAllocate(DataType dataType,MemoryKind memoryKind,long bytes) {
        dataTypeCounts.get(dataType).incrementAndGet();
        bytesTracked.get(memoryKind).addAndGet(-bytes);
    }

}
