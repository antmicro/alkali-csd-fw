/*
 * Copyright 2021-2022 Western Digital Corporation or its affiliates
 * Copyright 2021-2022 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <filesystem>
#include <regex>
#include <tuple>
#include <algorithm>
#include <chrono>

#include "tensorflow/lite/interpreter.h"
#include "tensorflow/lite/kernels/register.h"
#include "tensorflow/lite/model.h"

#include "vta-delegate.hpp"

#include <spdlog/spdlog.h>
#include <spdlog/cfg/env.h>

#define NUM_MODELS 11

using std::chrono::milliseconds;

class VTAAddTest : public ::testing::TestWithParam<int>
{
    public:
        static const std::string modelspath;
        static std::vector<std::string> modelfiles;
        static std::unordered_map<std::string, std::vector<int8_t>> tfliteresults;
        static std::unordered_map<std::string, std::vector<int8_t>> vtaresults;
        static void SetUpTestSuite()
        {
            // spdlog::set_level(spdlog::level::debug);
            spdlog::cfg::load_env_levels();
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
            std::sort(modelfiles.begin(), modelfiles.end());
            spdlog::info("VTAAddTest suite ids:");
            for (unsigned int i = 0; i < modelfiles.size(); i++)
            {
                spdlog::info("{}:  {}", i, modelfiles[i]);
            }
            ASSERT_EQ(NUM_MODELS, modelfiles.size()) << "Invalid number of declared models and present models in the " << modelspath << " directory" << std::endl;
        }
        void SetUp()
        {
            spdlog::info("Running test on {}", modelfiles[GetParam()]);
        }
};

const std::string VTAAddTest::modelspath = "./test-models/add";
std::vector<std::string> VTAAddTest::modelfiles;
std::unordered_map<std::string, std::vector<int8_t>> VTAAddTest::tfliteresults;
std::unordered_map<std::string, std::vector<int8_t>> VTAAddTest::vtaresults;

TEST_P(VTAAddTest, AddTestTFLite)
{
    std::unique_ptr<tflite::FlatBufferModel> model = tflite::FlatBufferModel::BuildFromFile(VTAAddTest::modelfiles[GetParam()].c_str());

    tflite::ops::builtin::BuiltinOpResolver resolver;
    std::unique_ptr<tflite::Interpreter> interpreter;

    tflite::InterpreterBuilder(*model, resolver)(&interpreter);

    interpreter->AllocateTensors();

    int8_t *tfinput1 = interpreter->typed_input_tensor<int8_t>(0);
    int8_t *tfinput2 = interpreter->typed_input_tensor<int8_t>(1);

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
    // std::transform(input1.cbegin(), input1.cend(), input1.begin(), [](int8_t val) { return static_cast<int8_t>(rand() % 256 - 128); });
    // std::transform(input2.cbegin(), input2.cend(), input2.begin(), [](int8_t val) { return static_cast<int8_t>(rand() % 256 - 128); });
    std::transform(input1.cbegin(), input1.cend(), input1.begin(), [](int8_t val) { return 45; });
    std::transform(input2.cbegin(), input2.cend(), input2.begin(), [](int8_t val) { return 64; });

    std::copy(input1.begin(), input1.end(), tfinput1);
    std::copy(input2.begin(), input2.end(), tfinput2);

    auto t1 = std::chrono::high_resolution_clock::now();
    interpreter->Invoke();
    auto t2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> time = t2 - t1;
    spdlog::info("Processing time:  {} ms", time.count());

    int8_t *out = interpreter->typed_output_tensor<int8_t>(0);
    tfliteresults[modelfiles[GetParam()]].resize(input1.size(), 0);
    std::copy(&out[0], &out[input1.size()], tfliteresults[modelfiles[GetParam()]].begin());
}

TEST_P(VTAAddTest, AddTestDelegate)
{
    std::unique_ptr<tflite::FlatBufferModel> model = tflite::FlatBufferModel::BuildFromFile(VTAAddTest::modelfiles[GetParam()].c_str());

    tflite::ops::builtin::BuiltinOpResolver resolver;
    std::unique_ptr<tflite::Interpreter> interpreter;

    tflite::InterpreterBuilder(*model, resolver)(&interpreter);

    std::unique_ptr<TfLiteDelegate, decltype(&tflite::TfLiteVTADelegateDelete)> delegate(tflite::TfLiteVTADelegateCreate(NULL), &tflite::TfLiteVTADelegateDelete);

    interpreter->ModifyGraphWithDelegate(std::move(delegate));

    interpreter->AllocateTensors();

    int8_t *tfinput1 = interpreter->typed_input_tensor<int8_t>(0);
    int8_t *tfinput2 = interpreter->typed_input_tensor<int8_t>(1);

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
    // std::transform(input1.cbegin(), input1.cend(), input1.begin(), [](int8_t val) { return static_cast<int8_t>(rand() % 256 - 128); });
    // std::transform(input2.cbegin(), input2.cend(), input2.begin(), [](int8_t val) { return static_cast<int8_t>(rand() % 256 - 128); });
    std::transform(input1.cbegin(), input1.cend(), input1.begin(), [](int8_t val) { return 45; });
    std::transform(input2.cbegin(), input2.cend(), input2.begin(), [](int8_t val) { return 64; });

    std::copy(input1.begin(), input1.end(), tfinput1);
    std::copy(input2.begin(), input2.end(), tfinput2);

    auto t1 = std::chrono::high_resolution_clock::now();
    interpreter->Invoke();
    auto t2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> time = t2 - t1;
    spdlog::info("Processing time:  {} ms", time.count());

    int8_t *out = interpreter->typed_output_tensor<int8_t>(0);
    vtaresults[modelfiles[GetParam()]].resize(input1.size(), 0);
    std::copy(&out[0], &out[input1.size()], vtaresults[modelfiles[GetParam()]].begin());
}

TEST_P(VTAAddTest, DelegateCPUComparison)
{
    std::string modelpath = modelfiles[GetParam()];
    spdlog::debug("Comparing native and delegate results for {}", modelpath);

    ASSERT_NE(tfliteresults.find(modelpath), tfliteresults.end()) << "Missing native results for:  " << modelpath << std::endl;
    ASSERT_NE(vtaresults.find(modelpath), vtaresults.end()) << "Missing delegate results for:  " << modelpath << std::endl;
    for (int i = 0; i < tfliteresults[modelpath].size(); i++)
    {
        int8_t tfliteval = tfliteresults[modelpath][i];
        int8_t vtaval = vtaresults[modelpath][i];
        EXPECT_EQ(tfliteval, vtaval)
            << "  File=" << modelpath
            << "  Elem=" << i
            << "  TFLITE=" << static_cast<int>(tfliteval)
            << "  VTA=" << static_cast<int>(vtaval)
            << " are not equal" << std::endl;
    }
}

INSTANTIATE_TEST_SUITE_P(
    VTAAddTestGroup,
    VTAAddTest,
    ::testing::Range(0, NUM_MODELS)
);
