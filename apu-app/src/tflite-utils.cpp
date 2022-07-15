#include "tflite-utils.hpp"

#include "tensorflow/lite/kernels/register.h"
#include "tensorflow/lite/model.h"

std::unique_ptr<tflite::Interpreter> createInterpreter(std::string filepath)
{
    std::unique_ptr<tflite::FlatBufferModel> model = tflite::FlatBufferModel::BuildFromFile(filepath.c_str());

    tflite::ops::builtin::BuiltinOpResolver resolver;
    std::unique_ptr<tflite::Interpreter> interpreter;

    tflite::InterpreterBuilder(*model, resolver)(&interpreter);

    return std::move(interpreter);
}
