#include "tensorflow/lite/interpreter.h"
#include "tensorflow/lite/kernels/register.h"
#include "tensorflow/lite/model.h"

#include "vta/tf_driver.h"

#include "tflite-delegate.hpp"

#include <memory>

static const char model_path[] = "/bin/model.tflite";

int main(int argc, char *argv[])
{
    std::unique_ptr<tflite::FlatBufferModel> model = tflite::FlatBufferModel::BuildFromFile(model_path);

    tflite::ops::builtin::BuiltinOpResolver resolver;
    std::unique_ptr<tflite::Interpreter> interpreter;

    tflite::InterpreterBuilder(*model, resolver)(&interpreter);

    std::unique_ptr<TfLiteDelegate, decltype(&tflite::TfLiteCustomDelegateDelete)> delegate(tflite::TfLiteCustomDelegateCreate(NULL), &tflite::TfLiteCustomDelegateDelete);

    interpreter->ModifyGraphWithDelegate(std::move(delegate));

    interpreter->AllocateTensors();

    int8_t *input1 = interpreter->typed_input_tensor<int8_t>(0);
    int8_t *input2 = interpreter->typed_input_tensor<int8_t>(1);
    input1[0] = 5;
    input1[1] = 8;
    input2[0] = 2;
    input2[1] = 4;

    interpreter->Invoke();

    int8_t *out = interpreter->typed_output_tensor<int8_t>(0);

    printf("output: %d %d\n", out[0], out[1]);

    return 0;
}
