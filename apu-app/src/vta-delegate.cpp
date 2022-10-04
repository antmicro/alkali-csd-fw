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

CommunicationContext::CommunicationContext()
{
    VTAStartCommunication();
}

CommunicationContext::~CommunicationContext()
{
    VTAEndCommunication();
}

bool VTADelegate::IsNodeSupportedByDelegate(
        const TfLiteRegistration *registration,
        const TfLiteNode *node,
        TfLiteContext *context) const
{
    switch (registration->builtin_code)
    {
        case kTfLiteBuiltinAdd:
        case kTfLiteBuiltinConv2d:
            break;
        default:
            printf("Skipped builtin code %d\n", registration->builtin_code);
            return false;
    }
    // We support only INT8-based operations
    for (int i = 0; i < node->inputs->size; ++i)
    {
        auto &tensor = context->tensors[node->inputs->data[i]];
        if (tensor.type != kTfLiteInt8 &&
            tensor.type != kTfLiteUInt8 &&
            tensor.type != kTfLiteInt32)
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

VTAOp::VTAOp(VTADelegateKernel *parent, std::string name, int tfliteop, std::vector<int> tfliteinputs, std::vector<int> tfliteoutputs) :
    parent(parent),
    name(name),
    tfliteop(tfliteop),
    inputs(tfliteinputs),
    outputs(tfliteoutputs)
{}

VTAOp::~VTAOp()
{}

TfLiteStatus VTADelegateKernel::Init(TfLiteContext* context, const TfLiteDelegateParams* params)
{
    if (commcontext == nullptr)
    {
        commcontext = std::make_shared<CommunicationContext>();
    }
    // NOTE During Init, only tensors with parameters are allocated by the TensorFlow Lite.
    // This means that tensors for inputs, outputs and intermediate data are not allocated.
    // During Invoke, additionaly non-intermediate inputs and outputs are allocated (tensors
    // that go into ant out from the delegate).

    // TODO perform optimization passes
    printf("Initializing delegate...\n");
    // Iterate over tensors in a group that were marked as supported by the delegate
    for (int i = 0; i < params->nodes_to_replace->size; i++)
    {
        // get node index, its definition and registration data (contains opcode)
        const int node_index = params->nodes_to_replace->data[i];
        TfLiteNode *node = nullptr;
        TfLiteRegistration *registerdata = nullptr;
        // this is TensorFlow Lite assert per context
        TF_LITE_ENSURE_EQ(
            context,
            context->GetNodeAndRegistration(
                context,
                node_index,
                &node,
                &registerdata
            ),
            kTfLiteOk
        );
        // store indices to the inputs and outputs for a given node (actual tensors are in context->tensors)
        std::vector<int> inputs;
        std::vector<int> outputs;
        for (int i = 0; i < node->inputs->size; i++)
        {
            inputs.push_back(node->inputs->data[i]);
        }
        for (int i = 0; i < node->outputs->size; i++)
        {
            outputs.push_back(node->outputs->data[i]);
        }
        // check the builtin code and create a proper VTA Op object
        switch (registerdata->builtin_code)
        {
            case kTfLiteBuiltinAdd:
                ops.push_back(
                    std::make_shared<VTAALUOp>(
                        this,
                        registerdata->builtin_code,
                        inputs,
                        outputs
                    )
                );
                break;
            case kTfLiteBuiltinConv2d:
                ops.push_back(
                    std::make_shared<VTAGEMMOp>(
                        this,
                        registerdata->builtin_code,
                        inputs,
                        outputs
                    )
                );
                break;
            default:
                printf("Unsupported builtin code %d\n", registerdata->builtin_code);
                return TfLiteStatus::kTfLiteUnresolvedOps;
        }
    }
    // print I/O summary for the delegated nodes
    printf("Context tensors\n");
    for (int i = 0; i < context->tensors_size; i++)
    {
        printf("%d:  %x\n", i, context->tensors[i].data);
    }
    for (auto &op: ops)
    {
        printf("Op:  %s number of inputs:  %lu number of outputs:  %lu\n", op->name.c_str(), op->inputs.size(), op->outputs.size());
        for (auto &inp: op->inputs)
        {
            printf("%d:(%x)[%d", inp, context->tensors[inp].data, context->tensors[inp].dims->data[0]);
            for (int dim = 1; dim < context->tensors[inp].dims->size; dim++)
            {
                printf("x%d", context->tensors[inp].dims->data[dim]);
            }
            printf("]  ");
        }
        printf("\n");
        for (auto &out: op->outputs)
        {
            printf("%d:(%x)[%d", out, context->tensors[out].data, context->tensors[out].dims->data[0]);
            for (int dim = 1; dim < context->tensors[out].dims->size; dim++)
            {
                printf("x%d", context->tensors[out].dims->data[dim]);
            }
            printf("]  ");
        }
        printf("\n");
    }
    return kTfLiteOk;
}

TfLiteStatus VTADelegateKernel::Prepare(TfLiteContext* context, TfLiteNode* node)
{
    // NOTE the Prepare function also have only parameters allocated, as in Init function.
    printf("Preparing...\n");
    printf("Context tensors\n");
    for (int i = 0; i < context->tensors_size; i++)
    {
        printf("%d:  %x\n", i, context->tensors[i].data);
    }
    // TODO implement optimization steps here?
    return kTfLiteOk;
}

TfLiteStatus VTADelegateKernel::Eval(TfLiteContext* context, TfLiteNode* node)
{
    // NOTE During Init, only tensors with parameters are allocated by the TensorFlow Lite.
    // This means that tensors for inputs, outputs and intermediate data are not allocated.
    // During Invoke, additionaly non-intermediate inputs and outputs are allocated (tensors
    // that go into ant out from the delegate).

    // NOTE In this function, "node" represents all delegated operations.
    // It means that all inputs for "node" are all inputs for all the delegated operations (their weights, biases, other parameters and inputs coming from the outer context).
    // Also, outputs for "node" are all outputs returned outside the delegate.

    printf("Invoking...\n");
    printf("Context tensors\n");
    for (int i = 0; i < context->tensors_size; i++)
    {
        printf("%d:  %x\n", i, context->tensors[i].data);
    }
    printf("Inputs:  ");
    for (int i = 0; i < node->inputs->size; i++)
    {
        int inp = node->inputs->data[i];
        printf("\n    %d:(%x)[%d", inp, context->tensors[inp].data, context->tensors[inp].dims->data[0]);
        for (int dim = 1; dim < context->tensors[inp].dims->size; dim++)
        {
            printf("x%d", context->tensors[inp].dims->data[dim]);
        }
        printf("]  ");
    }
    printf("\nOutputs:  ");
    for (int o = 0; o < node->outputs->size; o++)
    {
        int out = node->outputs->data[o];
        printf("\n    %d:(%x)[%d", out, context->tensors[out].data, context->tensors[out].dims->data[0]);
        for (int dim = 1; dim < context->tensors[out].dims->size; dim++)
        {
            printf("x%d", context->tensors[out].dims->data[dim]);
        }
        printf("]  ");
    }
    printf("\n");

    printf("Inferring...\n");

    this->context = context;

    for (auto &op: ops)
    {
        TfLiteStatus ret = op->compute();
        if (ret != TfLiteStatus::kTfLiteOk)
        {
            printf("Failed to compute operation:  %s", op->name.c_str());
            return ret;
        }
    }

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
