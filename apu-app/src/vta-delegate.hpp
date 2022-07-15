/*
 * Copyright 2021-2022 Western Digital Corporation or its affiliates
 * Copyright 2021-2022 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <vector>
#include "tensorflow/lite/delegates/utils/simple_delegate.h"
#include "tensorflow/lite/builtin_ops.h"

#include "tensorflow/lite/kernels/internal/tensor_ctypes.h"
#include "tensorflow/lite/kernels/kernel_util.h"
#include "tensorflow/lite/tools/delegates/delegate_provider.h"
#include "tensorflow/lite/tools/tool_params.h"

using namespace tflite::tools;

namespace tflite
{

/**
 * A TensorFlow Lite delegate for VTA accelerator.
 */
class VTADelegate : public SimpleDelegateInterface
{
public:
    explicit VTADelegate(const SimpleDelegateInterface::Options &options) : options(options) {}

    bool IsNodeSupportedByDelegate(const TfLiteRegistration *registration, const TfLiteNode *node, TfLiteContext *context) const override;

    TfLiteStatus Initialize(TfLiteContext *context) override;

    const char *Name() const override;

    std::unique_ptr<SimpleDelegateKernelInterface> CreateDelegateKernelInterface() override;

    SimpleDelegateInterface::Options DelegateOptions() const override {return options;};
private:
    const SimpleDelegateInterface::Options options;
};

class VTADelegateKernel : public SimpleDelegateKernelInterface
{
public:
    TfLiteStatus Init(TfLiteContext* context, const TfLiteDelegateParams* params) override;

    TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) override;

    TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) override;
private:
    TfLiteStatus ComputeResult(
            TfLiteContext* context, int builtin_code,
            const TfLiteTensor* input_tensor_1,
            const TfLiteTensor* input_tensor_2,
            TfLiteTensor* output_tensor);

    std::vector<std::vector<int>> inputs_, outputs_;
    std::vector<int> builtin_code_;
};

tflite::SimpleDelegateInterface::Options TfLiteVTADelegateOptionsDefault();

TfLiteDelegate *TfLiteVTADelegateCreate(const SimpleDelegateInterface::Options *options);

void TfLiteVTADelegateDelete(TfLiteDelegate *delegate);

TfLiteDelegate *CreateVTADelegateFromOptions(char **options_keys, char **options_values, size_t num_options);
};

#ifdef __cplusplus
extern "C" {
#endif

TFL_CAPI_EXPORT TfLiteDelegate *tflite_plugin_create_delegate(char** options_keys, char** options_values, size_t num_options, void (*report_error)(const char*));

TFL_CAPI_EXPORT void tflite_plugin_destroy_delegate(TfLiteDelegate *delegate);

#ifdef __cplusplus
}
#endif
