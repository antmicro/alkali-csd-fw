#include "vm.h"

#include "tensorflow/lite/interpreter.h"
#include "tensorflow/lite/kernels/register.h"
#include "tensorflow/lite/model.h"
#include "tensorflow/lite/tools/gen_op_registration.h"

#include "vta/tf_driver.h"

#include "tflite-delegate.hpp"

#include <algorithm>

void vm_tflite(char *ibuf, char *obuf, int isize, int osize)
{
	static const char model_path[] = "/bin/model.tflite";

	std::unique_ptr<tflite::FlatBufferModel> model = tflite::FlatBufferModel::BuildFromFile(model_path);

	// Build the interpreter
	tflite::ops::builtin::BuiltinOpResolver resolver;
	std::unique_ptr<tflite::Interpreter> interpreter;
	tflite::InterpreterBuilder(*model, resolver)(&interpreter);

	std::unique_ptr<TfLiteDelegate, decltype(&tflite::TfLiteCustomDelegateDelete)> delegate(tflite::TfLiteCustomDelegateCreate(NULL), &tflite::TfLiteCustomDelegateDelete);

	interpreter->ModifyGraphWithDelegate(std::move(delegate));

	// Resize input tensors, if desired.
	interpreter->AllocateTensors();

	float* input = interpreter->typed_input_tensor<float>(0);
	// ... pass input data to the input array

	std::copy(ibuf, ibuf+isize, input);

	// run model
	interpreter->Invoke();

	// get pointer to outputs
	float* output = interpreter->typed_output_tensor<float>(0);

	std::copy((char*)output, ((char*)output) + osize, obuf);

	printf("%f %f\n", output[0], output[1]);
}

void vm_tflite_vta(char *ibuf, char *obuf, int isize, int osize)
{
	static const char model_path[] = "/bin/model2.tflite";

	std::unique_ptr<tflite::FlatBufferModel> model = tflite::FlatBufferModel::BuildFromFile(model_path);

	// Build the interpreter
	tflite::ops::builtin::BuiltinOpResolver resolver;
	std::unique_ptr<tflite::Interpreter> interpreter;
	tflite::InterpreterBuilder(*model, resolver)(&interpreter);

	std::unique_ptr<TfLiteDelegate, decltype(&tflite::TfLiteCustomDelegateDelete)> delegate(tflite::TfLiteCustomDelegateCreate(NULL), &tflite::TfLiteCustomDelegateDelete);

	interpreter->ModifyGraphWithDelegate(std::move(delegate));

	// Resize input tensors, if desired.
	interpreter->AllocateTensors();

	int8_t* input1 = interpreter->typed_input_tensor<int8_t>(0);
	int8_t* input2 = interpreter->typed_input_tensor<int8_t>(1);
	// ... pass input data to the input array

	if(isize >= 4) {
		std::copy(ibuf, ibuf+2, input1);
		std::copy(ibuf+2, ibuf+4, input2);
	} else {
		input1[0] = 5;
		input1[1] = 8;
		input2[0] = 2;
		input2[1] = 4;
	}

	// run model
	interpreter->Invoke();

	// get pointer to outputs
	int8_t* output = interpreter->typed_output_tensor<int8_t>(0);

	std::copy((char*)output, ((char*)output) + osize, obuf);

	printf("%d %d\n", output[0], output[1]);
}
