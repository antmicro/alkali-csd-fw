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
#include "vta/hw_spec_const.h"
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
 *
 * This class is responsible for verifying the compatibility of individual nodes
 * with a given accelerated implementations. Based on the delegate capabilities,
 * the delegate forms a VTADelegateKernel that implements operations supported
 * by the VTA delegate.
 */
class VTADelegate : public SimpleDelegateInterface
{
public:
    /**
     * Constructor for the delegate.
     *
     * Options available in SimpleDelegateInterface::Options are:
     *
     * * max_delegated_partitions - maximum number of delegated subgraph, <=0 means unlimited
     * * min_nodes_per_partition - the minimum number of nodes allowed in a delegated graph, values <=0 means unlimited.
     *
     * @param options delegate options
     */
    explicit VTADelegate(const SimpleDelegateInterface::Options &options) : options(options) {}

    /**
     * Checks if a given node is supported by the delegate.
     *
     * @param registration node registration structure, with builtin_code (or custom_name) and version of the op
     * @param node represents node to support. It contains such data as indices for inputs, outputs, intermediate and temporary tensors (indices to tensors in context->tensors array)
     * @param context a TFLite context containing list and count of tensors in the model, execution plan and methods for getting/manipulating tensors
     * @return true if node is supported by the delegate, false if not
     */
    bool IsNodeSupportedByDelegate(const TfLiteRegistration *registration, const TfLiteNode *node, TfLiteContext *context) const override;

    /**
     * Initializes the delegate before finding and replacing TFLite nodes with delegate kernels.
     *
     * It can be used for setting up the accelerator, or for retrieving some TFLite settings from the context.
     *
     * @param context context containing TFLite tensors
     * @return the initialization status
     */
    TfLiteStatus Initialize(TfLiteContext *context) override;

    /**
     * Returns the name of the delegate.
     *
     * @return name of the delegate
     */
    const char *Name() const override;

    /**
     * Creates a delegate kernel, providing an interface to it.
     *
     * @return unique pointer to the interface for the kernel delegate
     */
    std::unique_ptr<SimpleDelegateKernelInterface> CreateDelegateKernelInterface() override;

    /**
     * Getter for the delegate options.
     *
     * @return default options for the delegate
     */
    SimpleDelegateInterface::Options DelegateOptions() const override {return options;};
private:
    const SimpleDelegateInterface::Options options; ///< delegate's options
};

/**
 * Represents implementation of TFLite operation in VTA accelerator.
 */
class VTAOp
{
    public:
        VTAOp(int tfliteop, std::vector<int> tfliteinputs, std::vector<int> tfliteoutputs);

        int tfliteop = 0; ///< builtin code for the TFLite operation corresponding to this op

        std::vector<int> inputs; ///< indices to vector of inputs (and weights) from the context
        std::vector<int> outputs; ///< indices to vector of outputs

        /**
         * Provides VTA commands for executing the given operation.
         */
        virtual void getComputeOps() = 0;

        virtual ~VTAOp() = 0;
};

/**
 * Wrapper for ALU operations on VTA.
 */
class VTAALUOp : public VTAOp
{
    public:
        VTAALUOp(int tfliteop, std::vector<int> tfliteinputs, std::vector<int> tfliteoutputs);

        int opcode = VTA_ALU_OPCODE_ADD;
        void getComputeOps() override;
        ~VTAALUOp();
};

/**
 * Wrapper for GEMM operations on VTA.
 */
class VTAGEMMOp : public VTAOp
{
    public:
        VTAGEMMOp(int tfliteop, std::vector<int> tfliteinputs, std::vector<int> tfliteoutputs);
        void getComputeOps() override;
        ~VTAGEMMOp();
};

/**
 * A TensorFlow Lite Delegate Kernel for VTA accelerator.
 *
 * This class implements the operations running on the VTA accelerator.
 * It runs a delegated portion of the model.
 */
class VTADelegateKernel : public SimpleDelegateKernelInterface
{
public:
    /**
     * Initializes the delegated subgraph.
     *
     * @param context TFLite context containing tensors
     * @param params parameters of the delegation, i.e. nodes to replace, input and output tensors for the delegated block.
     */
    TfLiteStatus Init(TfLiteContext* context, const TfLiteDelegateParams* params) override;

    /**
     * Prepares the flow of the computation for the VTA delegate.
     *
     * @param context TFLite context
     * @param node node to prepare for
     *
     * @return status of preparation
     */
    TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) override;

    /**
     * Evaluates the node using the VTA delegate.
     *
     * Uploads data to the VTA, computes the micro-op kernel and
     * downloads the result.
     *
     * @param context TFLite context
     * @param node node to compute
     */
    TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) override;
private:
    TfLiteStatus ComputeResult(
            TfLiteContext* context, int builtin_code,
            const TfLiteTensor* input_tensor_1,
            const TfLiteTensor* input_tensor_2,
            TfLiteTensor* output_tensor);

    std::vector<std::shared_ptr<VTAOp>> ops;

    std::vector<std::vector<int>> inputs_, outputs_;
    std::vector<int> builtin_code_;
};

/**
 * Returns default VTA delegate options.
 */
tflite::SimpleDelegateInterface::Options TfLiteVTADelegateOptionsDefault();

/**
 * Creates a VTA delegate.
 *
 * @param options settings for the delegate
 * @return pointer to the created delegate
 */
TfLiteDelegate *TfLiteVTADelegateCreate(const SimpleDelegateInterface::Options *options);

/**
 * Deletes a VTA delegate.
 *
 * @param delegate delegate to remove
 */
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
