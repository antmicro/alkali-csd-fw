#include "vm.h"

#include "tensorflow/lite/interpreter.h"
#include "tensorflow/lite/kernels/register.h"
#include "tensorflow/lite/model.h"
#include "tensorflow/lite/tools/gen_op_registration.h"

#include "vta/tf_driver.h"

#include "tflite-delegate.hpp"

#include <algorithm>

#include <time.h>

#define DEBUG

static void tflite_handler(char *model_buf, char *input_buf, char *output_buf, int model_len, int input_len, int output_len, bool with_vta)
{
	struct timespec ts[2];

	std::unique_ptr<tflite::FlatBufferModel> model = tflite::FlatBufferModel::BuildFromBuffer(model_buf, model_len);

	// Build the interpreter
	tflite::ops::builtin::BuiltinOpResolver resolver;
	std::unique_ptr<tflite::Interpreter> interpreter;
	tflite::InterpreterBuilder(*model, resolver)(&interpreter);

	if(with_vta) {
		std::unique_ptr<TfLiteDelegate, decltype(&tflite::TfLiteCustomDelegateDelete)> delegate(tflite::TfLiteCustomDelegateCreate(NULL), &tflite::TfLiteCustomDelegateDelete);
		interpreter->ModifyGraphWithDelegate(std::move(delegate));
	}

	// Resize input tensors, if desired.
	interpreter->AllocateTensors();

	// Check if buffer sizes are correct
	auto in = interpreter->input_tensor(0);
	auto out = interpreter->output_tensor(0);
	if(input_len != in->bytes)
		printf("Input buffer length mismatch: %d != %d\n", input_len, in->bytes);
	if(output_len != out->bytes)
		printf("Output buffer length mismatch: %d != %d\n", output_len, out->bytes);

	void *input_tensor = (void*)interpreter->typed_input_tensor<int8_t>(0);
	if(!input_tensor)
		input_tensor = (void*)interpreter->typed_input_tensor<float>(0);

	// ... pass input data to the input array
	std::copy(input_buf, input_buf+input_len, (char*)input_tensor);

	timespec_get(&ts[0], TIME_UTC);
	interpreter->Invoke();
	timespec_get(&ts[1], TIME_UTC);

#ifdef DEBUG
	const uint64_t duration = (ts[1].tv_sec * 1000000000 + ts[1].tv_nsec) - (ts[0].tv_sec * 1000000000 + ts[0].tv_nsec);
	printf("Model processing took %llu ns\n", duration);
#endif

	// get pointer to outputs
	void *output_tensor = interpreter->typed_output_tensor<int8_t>(0);
	if(!output_tensor)
		output_tensor = interpreter->typed_output_tensor<float>(0);

	std::copy((char*)output_tensor, ((char*)output_tensor) + output_len, output_buf);
}

void vm_tflite_apu(char *ibuf, char *obuf, int isize, int osize, int model_size)
{
	char *model_buf = ibuf;
	char *input_buf = ibuf+model_size;

	tflite_handler(model_buf, input_buf, obuf, model_size, isize, osize, false);
}

void vm_tflite_vta(char *ibuf, char *obuf, int isize, int osize, int model_size)
{
	char *model_buf = ibuf;
	char *input_buf = ibuf+model_size;

	tflite_handler(model_buf, input_buf, obuf, model_size, isize, osize, true);
}
