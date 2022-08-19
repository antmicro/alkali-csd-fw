#include <gtest/gtest.h>

#include "tensorflow/lite/interpreter.h"
#include "tensorflow/lite/kernels/register.h"
#include "tensorflow/lite/model.h"

#include "vta-delegate.hpp"

#define SIMPLE_MODEL "../tests/data/simple-models/simple-conv2d-add-v2.tflite"

TEST(BasicVTADelegateTests, ModelLoadingCPU)
{
    std::unique_ptr<tflite::FlatBufferModel> model = tflite::FlatBufferModel::BuildFromFile(SIMPLE_MODEL);

    tflite::ops::builtin::BuiltinOpResolver resolver;
    std::unique_ptr<tflite::Interpreter> interpreter;

    tflite::InterpreterBuilder(*model, resolver)(&interpreter);

    interpreter->AllocateTensors();

    interpreter->Invoke();
}

TEST(BasicVTADelegateTests, ModelLoadingVTA)
{
    std::unique_ptr<tflite::FlatBufferModel> model = tflite::FlatBufferModel::BuildFromFile(SIMPLE_MODEL);

    tflite::ops::builtin::BuiltinOpResolver resolver;
    std::unique_ptr<tflite::Interpreter> interpreter;

    tflite::InterpreterBuilder(*model, resolver)(&interpreter);

    std::unique_ptr<TfLiteDelegate, decltype(&tflite::TfLiteVTADelegateDelete)> delegate(tflite::TfLiteVTADelegateCreate(NULL), &tflite::TfLiteVTADelegateDelete);
    interpreter->ModifyGraphWithDelegate(std::move(delegate));

    interpreter->AllocateTensors();

    interpreter->Invoke();
}
