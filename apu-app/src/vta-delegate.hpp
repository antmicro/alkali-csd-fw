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
typedef struct {
  // Allowed ops to delegate.
  int allowed_builtin_code;
  // Report error during init.
  bool error_during_init;
  // Report error during prepare.
  bool error_during_prepare;
  // Report error during invoke.
  bool error_during_invoke;
} CustomDelegateOptions;

class CustomDelegate : public SimpleDelegateInterface
{
public:
	explicit CustomDelegate(const CustomDelegateOptions &options) : options_(options) {}

	bool IsNodeSupportedByDelegate(const TfLiteRegistration *registration, const TfLiteNode *node, TfLiteContext *context) const;

	TfLiteStatus Initialize(TfLiteContext *context) override;

	const char *Name() const override;

	std::unique_ptr<SimpleDelegateKernelInterface> CreateDelegateKernelInterface() override;

	SimpleDelegateInterface::Options DelegateOptions() const {return options;};
private:
	const CustomDelegateOptions options_;
	const SimpleDelegateInterface::Options options;
};

class CustomDelegateKernel : public SimpleDelegateKernelInterface
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

CustomDelegateOptions TfLiteCustomDelegateOptionsDefault();

TfLiteDelegate *TfLiteCustomDelegateCreate(const CustomDelegateOptions *options);

void TfLiteCustomDelegateDelete(TfLiteDelegate *delegate);

TfLiteDelegate *CreateCustomDelegateFromOptions(char **options_keys, char **options_values, size_t num_options);
};

#ifdef __cplusplus
extern "C" {
#endif 

TFL_CAPI_EXPORT TfLiteDelegate *tflite_plugin_create_delegate(char** options_keys, char** options_values, size_t num_options, void (*report_error)(const char*));

TFL_CAPI_EXPORT void tflite_plugin_destroy_delegate(TfLiteDelegate *delegate);

#ifdef __cplusplus
}
#endif
