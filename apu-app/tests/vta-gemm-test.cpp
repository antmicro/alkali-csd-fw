/*
 * Copyright 2021-2022 Western Digital Corporation or its affiliates
 * Copyright 2021-2022 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <vta-delegate.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <spdlog/spdlog.h>

TEST(VTAGEMM, permuteDimsTest)
{
    tflite::VTAGEMMOp op(nullptr, nullptr, kTfLiteBuiltinConv2d, {0, 1, 2}, {3});
    op.setDim("No", 1);
    op.setDim("Ni", 4);
    op.setDim("Io", 4);
    op.setDim("Ii", 4);
    op.setDim("H", 4);
    op.setDim("W", 4);

    std::vector<std::string> inputlayout =  {"No", "Ni", "Io", "Ii", "H", "W"};
    std::vector<std::string> outputlayout = {"No", "Io", "H", "W", "Ni", "Ii"};

    ASSERT_EQ(op.getDimStep(inputlayout, "Io"), 4 * 4 * 4);

    int numelements = op.tensorElements(inputlayout);

    std::vector<uint8_t> input(numelements);
    std::vector<uint8_t> output(numelements);

    int i = 0;
    std::transform(input.cbegin(), input.cend(), input.begin(), [&i](uint8_t val){ return i++; });

    op.permuteDims(inputlayout, outputlayout, input.data(), output.data(), 1);

    spdlog::info("\n\n");
    spdlog::info("inp:  ");
    for (int i = 0; i < numelements; i++)
    {
        spdlog::info("{} ", input[i]);
    }
    spdlog::info("\n\n");
    spdlog::info("out:  ");
    for (int i = 0; i < numelements; i++)
    {
        spdlog::info("%d ", output[i]);
    }
    spdlog::info("\n\n");

    for (int No = 0; No < op.dim("No"); No++)
        for (int Ni = 0; Ni < op.dim("Ni"); Ni++)
            for (int Io = 0; Io < op.dim("Io"); Io++)
                for (int Ii = 0; Ii < op.dim("Ii"); Ii++)
                    for (int H = 0; H < op.dim("H"); H++)
                        for (int W = 0; W < op.dim("W"); W++)
                        {
                            uint8_t valinput = input[
                                No * op.getDimStep(inputlayout, "No") +
                                Ni * op.getDimStep(inputlayout, "Ni") +
                                Io * op.getDimStep(inputlayout, "Io") +
                                Ii * op.getDimStep(inputlayout, "Ii") +
                                H * op.getDimStep(inputlayout, "H") +
                                W * op.getDimStep(inputlayout, "W")
                            ];
                            uint8_t valoutput = output[
                                No * op.getDimStep(outputlayout, "No") +
                                Ni * op.getDimStep(outputlayout, "Ni") +
                                Io * op.getDimStep(outputlayout, "Io") +
                                Ii * op.getDimStep(outputlayout, "Ii") +
                                H * op.getDimStep(outputlayout, "H") +
                                W * op.getDimStep(outputlayout, "W")
                            ];
                            ASSERT_EQ(valinput, valoutput);
                        }
}
