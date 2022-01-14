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
package org.eclipse.deeplearning4j.resources;

import org.deeplearning4j.omnihub.OmnihubConfig;

import java.io.File;

public class OmniHubResource extends BaseResource {


    public OmniHubResource(String fileName) {
        super(fileName,"");
    }

    @Override
    public String fileName() {
        return fileName;
    }

    @Override
    public String rootUrl() {
        return OmnihubConfig.getOmnihubUrl();
    }

    @Override
    public File localCacheDirectory() {
        return OmnihubConfig.getOmnihubHome();
    }

    @Override
    public ResourceType resourceType() {
        return ResourceType.OMNIHUB;
    }


}
