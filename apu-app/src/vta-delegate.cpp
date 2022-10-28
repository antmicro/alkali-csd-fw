/*
 * Copyright 2021-2022 Western Digital Corporation or its affiliates
 * Copyright 2021-2022 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vta-delegate.hpp"

#include "vta/vta_runtime.h"
#include <cassert>
#include "vta/hw_spec_const.h"
#include "tensorflow/lite/kernels/internal/common.h"
#include <limits>
#include <spdlog/spdlog.h>
#include <algorithm>

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

int32_t simulateRealValue(int8_t val, QuantizationData qdata)
{
    const int16_t valoffset = static_cast<int16_t>(qdata.offset) + static_cast<int16_t>(val); // max 7 bits required
    const int16_t valshift = valoffset * (static_cast<int16_t>(1) << qdata.left_shift); // ~15 bits required?
    bool overflow = valshift == qdata.multiplier && valshift == std::numeric_limits<int16_t>::min();
    const int32_t valscaledraw32 = valshift * qdata.multiplier;
    const int32_t nudge = valscaledraw32 >= 0 ? (1 << 14) : (1 - (1 << 14));
    const int32_t valscaled = (valscaledraw32 + nudge) >> 15;
    const int16_t valsaturated = overflow ? std::numeric_limits<int16_t>::max() : valscaled;
    // TODO introduce rounding?
    const int16_t finval = valsaturated >> -qdata.shift;
    return finval;
}

int8_t requantizeResults(int32_t val, QuantizationData qdata)
{
    const int32_t valscaled = static_cast<int32_t>(val) * qdata.multiplier;
    const int32_t valshifted = valscaled >> (15 - qdata.shift);
    const int32_t valoffset = valshifted + qdata.offset;
    // TODO compute clamping from previous parameters?
    using range = std::numeric_limits<int8_t>;
    const int32_t valclamped = std::max(static_cast<int32_t>(range::min()), std::min(valoffset, static_cast<int32_t>(range::max())));
    return static_cast<int8_t>(valclamped);
}

void computeQuantizationParameters(double scale, int16_t &multiplier, int16_t &shift)
{
    multiplier = 0;
    shift = 0;
    if (scale < 1e-6)
    {
        // TODO should this value be updated?
        return;
    }
    int32_t shift32;
    const double q = frexp(scale, &shift32);
    auto q_fixed = static_cast<int64_t>(round(q * (1 << 15)));
    assert(q_fixed <= (1 << 15));
    if (q_fixed == (1 << 15)) {
        q_fixed /= 2;
        ++shift32;
    }
    assert(q_fixed <= std::numeric_limits<int16_t>::max());
    if (shift32 < -15)
    {
        shift32 = 0;
        q_fixed = 0;
    }
    assert(abs(shift) <= std::numeric_limits<int16_t>::max());
    multiplier = static_cast<int16_t>(q_fixed);
    shift = static_cast<int16_t>(shift32);
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
            spdlog::warn("Skipped builtin code {}", registration->builtin_code);
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
            spdlog::warn("Skipped tensor type {} for {} ({},{})",
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

VTAOp::VTAOp(VTADelegateKernel *parent, TfLiteNode *node, std::string name, int tfliteop, std::vector<int> tfliteinputs, std::vector<int> tfliteoutputs) :
    parent(parent),
    node(node),
    name(name),
    tfliteop(tfliteop),
    inputs(tfliteinputs),
    outputs(tfliteoutputs)
{}

VTAOp::~VTAOp()
{}

TfLiteStatus VTADelegateKernel::Init(TfLiteContext* context, const TfLiteDelegateParams* params)
{
    this->context = context;
    if (commcontext == nullptr)
    {
        commcontext = std::make_shared<CommunicationContext>();
    }
    // NOTE During Init, only tensors with parameters are allocated by the TensorFlow Lite.
    // This means that tensors for inputs, outputs and intermediate data are not allocated.
    // During Invoke, additionaly non-intermediate inputs and outputs are allocated (tensors
    // that go into ant out from the delegate).

    // TODO perform optimization passes
    spdlog::info("Initializing delegate...");
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
                        node,
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
                        node,
                        registerdata->builtin_code,
                        inputs,
                        outputs
                    )
                );
                break;
            default:
                spdlog::warn("Unsupported builtin code {}", registerdata->builtin_code);
                return TfLiteStatus::kTfLiteUnresolvedOps;
        }
    }
    // print I/O summary for the delegated nodes
    spdlog::debug("Context tensors");
    for (int i = 0; i < context->tensors_size; i++)
    {
        spdlog::debug("{}:  {}", i, fmt::ptr(context->tensors[i].data.int8));
    }
    for (auto &op: ops)
    {
        spdlog::debug("Op:  {} number of inputs:  {} number of outputs:  {}", op->name.c_str(), op->inputs.size(), op->outputs.size());
        for (auto &inp: op->inputs)
        {
            std::string dat = "";
            dat += fmt::format("{}:({})[{}", inp, fmt::ptr(context->tensors[inp].data.int8), context->tensors[inp].dims->data[0]);
            for (int dim = 1; dim < context->tensors[inp].dims->size; dim++)
            {
                dat += fmt::format("x{}", context->tensors[inp].dims->data[dim]);
            }
            spdlog::debug("{}]  ", dat);
        }
        for (auto &out: op->outputs)
        {
            std::string dat = "";
            dat += fmt::format("{}:({})[{}", out, fmt::ptr(context->tensors[out].data.int8), context->tensors[out].dims->data[0]);
            for (int dim = 1; dim < context->tensors[out].dims->size; dim++)
            {
                dat += fmt::format("x{}", context->tensors[out].dims->data[dim]);
            }
            spdlog::debug("{}]  ", dat);
        }
    }
    spdlog::info("Delegate initialized");
    return kTfLiteOk;
}

TfLiteStatus VTADelegateKernel::Prepare(TfLiteContext* context, TfLiteNode* node)
{
    // NOTE the Prepare function also have only parameters allocated, as in Init function.
    spdlog::debug("Preparing kernel...");
    spdlog::debug("Context tensors");
    for (int i = 0; i < context->tensors_size; i++)
    {
        spdlog::debug("{}:  {}", i, fmt::ptr(context->tensors[i].data.int8));
    }
    spdlog::debug("Kernel prepared");
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

    spdlog::debug("Invoking...");
    this->context = context;
    spdlog::debug("Context tensors");
    for (int i = 0; i < context->tensors_size; i++)
    {
        spdlog::debug("{}:  {}", i, fmt::ptr(context->tensors[i].data.int8));
    }
    spdlog::debug("Inputs:  ");
    for (int i = 0; i < node->inputs->size; i++)
    {
        int inp = node->inputs->data[i];
        std::string dat = "";
        dat += fmt::format("    {}:({})[{}", inp, fmt::ptr(context->tensors[inp].data.int8), context->tensors[inp].dims->data[0]);
        for (int dim = 1; dim < context->tensors[inp].dims->size; dim++)
        {
            dat += fmt::format("x{}", context->tensors[inp].dims->data[dim]);
        }
        spdlog::debug("{}]  ", dat);
    }
    spdlog::debug("Outputs:  ");
    for (int o = 0; o < node->outputs->size; o++)
    {
        int out = node->outputs->data[o];
        std::string dat = "";
        dat += fmt::format("    {}:({})[{}", out, fmt::ptr(context->tensors[out].data.int8), context->tensors[out].dims->data[0]);
        for (int dim = 1; dim < context->tensors[out].dims->size; dim++)
        {
            dat += fmt::format("x{}", context->tensors[out].dims->data[dim]);
        }
        spdlog::debug("{}]", dat);
    }

    spdlog::debug("Inferring...");

    for (auto &op: ops)
    {
        TfLiteStatus ret = op->compute();
        if (ret != TfLiteStatus::kTfLiteOk)
        {
            spdlog::error("Failed to compute operation:  {}", op->name.c_str());
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
