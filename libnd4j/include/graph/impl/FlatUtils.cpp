//
// Created by raver119 on 22.11.2017.
//

#include <graph/FlatUtils.h>
#include <array/DataTypeConversions.h>
#include <array/DataTypeUtils.h>
#include <array/ByteOrderUtils.h>


namespace nd4j {
    namespace graph {
        std::pair<int, int> FlatUtils::fromIntPair(IntPair *pair) {
            return std::pair<int, int>(pair->first(), pair->second());
        }

        std::pair<Nd4jLong, Nd4jLong> FlatUtils::fromLongPair(LongPair *pair) {
            return std::pair<Nd4jLong, Nd4jLong>(pair->first(), pair->second());
        }

        template<typename T>
        NDArray<T> *FlatUtils::fromFlatArray(const nd4j::graph::FlatArray *flatArray) {
            auto rank = static_cast<int>(flatArray->shape()->Get(0));
            auto newShape = new Nd4jLong[shape::shapeInfoLength(rank)];
            memcpy(newShape, flatArray->shape()->data(), shape::shapeInfoByteLength(rank));

            // empty arrays is special case, nothing to restore here
            if (shape::isEmpty(newShape)) {
                delete[] newShape;
                return NDArray<T>::createEmpty();
            }

            auto length = shape::length(newShape);
            auto newBuffer = new T[length];
            auto dtype = DataTypeUtils::fromFlatDataType(flatArray->dtype());

            DataTypeConversions<T>::convertType(newBuffer, (void *)flatArray->buffer()->data(), dtype, ByteOrderUtils::fromFlatByteOrder(flatArray->byteOrder()),  length);

            auto array = new NDArray<T>(newBuffer, newShape);
            array->triggerAllocationFlag(true, true);

            return array;
        }


        template NDArray<float> *FlatUtils::fromFlatArray<float>(const nd4j::graph::FlatArray *flatArray);
        template NDArray<float16> *FlatUtils::fromFlatArray<float16>(const nd4j::graph::FlatArray *flatArray);
        template NDArray<double> *FlatUtils::fromFlatArray<double>(const nd4j::graph::FlatArray *flatArray);
    }
}