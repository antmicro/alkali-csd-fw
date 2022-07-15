/*
 * Copyright 2021-2022 Western Digital Corporation or its affiliates
 * Copyright 2021-2022 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vta-delegate.hpp"

#include "vta/vta_runtime.h"
#include "vta/hw_spec_const.h"

namespace tflite
{
bool VTADelegate::IsNodeSupportedByDelegate(
        const TfLiteRegistration *registration,
        const TfLiteNode *node,
        TfLiteContext *context) const
{
    // Only support add
    switch (registration->builtin_code)
    {
        case kTfLiteBuiltinAdd:
            break;
        default:
            printf("Skipped builtin code%d\n", registration->builtin_code);
            return false;
    }
    // Only support int8
    for (int i = 0; i < node->inputs->size; ++i)
    {
        auto &tensor = context->tensors[node->inputs->data[i]];
        if (tensor.type != kTfLiteInt8)
        {
            printf("Skipped tensor type %d for %d (%s,%d)\n",
                tensor.type,
                i,
                registration->custom_name == NULL ? "" : registration->custom_name,
                registration->builtin_code
            );
            return false;
        }
    }
    return true;
}

TfLiteStatus VTADelegate::Initialize(TfLiteContext *context)
{
    return kTfLiteOk;
}

const char *VTADelegate::Name() const
{
    static constexpr char kName[] = "VTADelegate";
    return kName;
}

std::unique_ptr<SimpleDelegateKernelInterface> VTADelegate::CreateDelegateKernelInterface()
{
    return std::make_unique<VTADelegateKernel>();
}

TfLiteStatus VTADelegateKernel::Init(TfLiteContext* context, const TfLiteDelegateParams* params)
{
    // Save index to all nodes which are part of this delegate.
    inputs_.resize(params->nodes_to_replace->size);
    outputs_.resize(params->nodes_to_replace->size);
    builtin_code_.resize(params->nodes_to_replace->size);
    for (int i = 0; i < params->nodes_to_replace->size; ++i)
    {
        const int node_index = params->nodes_to_replace->data[i];
        // Get this node information.
        TfLiteNode* delegated_node = nullptr;
        TfLiteRegistration* delegated_node_registration = nullptr;
        TF_LITE_ENSURE_EQ(
            context,
            context->GetNodeAndRegistration(context, node_index, &delegated_node,
                &delegated_node_registration),
                kTfLiteOk);
        inputs_[i].push_back(delegated_node->inputs->data[0]);
        inputs_[i].push_back(delegated_node->inputs->data[1]);
        outputs_[i].push_back(delegated_node->outputs->data[0]);
        builtin_code_[i] = delegated_node_registration->builtin_code;
    }
    return kTfLiteOk;
}

TfLiteStatus VTADelegateKernel::Prepare(TfLiteContext* context, TfLiteNode* node)
{
    return kTfLiteOk;
}

TfLiteStatus VTADelegateKernel::Eval(TfLiteContext* context, TfLiteNode* node)
{
    for (int i = 0; i < inputs_.size(); ++i)
    {
        auto& input_tensor_1 = context->tensors[inputs_[i][0]];
        auto& input_tensor_2 = context->tensors[inputs_[i][1]];
        auto& output_tensor = context->tensors[outputs_[i][0]];
        TF_LITE_ENSURE_EQ(
            context,
            ComputeResult(context, builtin_code_[i], &input_tensor_1,
                &input_tensor_2, &output_tensor),
            kTfLiteOk
        );
    }
    return kTfLiteOk;
}

TfLiteStatus VTADelegateKernel::ComputeResult(
        TfLiteContext* context, int builtin_code,
        const TfLiteTensor* input_tensor_1,
        const TfLiteTensor* input_tensor_2,
        TfLiteTensor* output_tensor)
{
    if (NumElements(input_tensor_1) != NumElements(input_tensor_2) ||
            NumElements(input_tensor_1) != NumElements(output_tensor))
    {
        return kTfLiteDelegateError;
    }
    auto* input_1 = GetTensorData<int8_t>(input_tensor_1);
    auto* input_2 = GetTensorData<int8_t>(input_tensor_2);
    auto* output = GetTensorData<int8_t>(output_tensor);
    size_t num = NumElements(input_tensor_1);

    // get VTA command handler
    auto cmd = VTATLSCommandHandle();
    // set debug for VTA
    // VTASetDebugMode(cmd, 1);

    // Create VTA buffers (they are host-target sync buffers)
    auto *vtainput1 = VTABufferAlloc(sizeof(int32_t) * num);
    auto *vtainput2 = VTABufferAlloc(sizeof(int32_t) * num);
    auto *vtaoutput = VTABufferAlloc(sizeof(int32_t) * num);

    int32_t *tmp_in_buf1 = (int32_t*)malloc(sizeof(int32_t) * num);
    int32_t *tmp_in_buf2 = (int32_t*)malloc(sizeof(int32_t) * num);

    for(int i = 0; i < num; i++) {
    tmp_in_buf1[i] = input_1[i];
    }

    for(int i = 0; i < num; i++) {
    tmp_in_buf2[i] = input_2[i];
    }

    // empty pointer to the UopKernelMap object required for forming micro-op kernel
    void *map = nullptr;

    // copy data to DRAM
    // from address, from offset, to address, to offset, data size and kind (VTA_MEMCPY_H2D means copy from host to DRAM)
    VTABufferCopy(tmp_in_buf1, 0, vtainput1, 0, sizeof(int32_t) * num, VTA_MEMCPY_H2D);
    VTABufferCopy(tmp_in_buf2, 0, vtainput2, 0, sizeof(int32_t) * num, VTA_MEMCPY_H2D);

    // schedule loading data to VTA
    // command handle, vtabuffer with source DRAM address, source offset, number of elements on X axis, number of elements on Y axis, X axis stride, start padding on X, start padding on Y, end padding on X, end padding on Y, destination SRAM index, memory type (VTA_MEM_ID_ACC_8BIT means 8-bit integers)
    VTALoadBuffer2D(cmd, vtainput1, 0, num, 1, num, 0, 0, 0, 0, 0, VTA_MEM_ID_ACC);
    VTALoadBuffer2D(cmd, vtainput2, 0, num, 1, num, 0, 0, 0, 0, num, VTA_MEM_ID_ACC);

    // wrap micro-op schedule functions in lambda function
    auto lambda = [num](void *x) -> int {
        // start micro op loop
        // the extent of the loop, the accum factor, the input factor, the weight factor
        VTAUopLoopBegin(num, 1, 1, 0);
        // run vector operation
        // mode (0 for GEMM, 1 for ALU), reset (1 if reset accum to 0), input memory index, weight memory index (not used here), ALU opcode, use_imm (1 if use immediate mode in ALU), imm_val (immediate value in ALU)
        VTAUopPush(1, 0, 0, num, 0, VTA_ALU_OPCODE_ADD, 0, 0);
        // end micro op loop
        VTAUopLoopEnd();
        return 0;
    };

    // create micro-op kernel for vector addition
    // UopKernelMap object (can be nullptr), op definition, text signature, length of signature
    VTAPushALUOp(
        &map,
        lambda,
        nullptr,
        0
    );

    // push dependency between command and getting outputs
    VTADepPush(cmd, 2, 3);

    // TODO set more appropriate timeout
    // run VTA computations (wait for 1000 cycles before timing out)
    VTASynchronize(cmd, 10000000);

    // pop the dependency
    VTADepPop(cmd, 2, 3);

    // Get the data from VTA
    // command, SRAM index, data type (VTA_MEM_ID_OUT means int8), destination DRAM address, destination offset, the size of the lowest dimension, the number of rows and x axis stride
    VTAStoreBuffer2D(cmd, 0, VTA_MEM_ID_OUT, vtaoutput, 0, num, 1, num);

    // TODO set more appropriate timeout
    // sync storing
    VTASynchronize(cmd, 10000000);

    // get data
    VTABufferCopy(vtaoutput, 0, output, 0, sizeof(int8_t) * num, VTA_MEMCPY_D2H);

    // Release VTA buffers
    VTABufferFree(vtainput1);
    VTABufferFree(vtainput2);
    VTABufferFree(vtaoutput);

    free(tmp_in_buf1);
    free(tmp_in_buf2);

    // Reset VTA
    VTARuntimeShutdown();

    return kTfLiteOk;
}

tflite::SimpleDelegateInterface::Options TfLiteVTADelegateOptionsDefault() {
        tflite::SimpleDelegateInterface::Options options = {0};
            return options;
}

TfLiteDelegate *TfLiteVTADelegateCreate(const SimpleDelegateInterface::Options *options) {
  std::unique_ptr<tflite::VTADelegate> custom(
      new tflite::VTADelegate(
          options ? *options : TfLiteVTADelegateOptionsDefault()));
  return tflite::TfLiteDelegateFactory::CreateSimpleDelegate(std::move(custom));
}

void TfLiteVTADelegateDelete(TfLiteDelegate* delegate) {
  tflite::TfLiteDelegateFactory::DeleteSimpleDelegate(delegate);
}

TfLiteDelegate *CreateVTADelegateFromOptions(char **options_keys, char **options_values, size_t num_options)
{
    return TfLiteVTADelegateCreate(NULL);
}

TfLiteDelegate *tflite_plugin_create_delegate(char** options_keys, char** options_values, size_t num_options, void (*report_error)(const char*))
{
    return tflite::CreateVTADelegateFromOptions(options_keys, options_values, num_options);
}

void tflite_plugin_destroy_delegate(TfLiteDelegate *delegate)
{
    tflite::TfLiteVTADelegateDelete(delegate);
}

};
