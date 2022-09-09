/*
 * Copyright 2021-2022 Western Digital Corporation or its affiliates
 * Copyright 2021-2022 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <vta-delegate.hpp>

#include "vta/vta_runtime.h"
#include "vta/hw_spec_const.h"
#include "tensorflow/lite/kernels/internal/tensor_ctypes.h"

#include <algorithm>

#define NUM_THREADS 2
#define VTA_UOP_GEMM 0
#define VTA_UOP_ALU 1

namespace tflite
{

// NOTE summary of useful functions

// VTAUopLoopBegin(extent, dst_factor, src_factor, wgt_factor)
//  * extent - the extent of the loop
//  * dst_factor - the accum factor
//  * src_factor - the input factor
//  * wgt_factor - the weight factor

// VTALoadBuffer2D(VTACommandHandle cmd, void* src_dram_addr, uint32_t src_elem_offset,
//                 uint32_t x_size, uint32_t y_size, uint32_t x_stride,
//                 uint32_t x_pad_before, uint32_t y_pad_before, uint32_t x_pad_after,
//                 uint32_t y_pad_after, uint32_t dst_sram_index,
//                 uint32_t dst_memory_type);
//  * cmd - VTA command handle
//  * src_dram_addr - source DRAM address
//  * src_elem_offset - the source DRAM offset in number of unit elements
//  * x_size - the lowest dimension (x axis) size in number of unit elements
//  * y_size - the number of rows (y axis)
//  * x_stride - the x axis stride
//  * x_pad_before - start padding on x axis
//  * y_pad_before - start padding on y axis
//  * x_pad_after - end padding on x axis
//  * y_pad_after - end padding on y axis
//  * dst_sram_index - destination SRAM index
//  * dst_memory_type - destination memory type

// VTAStoreBuffer2D(VTACommandHandle cmd, uint32_t src_sram_index,
//                  uint32_t src_memory_type, void* dst_dram_addr,
//                  uint32_t dst_elem_offset, uint32_t x_size, uint32_t y_size,
//                  uint32_t x_stride);

// VTAUopPush(uint32_t mode, uint32_t reset_out, uint32_t dst_index, uint32_t src_index,
//                 uint32_t wgt_index, uint32_t opcode, uint32_t use_imm, int32_t imm_val);
//
//      DType accum[INP_BUFF_DEPTH][l][n];
//      DType weight[WGT_BUFF_DEPTH][n][m];
//      DType input[INP_BUFF_DEPTH][l][m];
//      if reset_out == 1
//       accum[dst_index] = 0
//      elif mode == 0
//       accum[dst_index] += GEMM(input[src_index], weight[wgt_index]);
//      else
//       if (use_imm)
//         accum[dst_index] = opcode(accum[dst_index], imm_val);
//       else
//         accum[dst_index] = opcode(accum[dst_index], accum[src_index]);
//
//  * mode - 0 for GEMM, 1 for ALU
//  * reset_out - resets the accum
//  * dst_index - accum memory index
//  * src_index - input memory (gemm) / accum memory (alu) index
//  * wgt_index - weight memory index
//  * opcode - ALU opcode
//  * use_imm - store in imm_val
//  * imm_val - immediate value in ALU mode

VTAALUOp::~VTAALUOp()
{}

VTAGEMMOp::~VTAGEMMOp()
{}

VTAALUOp::VTAALUOp(VTADelegateKernel *parent, int tfliteop, std::vector<int> tfliteinputs, std::vector<int> tfliteoutputs) :
    VTAOp(parent, "ALU", tfliteop, tfliteinputs, tfliteoutputs)
{
    switch (tfliteop)
    {
        case kTfLiteBuiltinAdd:
            name = "ADD";
            break;
        default:
            name = "unknown";
    }
}

VTAGEMMOp::VTAGEMMOp(VTADelegateKernel *parent, int tfliteop, std::vector<int> tfliteinputs, std::vector<int> tfliteoutputs) :
    VTAOp(parent, "GEMM", tfliteop, tfliteinputs, tfliteoutputs)
{
    switch (tfliteop)
    {
        case kTfLiteBuiltinConv2d:
            name = "CONV2D";
            break;
        default:
            name = "unknown";
    }
}

TfLiteStatus VTAALUOp::compute()
{
    switch (tfliteop)
    {
        case kTfLiteBuiltinAdd:
            return aluAdd();
            break;
    }
    return kTfLiteUnresolvedOps;
}

TfLiteStatus VTAGEMMOp::compute()
{
    switch (tfliteop)
    {
        case kTfLiteBuiltinConv2d:
            return gemmConv2D();
            break;
    }
    return kTfLiteOk;
}

TfLiteStatus VTAALUOp::aluAdd()
{
    return kTfLiteOk;
}

TfLiteStatus VTAGEMMOp::gemmConv2D()
{
    // The convolution can be described with the following parameters:
    // * padding - TODO
    // * stride - TODO
    // * N - batch size
    // * H - input tensor height
    // * W - input tensor width
    // * I - number of input channels (also number of kernel channels)
    // * O - number of output channels (also number of kernels)
    //
    // For VTA, we have:
    // * input tensor - N I H W format
    // * kernel tensor - O Hk Wk I format
    // * output tensor - N O Ho Wo format
    //
    // VTA performs GEMM operation on:
    // * input matrix (usually vector) of size BATCH_SIZE x BLOCK_IN
    // * weight matrix of size BLOCK_OUT x BLOCK_IN
    // * output matrix (usually vector) of size BATCH_SIZE x BLOCK_OUT
    //
    // VTA cannot process entire convolution at once - the processing needs to be blocked
    // to satisfy below constraints.
    //
    // That is why the actual processing flow is performed as follows:
    // * input takes shape No Io H W n i
    // * weight takes shape Oo Io Hk Wk o i
    // * output takes shape No Oo Ho Wo n o
    //
    // Where:
    // * Xo is an outer dimension of its original dimension.
    //   For example, if we have 64 input channels, and the BLOCK_IN equals 16
    //   then "Io" equals 4 and "i" equals 16
    // * lowercase letters are inner dimension of the original dimension.
    //
    // This way we have loops for No/Io/Oo/H/W dimensions, while operating on matrices
    // of size (n,i), (o,i) and (n,o), which fits into VTA.

    // The data in TFLite is delivered in this order:
    // 0 - input activations (N H W C format)
    // 1 - weights (O Hk Wk I)
    // 2 - biases (O)

    // Grab tensors for the operation
    auto &inpptr = parent->context->tensors[inputs[0]]; // input tensor
    auto &wgtptr = parent->context->tensors[inputs[1]]; // weight tensor
    auto &bisptr = parent->context->tensors[inputs[2]]; // bias tensor
    auto &outptr = parent->context->tensors[outputs[0]]; // output tensor

    resetDims();

    // Get input, weights and bias dimensions
    setDim("N", inpptr.dims->data[0]);
    setDim("H", inpptr.dims->data[1]);
    setDim("W", inpptr.dims->data[2]);
    setDim("I", inpptr.dims->data[3]);

    setDim("Ho", outptr.dims->data[1]);
    setDim("Wo", outptr.dims->data[2]);
    setDim("O", outptr.dims->data[3]);

    setDim("Hk", wgtptr.dims->data[1]);
    setDim("Wk", wgtptr.dims->data[2]);

    setDim("Ni", VTA_BATCH);
    setDim("Ii", VTA_BLOCK_IN);
    setDim("Oi", VTA_BLOCK_OUT);

    // FIXME set padding and stride according to TFLite model
    setDim("paddingH", 0);
    setDim("paddingW", 0);
    setDim("strideH", 1);
    setDim("strideW", 1);

    // Compute working dimensions for VTA
    // TODO consider dimensions not divisible by below dimensions (TVM adds padding)
    setDim("No", dim("N") / VTA_BATCH);
    setDim("Io", dim("I") / VTA_BLOCK_IN);
    setDim("Oo", dim("O") / VTA_BLOCK_OUT);

    // Reshape inputs for tensorization
    std::vector<uint8_t> inparray(inpptr.bytes);
    permuteDims(
        {"No", "Ni", "Io", "Ii", "H", "W"},
        {"No", "Io", "H", "W", "Ni", "Ii"},
        GetTensorData<uint8_t>(&inpptr),
        inparray.data(),
        1 // TODO configure size of input
    );

    // TODO move weights' permutting to Init or Prepare

    // Reshape weights for tensorization
    std::vector<uint8_t> wgtarray(wgtptr.bytes);
    permuteDims(
        {"Oo", "Oi", "Io", "Ii", "H", "W"},
        {"Oo", "Io", "H", "W", "Oi", "Ii"},
        GetTensorData<uint8_t>(&wgtptr),
        wgtarray.data(),
        1 // TODO make size of input configurable
    );

    std::vector<uint8_t> outarray(outptr.bytes);

    // Create a command handler for VTA
    auto cmd = VTATLSCommandHandle();

    const int inpelemsfull = tensorElements({"No", "Io", "H", "W", "Ni", "Ii"});
    const int wgtelemsfull = tensorElements({"Oo", "Io", "Hk", "Wk", "Oi", "Ii"});
    const int outelemsfull = tensorElements({"No", "Oo", "Ho", "Wo", "Ni", "Oi"});

    // Create DRAM buffers for inputs, weights and outputs
    auto *inpbuf = VTABufferAlloc(sizeof(int8_t) * inpelemsfull);
    auto *wgtbuf = VTABufferAlloc(sizeof(int8_t) * wgtelemsfull);
    auto *outbuf = VTABufferAlloc(sizeof(int32_t) * outelemsfull); // TODO int32

    // TODO consider tiling here?
    VTABufferCopy(inparray.data(), 0, inpbuf, 0, sizeof(int8_t) * inpelemsfull, VTA_MEMCPY_H2D);
    VTABufferCopy(wgtarray.data(), 0, wgtbuf, 0, sizeof(int8_t) * wgtelemsfull, VTA_MEMCPY_H2D);

    // Sync for finishing the processing
    VTADepPush(cmd, vta::kStoreStage, vta::kComputeStage);

    // Sync for fetching the data
    VTADepPush(cmd, vta::kStoreStage, vta::kComputeStage);

    // TODO implement tests for dimension permutations
    // TODO finish implementation of CONV2D operation

    // The looping below does not perform computations, only creates commands that
    // are executed asynchronously

    // Splitting along height
    // TODO improve splitting height dimension
    const int hthreads = 2;
    // Virtual threads for latency hiding
    const int vthreads = 2;

    // output: No HOo COoo WOo COoi HOi WOi Ni COi
    for (int No = 0; No < dim("No"); No++)
    {
        // We split the computations along height
        for (int Hoo = 0; Hoo < hthreads; Hoo++)
        {
            int heightstep = dim("Ho") / hthreads;
            // Let's start off with resetting the accumulator
            // To hide latency, do it in two virtual threads, splitted along inner output channels
            for (int threadid = 0; threadid < vthreads; threadid++)
            {
                VTADepPop(cmd, vta::kStoreStage, vta::kComputeStage);
                // wrap micro-op schedule functions in lambda function
                auto lambda = [threadid, heightstep, Wo=dim("Wo"), Oi=dim("Oi")](void *signature) -> int {
                    // the outer loop
                    // TODO Oi may need to be replaced with tiled version
                    int outstep = Oi / 2;
                    VTAUopLoopBegin(outstep, heightstep * Wo, 0, 0);
                    // the inner loop
                    VTAUopLoopBegin(heightstep, Wo, 0, 0);
                    // run vector operation
                    // mode (0 for GEMM, 1 for ALU), reset (1 if reset accum to 0), input memory index, weight memory index (not used here), ALU opcode, use_imm (1 if use immediate mode in ALU), imm_val (immediate value in ALU)
                    for (int w = 0; w < Wo; w++)
                    {
                        VTAUopPush(VTA_UOP_GEMM, 1, threadid * heightstep * outstep * Wo + w, 0, 0, 0, 0, 0);
                    }
                    // end inner loop
                    VTAUopLoopEnd();
                    // end outer loop
                    VTAUopLoopEnd();
                    return 0;
                };

                // create micro-op kernel for vector addition
                // UopKernelMap object (can be nullptr), op definition, text signature, length of signature
                VTAPushALUOp(
                    &cmd,
                    lambda,
                    nullptr,
                    0
                );
                VTADepPush(cmd, vta::kComputeStage, vta::kLoadStage);
            }

            // perform CONV2D
            for (int Io = 0; Io < dim("Io"); Io++)
            {
                int outheightshift = Hoo * heightstep;
                int wgtheightshift = Io * tensorElements({"Hk", "Wk"});
                // TODO check paddings
                int paddingheighttop = std::max(dim("paddingH") - outheightshift, 0);
                int paddingheightbottom = std::max(outheightshift - (heightstep * (hthreads - 1) - dim("paddingH")), 0);
                int inp_y_size = heightstep + dim("Hk") - 1 - paddingheighttop - paddingheightbottom;
                int inpshift = Io * hthreads * heightstep * dim("Wi") + Hoo * heightstep * dim("Wi") + paddingheighttop * dim("Wi") - dim("paddingH") * dim("Wi");
                // Data loading thread 1
                VTADepPop(cmd, vta::kComputeStage, vta::kLoadStage);
                // Load input data
                VTALoadBuffer2D(
                    cmd, // cmd
                    inpbuf, // src_dram_addr
                    inpshift, // src_elem_offset
                    dim("Wi"), // x_size
                    inp_y_size, // y_size
                    dim("Wi"), // x_stride
                    dim("paddingW"), // x_pad_before
                    paddingheighttop, // y_pad_before
                    dim("paddingW"), // x_pad_after
                    paddingheightbottom, // y_pad_after
                    0, //dst_sram_index
                    VTA_MEM_ID_INP // dst_memory_type
                );
                // Load weights data
                VTALoadBuffer2D(
                    cmd, // cmd
                    wgtbuf, // src_dram_addr
                    wgtheightshift, // src_elem_offset
                    tensorElements({"Hk", "Wk"}), // x_size
                    dim("Ii") / 2, // y_size
                    tensorElements({"Hk", "Wk", "Io"}), // x_stride
                    0, // x_pad_before
                    0, // y_pad_before
                    0, // x_pad_after
                    0, // y_pad_after
                    0, //dst_sram_index
                    VTA_MEM_ID_WGT // dst_memory_type
                );
                VTADepPush(cmd, vta::kLoadStage, vta::kComputeStage);
                // Data loading thread 2
                VTALoadBuffer2D(
                    cmd, // cmd
                    inpbuf, // src_dram_addr
                    inpshift, // src_elem_offset
                    dim("Wi"), // x_size
                    inp_y_size, // y_size
                    dim("Wi"), // x_stride
                    dim("paddingW"), // x_pad_before
                    paddingheighttop, // y_pad_before
                    dim("paddingW"), // x_pad_after
                    paddingheightbottom, // y_pad_after
                    (dim("Wi") + 2 * dim("paddingW")) * (heightstep + dim("Hk") - 1), //dst_sram_index
                    VTA_MEM_ID_INP // dst_memory_type
                );
                VTALoadBuffer2D(
                    cmd, // cmd
                    inpbuf, // src_dram_addr
                    inpshift, // src_elem_offset
                    dim("Wi"), // x_size
                    inp_y_size, // y_size
                    dim("Wi"), // x_stride
                    dim("paddingW"), // x_pad_before
                    paddingheighttop, // y_pad_before
                    dim("paddingH"), // x_pad_after
                    paddingheightbottom, // y_pad_after
                    heightstep * dim("Wi"), // dst_sram_index
                    VTA_MEM_ID_INP // dst_memory_type
                );
                // Load weights data
                VTALoadBuffer2D(
                    cmd, // cmd
                    wgtbuf, // src_dram_addr
                    wgtheightshift + tensorElements({"Hk", "Wk", "Io"}) * (dim("Ii") / 2), // src_elem_offset
                    tensorElements({"Hk", "Wk"}), // x_size
                    dim("Ii") / 2, // y_size
                    tensorElements({"Hk", "Wk", "Io"}), // x_stride
                    0, // x_pad_before
                    0, // y_pad_before
                    0, // x_pad_after
                    0, // y_pad_after
                    tensorElements({"Hk", "Wk"}) * (dim("Ii") / 2), //dst_sram_index
                    VTA_MEM_ID_WGT // dst_memory_type
                );
                VTADepPop(cmd, vta::kComputeStage, vta::kLoadStage);
                VTADepPush(cmd, vta::kLoadStage, vta::kComputeStage);
                for (int threadid = 0; threadid < vthreads; threadid++)
                {
                    VTADepPop(cmd, vta::kLoadStage, vta::kComputeStage);
                    auto lambda = [threadid, Wi=dim("Wi"), paddingW=dim("paddingW"), Oi=dim("Oi"), Wo=dim("Wo"), Hk=dim("Hk"), Wk=dim("Wk"), heightstep](void *signature) -> int {
                        // the outer loop
                        // TODO Oi may need to be replaced with tiled version
                        VTAUopLoopBegin(Oi, heightstep * Wo, 0, Hk * Wk);
                        // the inner loop
                        VTAUopLoopBegin(heightstep, Wo, Wi + paddingW * 2, 0);
                        // iterate over kernel height
                        for (int hk = 0; hk < Hk; hk++)
                        {
                            // iterate over kernel width
                            for (int wk = 0; wk < Wk; wk++)
                            {
                                // iterate over output width
                                for (int wo = 0; wo < Wo; wo++)
                                {
                                    // TODO
                                    VTAUopPush(
                                        VTA_UOP_GEMM, // mode
                                        0, // reset_out
                                        threadid * Oi * heightstep * Wo + wo, //accum memory index
                                        threadid * Oi * (Wi + paddingW * 2) + hk * (Wi + paddingW * 2) + wo + wk, // input memory index
                                        threadid * heightstep *  + hk * Wk + wk, // weight memory index
                                        0, // opcode, unused
                                        0, // use_imm, unused
                                        0  // imm_val, unused
                                    );
                                }
                            }
                        }
                        for (int w = 0; w < Wo; w++)
                        {
                            VTAUopPush(VTA_UOP_GEMM, 1, threadid * Oi * heightstep * Wo, 0, 0, 0, 0, 0);
                        }
                        // end inner loop
                        VTAUopLoopEnd();
                        // end outer loop
                        VTAUopLoopEnd();
                        return 0;
                    };

                    // create micro-op kernel for vector addition
                    // UopKernelMap object (can be nullptr), op definition, text signature, length of signature
                    VTAPushALUOp(
                        &cmd,
                        lambda,
                        nullptr,
                        0
                    );
                    VTADepPush(cmd, vta::kComputeStage, vta::kLoadStage);
                }
            }
            VTADepPush(cmd, vta::kComputeStage, vta::kStoreStage);
            VTADepPush(cmd, vta::kComputeStage, vta::kStoreStage);
            VTADepPop(cmd, vta::kComputeStage, vta::kLoadStage);
            VTADepPop(cmd, vta::kComputeStage, vta::kLoadStage);
            // store results
            const int outchanstep = dim("Oo") / vthreads;
            for (int threadid = 0; threadid < vthreads; threadid++)
            {
                for (int o = 0; o < outchanstep; o++)
                {
                    for (int y = 0; y < heightstep; y++)
                    {
                        for (int x = 0; x < dim("Wo"); x++)
                        {
                            int widthshift = y * dim("Wo");
                            VTAStoreBuffer2D(
                                cmd, // command handle
                                threadid * outchanstep * heightstep * dim("Wo")  + o * heightstep * dim("Wo") + widthshift + x, // src_sram_index
                                VTA_MEM_ID_OUT, // src_memory_type
                                outbuf, // dst_dram_addr
                                threadid * outchanstep * dim("Ho") * dim("Wo") + o * dim("Ho") * dim("Wo") + y * heightstep * dim("Wo") + widthshift + x, // dst_elem_offset
                                1, // FIXME x_size?
                                1, // FIXME y_size?
                                1  // FIXME x_stride?
                            );
                        }
                    }
                }
                VTADepPush(cmd, vta::kComputeStage, vta::kStoreStage);
            }
        }
    }

    // Sync for finishing the processing
    VTADepPop(cmd, vta::kStoreStage, vta::kComputeStage);

    // Sync for fetching the data
    VTADepPop(cmd, vta::kStoreStage, vta::kComputeStage);

    // synchronize with VTA
    VTASynchronize(cmd, 10000000);

    VTABufferCopy(outbuf, 0, outarray.data(), 0, sizeof(int32_t) * outelemsfull, VTA_MEMCPY_D2H);

    permuteDims(
        {"No", "Oo", "Ho", "Wo", "Ni", "Oi"},
        {"No", "Ni", "Oo", "Oi", "Ho", "Wo"},
        outarray.data(),
        GetTensorData<uint8_t>(&outptr),
        1 // TODO make size of input configurable
    );

    return kTfLiteOk;
}

void VTAGEMMOp::permuteDims(
    const std::vector<std::string> &inplayout,
    const std::vector<std::string> &outlayout,
    uint8_t *inparray,
    uint8_t *outarray,
    size_t elemsize)
{
    for (int i = 0; i < outlayout.size() / elemsize; i++)
    {
        int newindex = 0;
        int remaining = i;
        for (auto &inpdim : inplayout)
        {
            int step = getDimStep(inplayout, inpdim);
            int dimid = remaining / step;
            newindex += dimid * getDimStep(outlayout, inpdim);
            remaining %= step;
        }
        std::copy(&inparray[i * elemsize], &inparray[(i + 1) * elemsize], &outarray[newindex]);
    }
}

int VTAGEMMOp::tensorElements(const std::vector<std::string> &layout)
{
    int size = 1;
    for (auto &dimname : layout)
    {
        size *= dim(dimname);
    }
    return size;
}

void VTAGEMMOp::resetDims()
{
    dims.clear();
}

void VTAGEMMOp::setDim(std::string dimname, int dimsize)
{
    dims[dimname] = dimsize;
}

int VTAGEMMOp::dim(std::string dimname)
{
    return dims[dimname];
}

int VTAGEMMOp::getDimStep(const std::vector<std::string> &layout, const std::string &axis)
{
    int step = 1;
    for (int i = layout.size() - 1; layout[i] != axis; i--)
    {
        step *= dims[layout[i]];
    }
    return step;
}
};
