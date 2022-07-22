#include <gtest/gtest.h>
#include <filesystem>
#include <regex>
#include <tuple>
#include <algorithm>

#include "tensorflow/lite/interpreter.h"
#include "tensorflow/lite/kernels/register.h"
#include "tensorflow/lite/model.h"

#include "vta-delegate.hpp"

#define NUM_MODELS 8

class VTAAddTest : public ::testing::TestWithParam<int>
{
    public:
        static const std::string modelspath;
        static std::vector<std::string> modelfiles;
        static void SetUpTestSuite()
        {
            srand(12345);
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
        void SetUp()
        {
            std::cout << "Running test on " << modelfiles[GetParam()] << std::endl;
        }
};

const std::string VTAAddTest::modelspath = "../tests/data/add";
std::vector<std::string> VTAAddTest::modelfiles;

TEST_P(VTAAddTest, AddTestTFLite)
{
    std::unique_ptr<tflite::FlatBufferModel> model = tflite::FlatBufferModel::BuildFromFile(VTAAddTest::modelfiles[GetParam()].c_str());

    tflite::ops::builtin::BuiltinOpResolver resolver;
    std::unique_ptr<tflite::Interpreter> interpreter;

    tflite::InterpreterBuilder(*model, resolver)(&interpreter);
    interpreter->AllocateTensors();

    // int8_t *input1 = interpreter->typed_input_tensor<int8_t>(0);
    // int8_t *input2 = interpreter->typed_input_tensor<int8_t>(1);

    ASSERT_EQ(interpreter->input_tensor(0)->dims->size, interpreter->input_tensor(1)->dims->size) << "Input tensors differ in dimensionality";

    int numdims = interpreter->input_tensor(0)->dims->size;

    int inputsize = 1;

    for (int i = 0; i < numdims; i++)
    {
        ASSERT_EQ(interpreter->input_tensor(0)->dims->data[i], interpreter->input_tensor(1)->dims->data[i]) << "Input tensors differ in " << i << "th dimension";
        inputsize *= interpreter->input_tensor(0)->dims->data[i];
    }

    std::vector<int8_t> input1(interpreter->typed_input_tensor<int8_t>(0), interpreter->typed_input_tensor<int8_t>(0) + inputsize);
    std::vector<int8_t> input2(interpreter->typed_input_tensor<int8_t>(0), interpreter->typed_input_tensor<int8_t>(0) + inputsize);

    std::transform(input1.cbegin(), input1.cend(), input1.begin(), [](int8_t val) { return static_cast<int8_t>(rand() % 256 - 128); });
    std::transform(input2.cbegin(), input2.cend(), input2.begin(), [](int8_t val) { return static_cast<int8_t>(rand() % 256 - 128); });

    interpreter->Invoke();
}

// TEST_P(VTAAddTest, AddTestDelegate)
// {
//     std::unique_ptr<tflite::Interpreter> interpreter = createInterpreter(VTAAddTest::modelfiles[GetParam()]);

//     interpreter->AllocateTensors();

//     // int8_t *input1 = interpreter->typed_input_tensor<int8_t>(0);
//     // int8_t *input2 = interpreter->typed_input_tensor<int8_t>(1);

//     ASSERT_EQ(interpreter->input_tensor(0)->dims->size, interpreter->input_tensor(1)->dims->size) << "Input tensors differ in dimensionality";

//     int numdims = interpreter->input_tensor(0)->dims->size;

//     int inputsize = 1;

//     for (int i = 0; i < numdims; i++)
//     {
//         ASSERT_EQ(interpreter->input_tensor(0)->dims->data[i], interpreter->input_tensor(1)->dims->data[i]) << "Input tensors differ in " << i << "th dimension";
//         inputsize *= interpreter->input_tensor(0)->dims->data[i];
//     }

//     std::vector<int8_t> input1(interpreter->typed_input_tensor<int8_t>(0), interpreter->typed_input_tensor<int8_t>(0) + inputsize);
//     std::vector<int8_t> input2(interpreter->typed_input_tensor<int8_t>(0), interpreter->typed_input_tensor<int8_t>(0) + inputsize);

//     std::transform(input1.cbegin(), input1.cend(), input1.begin(), [](int8_t val) { return static_cast<int8_t>(rand() % 256 - 128); });
//     std::transform(input2.cbegin(), input2.cend(), input2.begin(), [](int8_t val) { return static_cast<int8_t>(rand() % 256 - 128); });

//     interpreter->Invoke();

//     std::unique_ptr<tflite::Interpreter> vtainterpreter = createInterpreter(VTAAddTest::modelfiles[GetParam()]);

//     std::unique_ptr<TfLiteDelegate, decltype(&tflite::TfLiteVTADelegateDelete)> delegate(tflite::TfLiteVTADelegateCreate(NULL), &tflite::TfLiteVTADelegateDelete);

//     vtainterpreter->ModifyGraphWithDelegate(std::move(delegate));

//     vtainterpreter->AllocateTensors();

//     int8_t *input1 = vtainterpreter->typed_input_tensor<int8_t>(0);
//     int8_t *input2 = vtainterpreter->typed_input_tensor<int8_t>(1);

//     vtainterpreter->Invoke();

//     int8_t *out = vtainterpreter->typed_output_tensor<int8_t>(0);

//     printf("output: %d %d\n", out[0], out[1]);
// }

INSTANTIATE_TEST_SUITE_P(
    VTAAddTestGroup,
    VTAAddTest,
    ::testing::Range(0, NUM_MODELS)
);
