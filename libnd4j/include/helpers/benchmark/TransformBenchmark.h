//
// Created by raver on 2/28/2019.
//

#include "../OpBenchmark.h"

#ifndef DEV_TESTS_TRANSFORMBENCHMARK_H
#define DEV_TESTS_TRANSFORMBENCHMARK_H

namespace nd4j {
    class ND4J_EXPORT TransformBenchmark : public OpBenchmark {
    public:
        TransformBenchmark() : OpBenchmark() {
            //
        }

        TransformBenchmark(transform::StrictOps op, NDArray *x, NDArray *z) : OpBenchmark(x, z) {
            _opNum = (int) op;
        }

        void executeOnce() override {
            NativeOpExcutioner::execTransformStrict(_opNum, _x->buffer(), _x->shapeInfo(),  _z->buffer(), _z->shapeInfo(), nullptr, nullptr, nullptr);
        }

        std::string orders() {
            std::string result;
            result += _x->ordering();
            result += "/";
            result += _z == nullptr ? _x->ordering() : _z->ordering();
            return result;
        }

        OpBenchmark* clone() override  {
            return new TransformBenchmark((transform::StrictOps) _opNum, _x, _z);
        }
    };
}

#endif //DEV_TESTS_SCALARBENCHMARK_H
