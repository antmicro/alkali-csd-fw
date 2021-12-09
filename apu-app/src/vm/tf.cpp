#include "vm.h"

#include "tensorflow/lite/interpreter.h"
#include "tensorflow/lite/kernels/register.h"
#include "tensorflow/lite/model.h"
#include "tensorflow/lite/tools/gen_op_registration.h"

#include "vta/tf_driver.h"

#include "tflite-delegate.hpp"

#include <algorithm>

#include <time.h>

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

	struct timespec ts[2];
	timespec_get(&ts[0], TIME_UTC);

	// run model
	interpreter->Invoke();

	timespec_get(&ts[1], TIME_UTC);

	const uint64_t duration = (ts[1].tv_sec - ts[0].tv_sec) * 1000000000 + (ts[1].tv_nsec - ts[0].tv_nsec);

	printf("Model processing took %llu ns\n", duration);

	// get pointer to outputs
	float* output = interpreter->typed_output_tensor<float>(0);

	std::copy((char*)output, ((char*)output) + osize, obuf);

	printf("%f %f\n", output[0], output[1]);
}

void vm_tflite_float(char *ibuf, char *obuf, int isize, int osize, int model_size)
{
	const char *model_buf = ibuf;
	ibuf += model_size;

	std::unique_ptr<tflite::FlatBufferModel> model = tflite::FlatBufferModel::BuildFromBuffer(model_buf, model_size);

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

	struct timespec ts[2];
	timespec_get(&ts[0], TIME_UTC);

	// run model
	interpreter->Invoke();

	timespec_get(&ts[1], TIME_UTC);

	const uint64_t duration = (ts[1].tv_sec - ts[0].tv_sec) * 1000000000 + (ts[1].tv_nsec - ts[0].tv_nsec);

	printf("Model processing took %llu ns\n", duration);

	// get pointer to outputs
	float* output = interpreter->typed_output_tensor<float>(0);

	std::copy((char*)output, ((char*)output) + osize, obuf);

	for(int i = 0; i < 1000; i++) {
		if (i%20 == 0)
			printf("\n");
		printf("%f ", output[i]);
	}
}

void vm_tflite_uint(char *ibuf, char *obuf, int isize, int osize, int model_size)
{
	const char *model_buf = ibuf;
	ibuf += model_size;

	std::unique_ptr<tflite::FlatBufferModel> model = tflite::FlatBufferModel::BuildFromBuffer(model_buf, model_size);

	// Build the interpreter
	tflite::ops::builtin::BuiltinOpResolver resolver;
	std::unique_ptr<tflite::Interpreter> interpreter;
	tflite::InterpreterBuilder(*model, resolver)(&interpreter);

	// Resize input tensors, if desired.
	interpreter->AllocateTensors();

	int8_t* input = interpreter->typed_input_tensor<int8_t>(0);
	// ... pass input data to the input array

	std::copy(ibuf, ibuf+isize, input);

	struct timespec ts[2];
	timespec_get(&ts[0], TIME_UTC);

	// run model
	interpreter->Invoke();

	timespec_get(&ts[1], TIME_UTC);

	const uint64_t duration = (ts[1].tv_sec - ts[0].tv_sec) * 1000000000 + (ts[1].tv_nsec - ts[0].tv_nsec);

	printf("Model processing took %llu ns\n", duration);

	// get pointer to outputs
	int8_t* output = interpreter->typed_output_tensor<int8_t>(0);

	std::copy((char*)output, ((char*)output) + osize, obuf);

	for(int i = 0; i < 1000; i++) {
		if (i%20 == 0)
			printf("\n");
		printf("%d ", output[i]);
	}
}

void vm_tflite_vta(char *ibuf, char *obuf, int isize, int osize, int model_size)
{
	const char *model_buf = ibuf;
	ibuf += model_size;

	std::unique_ptr<tflite::FlatBufferModel> model = tflite::FlatBufferModel::BuildFromBuffer(model_buf, model_size);

	// Build the interpreter
	tflite::ops::builtin::BuiltinOpResolver resolver;
	std::unique_ptr<tflite::Interpreter> interpreter;
	tflite::InterpreterBuilder(*model, resolver)(&interpreter);

	std::unique_ptr<TfLiteDelegate, decltype(&tflite::TfLiteCustomDelegateDelete)> delegate(tflite::TfLiteCustomDelegateCreate(NULL), &tflite::TfLiteCustomDelegateDelete);

	interpreter->ModifyGraphWithDelegate(std::move(delegate));

	// Resize input tensors, if desired.
	interpreter->AllocateTensors();

	int8_t * input = interpreter->typed_input_tensor<int8_t>(0);
	// ... pass input data to the input array

	std::copy(ibuf, ibuf+isize, input);

	struct timespec ts[2];
	timespec_get(&ts[0], TIME_UTC);

	// run model
	interpreter->Invoke();

	timespec_get(&ts[1], TIME_UTC);

	const uint64_t duration = (ts[1].tv_sec - ts[0].tv_sec) * 1000000000 + (ts[1].tv_nsec - ts[0].tv_nsec);

	printf("Model processing took %llu ns\n", duration);

	// get pointer to outputs
	int8_t* output = interpreter->typed_output_tensor<int8_t>(0);

	std::copy((char*)output, ((char*)output) + osize, obuf);

	for(int i = 0; i < 1000; i++) {
		if (i%20 == 0)
			printf("\n");
		printf("%d ", output[i]);
	}
}
