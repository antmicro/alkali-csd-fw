#include <gtest/gtest.h>
#include <filesystem>
#include <regex>

#include "tensorflow/lite/interpreter.h"
#include "tensorflow/lite/kernels/register.h"
#include "tensorflow/lite/model.h"

#include "vta-delegate.hpp"

#define NUM_MODELS 96

class VTAConv2DTest : public ::testing::TestWithParam<int>
{
    public:
        static const std::string modelspath;
        static std::vector<std::string> modelfiles;
        static void SetUpTestSuite()
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
        void SetUp()
        {
            std::cout << "Running test on " << modelfiles[GetParam()] << std::endl;
        }
};

const std::string VTAConv2DTest::modelspath = "../tests/data/conv2d";
std::vector<std::string> VTAConv2DTest::modelfiles;

TEST_P(VTAConv2DTest, AddTestSimple)
{
}

INSTANTIATE_TEST_SUITE_P(
    VTAConv2DTestGroup,
    VTAConv2DTest,
    ::testing::Range(0, NUM_MODELS)
);
