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
#include <spdlog/cfg/env.h>

using namespace tflite::tools;

namespace tflite
{

/**
 * A class for holding the communication context during VTA usage.
 * It initializes communication context upon construction,
 * and cleans communication context upon destruction (i.e. CMA).
 */
class CommunicationContext
{
    public:
        /**
         * Initializes VTA communication context.
         */
        CommunicationContext();
        /**
         * Cleans VTA communication context.
         */
        ~CommunicationContext();
};

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
    explicit VTADelegate(const SimpleDelegateInterface::Options &options) : options(options)
    {
        spdlog::cfg::load_env_levels();
    }

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

class VTAOp;

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

    TfLiteContext *context = nullptr;
private:
    std::vector<std::shared_ptr<VTAOp>> ops;
    inline static std::shared_ptr<CommunicationContext> commcontext = nullptr;
};

/**
 * Represents implementation of TFLite operation in VTA accelerator.
 */
class VTAOp
{
    public:
        /**
         * Base constructor for VTAOp.
         *
         * It holds indexes to inputs and outputs of the operator, its TFLite opcode and the parent delegate kernel.
         *
         * @param parent pointer to the owning VTADelegateKernel
         * @param name name of the operator
         * @param tfliteop opcode for the TFLite operation
         * @param tfliteinputs vector of indexes to tensors with input data for the operator (tensors are in TfLiteContext)
         * @param tfliteoutputs vector of indexes to tensors with output data for the operator
         */
        VTAOp(VTADelegateKernel *parent, TfLiteNode* node, std::string name, int tfliteop, std::vector<int> tfliteinputs, std::vector<int> tfliteoutputs);

        VTADelegateKernel *parent = nullptr; ///< owning VTADelegateKernel
        TfLiteNode *node = nullptr; ///< TFLite node implemented by this VTAOp

        std::string name; ///< user-friendly name of the operator/subnode
        int tfliteop = 0; ///< builtin code for the TFLite operation corresponding to this op

        std::vector<int> inputs; ///< indices to vector of inputs (and weights) from the context
        std::vector<int> outputs; ///< indices to vector of outputs

        /**
         * Provides VTA commands for executing the given operation.
         *
         * @return status of execution
         */
        virtual TfLiteStatus compute() = 0;

        /**
         * Virtual abstract destructor.
         */
        virtual ~VTAOp() = 0;
};

/**
 * Computes multiplier and shift from double-precision value.
 *
 * The value is represented in format:
 *
 *    scale = multiplier * 2 ^ (-shift)
 *
 * where multiplier is value from range [0.5;1.0), multiplied by 2**15
 * so it can be expressed as integer.
 *
 * @param scale the original, double-precision scale
 * @param multiplier 16-bit signed integer representing multiplier (values ranging 0.5-1.0 * 2 ** 15)
 * @param shift shift of fixed-point representation
 */
void computeQuantizationParameters(double scale, int16_t &multiplier, int16_t &shift);

/**
 * Struct for holding ALU quantizer/requantizer data.
 *
 * Data requires quantization/requantization after each operation.
 * For this we need scale and zero point, as in any quantization.
 *
 * But, since we operate on INT8/INT32 values, those parameters are
 * stored in a following format:
 *
 * * offset - zero point (negated for inputs)
 * * multiplier and shift - the floating-point scales are represented using fixed-point multiplers in INT32 notation:
     M = 2 ^ (-shift) * multiplier
 */
struct QuantizationData
{
    int32_t offset = 0; ///< quantization offset

    /// 2**20 for INT8 values for 32-64 bit requantization,
    /// experimentally here is 7, which should fit into 16-bit
    static const int16_t left_shift = 7;
    int16_t shift = 0; ///< scale's shift (fixed-point representation)
    int16_t multiplier = 0; ///< scale's 32-bit multiplier (fixed-point representation)
};

/**
 * Performs conversion of quantized value to "real" value.
 *
 * Even in integer arithmetic, some of the operations are
 * done in 32-bit integers, "simulating" the real values
 * using fixed-point representation.
 *
 * This function converts INT8 values to INT32 values by
 * applying offset, and scaling the value using the
 * multiplier and shift stored in the QuantizationData.
 *
 * @param val single value to convert
 * @param qdata QuantizationData used to properly scale the values
 * @return 32-bit "real" value
 */
int32_t simulateRealValue(int8_t val, QuantizationData qdata);

/**
 * Requantizes the data after operation back to INT8 representation.
 *
 * The intermediate op results may not be able to fit into INT8 type,
 * most often they are stored int 32-bit variables.
 * They need to be quantized back to INT8 after the computations finish.
 *
 * @param val 32-bit data, i.e. from accumulator
 * @param qdata quantization data used for converting value to INT8
 * @return 8-bit requantized value
 */
int8_t requantizeResults(int32_t val, QuantizationData qdata);

/**
 * Wrapper for ALU operations on VTA.
 */
class VTAALUOp : public VTAOp
{
    public:
        /**
         * Constructor for ALU operations in VTA.
         *
         * @param parent pointer to the owning VTADelegateKernel
         * @param tfliteop opcode for the TFLite operation
         * @param tfliteinputs vector of indexes to tensors with input data for the operator (tensors are in TfLiteContext)
         * @param tfliteoutputs vector of indexes to tensors with output data for the operator
         */
        VTAALUOp(VTADelegateKernel *parent, TfLiteNode* node, int tfliteop, std::vector<int> tfliteinputs, std::vector<int> tfliteoutputs);

        int opcode = VTA_ALU_OPCODE_ADD; ///< VTA opcode, used in ALU instruction
        TfLiteStatus compute() override;
        ~VTAALUOp();

    private:
        /**
         * Performs ALU ADD operation
         */
        TfLiteStatus aluAdd();

        QuantizationData input1quant; ///< input 1 quantization parameters
        QuantizationData input2quant; ///< input 2 quantization parameters
        QuantizationData outputquant; ///< output quantization parameters
};

/**
 * Wrapper for GEMM operations on VTA.
 */
class VTAGEMMOp : public VTAOp
{
    public:
        /**
         * Constructor for GEMM operations in VTA.
         *
         * @param parent pointer to the owning VTADelegateKernel
         * @param tfliteop opcode for the TFLite operation
         * @param tfliteinputs vector of indexes to tensors with input data for the operator (tensors are in TfLiteContext)
         * @param tfliteoutputs vector of indexes to tensors with output data for the operator
         */
        VTAGEMMOp(VTADelegateKernel *parent, TfLiteNode* node, int tfliteop, std::vector<int> tfliteinputs, std::vector<int> tfliteoutputs);
        TfLiteStatus compute() override;
        ~VTAGEMMOp();

        /**
         * Resets the dim map.
         */
        void resetDims();

        /**
         * Sets the dimension of given name to a given value.
         *
         * @param dimname name of the dimension
         * @param dimsize size of the dimension
         */
        void setDim(std::string dimname, int dimsize);

        /**
         * Computes step for particular axis in a given layout.
         *
         * @param layout array of strings representing layout of the array (dimensions are stored in dims)
         * @param axis axis for which to compute the step
         */
        int getDimStep(const std::vector<std::string> &layout, const std::string &axis);

        /**
         * Returns size of given dimension.
         *
         * @param dimname name of dimension
         */
        int dim(std::string dimname);

        /**
         * Permutes dimensions based on given layouts.
         *
         * It maps the data in inplayout to be stored
         * as in outlayout.
         *
         * If actualdims are provided, the dimensions
         * where the actual dimension size is smaller
         * than required in outlayout, the data is
         * zero-padded.
         *
         * @param inplayout input layout
         * @param outlayout output layout
         * @param inparray input array
         * @param outarray output array
         * @param elemsize size of a single element in array
         */
        void permuteDims(
            const std::vector<std::string> &inplayout,
            const std::vector<std::string> &outlayout,
            uint8_t *inparray,
            uint8_t *outarray,
            const size_t elemsize
        );

        /**
         * Zero-pads the tensor based on given layouts.
         *
         * @param srclayout input layout
         * @param dstlayout output layout
         * @param inparray input array
         * @param outarray output array
         * @param elemsize size of a single element
         */
         void padData(
             const std::vector<std::string> &srclayout,
             const std::vector<std::string> &dstlayout,
             uint8_t *inparray,
             uint8_t *outarray,
             const size_t elemsize
         );

        /**
         * Prints configured dimensions.
         */
        void printDims();

        /**
         * Returns number of elements for tensor with given layout.
         * @param layout layout of the tensor
         * @return size of the tensor in elements
         */
        int tensorElements(const std::vector<std::string> &layout);
    private:
        /**
         * Performs 2D convolution.
         */
        TfLiteStatus gemmConv2D();

        /**
         * Map holding dimensions for GEMM data
         * CONV2D:
         *     Input, weights and bias dimensions
         *         N - batch size
         *         H - input height
         *         W - input width
         *         I - input channels
         *
         *         Ho - output height
         *         Wo - output width
         *         O - output channels
         *
         *         Hk - kernel height
         *         Wk - kernel width
         *     Computed dimensions
         *         No - outer batch size (N / VTA_BATCH)
         *         Io - outer input channels (I / VTA_BLOCK_IN)
         *         Oo - outer output channels (O / VTA_BLOCK_OUT)
         *     Walking dimensions
         *         paddingH - height padding
         *         paddingW - width padding
         *         strideH - stride along height axis
         *         strideW - stride along width axis
         */
        std::unordered_map<std::string, int> dims;

        QuantizationData inputquant; ///< stores input quantization data, here only offset
        std::vector<QuantizationData> filtersquant; ///< stores multipliers per output channel
        std::vector<int32_t> shifts; ///< stores shifts from filtersquant
        std::vector<int32_t> multipliers; ///< stores multipliers from filtersquant
        QuantizationData outputquant; ///< stores output quantization data, here only offset
};

/**
 * Returns default VTA delegate options.
 *
 * @return default VTA delegate options
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

/**
 * Creates a VTA Delegate.
 *
 * @param options_keys list of options' names
 * @param options_values list of options' values
 * @param num_options number of options
 *
 * @return pointer to the VTA delegate
 */
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
