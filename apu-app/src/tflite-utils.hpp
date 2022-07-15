#pragma once

#include "tensorflow/lite/interpreter.h"

/**
 * Creates an interpreter for TensorFlow Lite model.
 *
 * Loads the model from Flatbuffer and creates an interpreter.
 *
 * It still requires allocating tensors, and optionally assigning delegate to it.
 *
 * @param filepath path to the TFLite model
 * @return unique_ptr to the Interpreter object
 */
std::unique_ptr<tflite::Interpreter> createInterpreter(std::string filepath);
