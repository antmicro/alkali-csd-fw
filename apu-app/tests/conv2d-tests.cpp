/*
 * Copyright 2021-2022 Western Digital Corporation or its affiliates
 * Copyright 2021-2022 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <filesystem>
#include <regex>
#include <chrono>

#include "tensorflow/lite/interpreter.h"
#include "tensorflow/lite/kernels/register.h"
#include "tensorflow/lite/model.h"
#include <spdlog/spdlog.h>
#include <spdlog/cfg/env.h>

#include "vta-delegate.hpp"

#define NUM_MODELS 97

class VTAConv2DTest : public ::testing::TestWithParam<int>
{
    public:
        static const std::string modelspath;
        static std::vector<std::string> modelfiles;
        static std::unordered_map<std::string, std::vector<int8_t>> tfliteresults;
        static std::unordered_map<std::string, std::vector<int8_t>> vtaresults;
        static void SetUpTestSuite()
        {
            spdlog::set_level(spdlog::level::debug);
            srand(12345);
            std::filesystem::path modelsdir = modelspath;
            std::regex fileregex("(^.*)\\/.*\\.tflite");
            for (auto file : std::filesystem::directory_iterator(modelspath))
            {
                if (std::regex_match(file.path().string(), fileregex))
                {
                    modelfiles.push_back(file.path().string());
                }
            }
            std::sort(modelfiles.begin(), modelfiles.end());
            spdlog::info("VTAConv2DTest suite ids:");
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

const std::string VTAConv2DTest::modelspath = "./test-models/conv2d";
std::vector<std::string> VTAConv2DTest::modelfiles;
std::unordered_map<std::string, std::vector<int8_t>> VTAConv2DTest::tfliteresults;
std::unordered_map<std::string, std::vector<int8_t>> VTAConv2DTest::vtaresults;

TEST_P(VTAConv2DTest, Conv2DTestTFLite)
{
    std::unique_ptr<tflite::FlatBufferModel> model = tflite::FlatBufferModel::BuildFromFile(VTAConv2DTest::modelfiles[GetParam()].c_str());

    tflite::ops::builtin::BuiltinOpResolver resolver;
    std::unique_ptr<tflite::Interpreter> interpreter;

    tflite::InterpreterBuilder(*model, resolver)(&interpreter);
    interpreter->AllocateTensors();

    int numdims = interpreter->input_tensor(0)->dims->size;

    int inputsize = 1;

    for (int i = 0; i < numdims; i++)
    {
        inputsize *= interpreter->input_tensor(0)->dims->data[i];
    }

    std::vector<int8_t> input1(inputsize);

    srand(GetParam());
    std::transform(input1.cbegin(), input1.cend(), input1.begin(), [](int8_t val) { return static_cast<int8_t>(rand() % 256 - 128); });

    int8_t *tfinput1 = interpreter->typed_input_tensor<int8_t>(0);

    std::copy(input1.cbegin(), input1.cend(), tfinput1);

    auto t1 = std::chrono::high_resolution_clock::now();
    interpreter->Invoke();
    auto t2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> time = t2 - t1;
    spdlog::info("Processing time:  {} ms", time.count());

    int outputsize = 1;
    for (int i = 0; i < interpreter->output_tensor(0)->dims->size; i++)
    {
        outputsize *= interpreter->output_tensor(0)->dims->data[i];
    }
    int8_t *out = interpreter->typed_output_tensor<int8_t>(0);
    tfliteresults[modelfiles[GetParam()]].resize(outputsize, 0);
    std::copy(&out[0], &out[outputsize], tfliteresults[modelfiles[GetParam()]].begin());
}

TEST_P(VTAConv2DTest, Conv2DTestDelegate)
{
    std::unique_ptr<tflite::FlatBufferModel> model = tflite::FlatBufferModel::BuildFromFile(VTAConv2DTest::modelfiles[GetParam()].c_str());

    tflite::ops::builtin::BuiltinOpResolver resolver;
    std::unique_ptr<tflite::Interpreter> interpreter;

    std::unique_ptr<TfLiteDelegate, decltype(&tflite::TfLiteVTADelegateDelete)> delegate(tflite::TfLiteVTADelegateCreate(NULL), &tflite::TfLiteVTADelegateDelete);

    tflite::InterpreterBuilder(*model, resolver)(&interpreter);

    interpreter->ModifyGraphWithDelegate(std::move(delegate));

    interpreter->AllocateTensors();

    int numdims = interpreter->input_tensor(0)->dims->size;

    int inputsize = 1;

    for (int i = 0; i < numdims; i++)
    {
        inputsize *= interpreter->input_tensor(0)->dims->data[i];
    }

    std::vector<int8_t> input1(inputsize);

    srand(GetParam());
    std::transform(input1.cbegin(), input1.cend(), input1.begin(), [](int8_t val) { return static_cast<int8_t>(rand() % 256 - 128); });

    int8_t *tfinput1 = interpreter->typed_input_tensor<int8_t>(0);

    std::copy(input1.cbegin(), input1.cend(), tfinput1);

    auto t1 = std::chrono::high_resolution_clock::now();
    interpreter->Invoke();
    auto t2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> time = t2 - t1;
    spdlog::info("Processing time:  {} ms", time.count());

    int outputsize = 1;
    for (int i = 0; i < interpreter->output_tensor(0)->dims->size; i++)
    {
        outputsize *= interpreter->output_tensor(0)->dims->data[i];
    }
    int8_t *out = interpreter->typed_output_tensor<int8_t>(0);
    vtaresults[modelfiles[GetParam()]].resize(outputsize, 0);
    std::copy(&out[0], &out[outputsize], vtaresults[modelfiles[GetParam()]].begin());
}

TEST_P(VTAConv2DTest, DelegateCPUComparison)
{
    std::string modelpath = modelfiles[GetParam()];
    spdlog::debug("Comparing native and delegate results for {}", modelpath);

    ASSERT_NE(tfliteresults.find(modelpath), tfliteresults.end()) << "Missing native results for:  " << modelpath << std::endl;
    ASSERT_NE(vtaresults.find(modelpath), vtaresults.end()) << "Missing delegate results for:  " << modelpath << std::endl;
    for (int i = 0; i < tfliteresults[modelpath].size(); i++)
    {
        int8_t tfliteval = tfliteresults[modelpath][i];
        int8_t vtaval = vtaresults[modelpath][i];
        EXPECT_NEAR(tfliteval, vtaval, 1)
            << "  File=" << modelpath
            << "  Elem=" << i
            << "  TFLITE=" << static_cast<int>(tfliteval)
            << "  VTA=" << static_cast<int>(vtaval)
            << " are not equal" << std::endl;
    }
}

INSTANTIATE_TEST_SUITE_P(
    VTAConv2DTestGroup,
    VTAConv2DTest,
    ::testing::Range(0, NUM_MODELS)
);
