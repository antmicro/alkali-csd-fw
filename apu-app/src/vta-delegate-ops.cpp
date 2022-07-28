#include <vta-delegate.hpp>

#include "vta/vta_runtime.h"
#include "vta/hw_spec_const.h"

namespace tflite
{

VTAALUOp::~VTAALUOp()
{}

VTAGEMMOp::~VTAGEMMOp()
{}

VTAALUOp::VTAALUOp(int tfliteop, std::vector<int> tfliteinputs, std::vector<int> tfliteoutputs) :
    VTAOp("ALU", tfliteop, tfliteinputs, tfliteoutputs)
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

VTAGEMMOp::VTAGEMMOp(int tfliteop, std::vector<int> tfliteinputs, std::vector<int> tfliteoutputs) :
    VTAOp("GEMM", tfliteop, tfliteinputs, tfliteoutputs)
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

TfLiteStatus VTAALUOp::getComputeOps()
{
    switch (tfliteop)
    {
        case kTfLiteBuiltinAdd:
            return getAddOps();
            break;
    }
    return kTfLiteUnresolvedOps;
}

TfLiteStatus VTAGEMMOp::getComputeOps()
{
    switch (tfliteop)
    {
        case kTfLiteBuiltinConv2d:
            return getConv2dOps();
            break;
    }
    return kTfLiteOk;
}

TfLiteStatus VTAALUOp::getAddOps()
{

    return kTfLiteOk;
}

TfLiteStatus VTAGEMMOp::getConv2dOps()
{
    return kTfLiteOk;
}

};
