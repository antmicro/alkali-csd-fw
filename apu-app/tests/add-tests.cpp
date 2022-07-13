#include <gtest/gtest.h>
#include <filesystem>
#include <regex>

#include "tensorflow/lite/interpreter.h"
#include "tensorflow/lite/kernels/register.h"
#include "tensorflow/lite/model.h"

#include "tflite-delegate.hpp"

#define NUM_MODELS 1

class VTAAddTest : public ::testing::TestWithParam<int>
{
    public:
        static const std::string modelspath;
        static std::vector<std::string> modelfiles;
        static void SetUpTestCase()
        {
            std::filesystem::path modelsdir = modelspath;
            std::regex fileregex("(^.*)\\/add-(\\d+).tflite");
            for (auto file : std::filesystem::directory_iterator(modelspath))
            {
                if (std::regex_match(file.path().string(), fileregex))
                {
                    modelfiles.push_back(file.path().string());
                }
            }
            ASSERT_EQ(NUM_MODELS, modelfiles.size()) << "Invalid number of declared models and present models in the " << modelspath << " directory" << std::endl;
        }
};

const std::string VTAAddTest::modelspath = "../tests/data";
std::vector<std::string> VTAAddTest::modelfiles;

TEST_P(VTAAddTest, AddTestSimple)
{
    std::unique_ptr<tflite::FlatBufferModel> model = tflite::FlatBufferModel::BuildFromFile(VTAAddTest::modelfiles[GetParam()].c_str());

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
}

INSTANTIATE_TEST_CASE_P(
    VTAAddTestGroup,
    VTAAddTest,
    ::testing::Range(0, NUM_MODELS)
);
