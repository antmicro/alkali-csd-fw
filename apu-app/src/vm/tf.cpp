/*
 * Copyright 2021-2022 Western Digital Corporation or its affiliates
 * Copyright 2021-2022 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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

	size_t offset = 0;
	for (unsigned int i = 0; i < interpreter->inputs().size(); i++)
	{
		auto in = interpreter->input_tensor(i);
		if (offset + in->bytes > input_len)
		{
			printf("Input buffer length mismatch: %d != %d\n", input_len, offset + in->bytes);
		}
		std::copy(input_buf + offset, input_buf + offset + in->bytes, interpreter->typed_input_tensor<int8_t>(i));
		offset += in->bytes;
	}

	timespec_get(&ts[0], TIME_UTC);
	interpreter->Invoke();
	timespec_get(&ts[1], TIME_UTC);

#ifdef DEBUG
	const uint64_t duration = (ts[1].tv_sec * 1000000000 + ts[1].tv_nsec) - (ts[0].tv_sec * 1000000000 + ts[0].tv_nsec);
	printf("Model processing took %llu ns\n", duration);
#endif

	offset = 0;
	for (unsigned int i = 0; i < interpreter->outputs().size(); i++)
	{
		auto out = interpreter->output_tensor(i);
		if (offset + out->bytes > output_len)
		{
			printf("Output buffer length mismatch: %d != %d\n", output_len, offset + out->bytes);
		}
		std::copy(interpreter->typed_output_tensor<int8_t>(i), interpreter->typed_output_tensor<int8_t>(i) + out->bytes, output_buf + offset);
		offset += out->bytes;
	}
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
