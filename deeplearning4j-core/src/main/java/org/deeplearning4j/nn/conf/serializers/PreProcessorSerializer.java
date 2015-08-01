/*
 *
 *  * Copyright 2015 Skymind,Inc.
 *  *
 *  *    Licensed under the Apache License, Version 2.0 (the "License");
 *  *    you may not use this file except in compliance with the License.
 *  *    You may obtain a copy of the License at
 *  *
 *  *        http://www.apache.org/licenses/LICENSE-2.0
 *  *
 *  *    Unless required by applicable law or agreed to in writing, software
 *  *    distributed under the License is distributed on an "AS IS" BASIS,
 *  *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  *    See the License for the specific language governing permissions and
 *  *    limitations under the License.
 *
 */

package org.deeplearning4j.nn.conf.serializers;

import com.fasterxml.jackson.core.JsonGenerator;
import com.fasterxml.jackson.databind.JsonSerializer;
import com.fasterxml.jackson.databind.SerializerProvider;

import org.deeplearning4j.nn.conf.OutputPostProcessor;
import org.deeplearning4j.util.Dl4jReflection;

import java.io.IOException;
import java.io.StringWriter;

/**
 * Created by agibsonccc on 2/11/15.
 */
public class PreProcessorSerializer extends JsonSerializer<OutputPostProcessor> {
    @Override
    public void serialize(OutputPostProcessor value, JsonGenerator jgen, SerializerProvider provider) throws IOException {
        try {
            try(StringWriter sw = new StringWriter()) {
                Dl4jReflection.getFieldsAsProperties(value, null).store(sw, "");
                jgen.writeString(value.getClass().getName() + "\t" + sw.toString() + ",");
            }
        } catch (Exception e) {
            throw new RuntimeException(e);
        }

    }
}
