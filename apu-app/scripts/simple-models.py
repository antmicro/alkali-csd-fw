# Copyright 2021-2022 Western Digital Corporation or its affiliates
# Copyright 2021-2022 Antmicro
#
# SPDX-License-Identifier: Apache-2.0

import torch
import argparse
from pathlib import Path
from torch.autograd import Variable
import itertools


def create_simple_network():
    model = torch.nn.Sequential(torch.nn.Linear(100, 2))
    for param in model.parameters():
        param.requires_grad = False
    model[0].weight[0, :] = 2
    model[0].weight[1, :] = 5
    model[0].bias[0] = 0
    model[0].bias[1] = 0
    print(model)
    data = Variable(torch.FloatTensor([[i for i in range(100)]]))
    print(model(data))
    torch.onnx.export(model, data, "simple.onnx", verbose=True)


def create_simple_conv():
    model = torch.nn.Sequential(torch.nn.Conv2d(1, 1, 3))
    for param in model.parameters():
        param.requires_grad = False
    model[0].weight[0, 0, 0, 0] = 0.0
    model[0].weight[0, 0, 0, 1] = -1.0
    model[0].weight[0, 0, 0, 2] = 0.0
    model[0].weight[0, 0, 1, 0] = -1.0
    model[0].weight[0, 0, 1, 1] = 4.0
    model[0].weight[0, 0, 1, 2] = -1.0
    model[0].weight[0, 0, 2, 0] = 0.0
    model[0].weight[0, 0, 2, 1] = -1.0
    model[0].weight[0, 0, 2, 2] = 0.0
    model[0].bias[:] = 0.0
    print(model[0].weight)
    print(model[0].bias)
    # model[0].weight[0, :] = 2
    # model[0].weight[1, :] = 5
    # model[0].bias[0] = 0
    # model[0].bias[1] = 0
    print(model)
    data = Variable(
        torch.FloatTensor([[[[i for i in range(5)] for _ in range(5)]]])
    )
    print(data)
    print(model(data).numpy())
    torch.onnx.export(model, data, "conv.onnx", verbose=True)


def simple_conv2d(
        out: Path,
        inputsize: int,
        inpchannels: int,
        outchannels: int,
        kernelsize: int,
        stride: int,
        padding: int):

    out.parent.mkdir(parents=True, exist_ok=True)

    print(f'Creating {str(out)}')

    model = torch.nn.Sequential(torch.nn.Conv2d(
        inpchannels,
        outchannels,
        kernelsize,
        stride=stride,
        padding=padding
    ))
    for param in model.parameters():
        param.requires_grad = False
    print(model)
    data = Variable(torch.zeros([1, inpchannels, inputsize, inputsize]))
    print(data)
    print(model(data).numpy())
    input_names = ['input_0']
    output_names = ['output_0']
    torch.onnx.export(
        model,
        data,
        str(out),
        verbose=True,
        input_names=input_names,
        output_names=output_names
    )


def simple_add(out: Path, vector_length: int):
    class SimpleAdd(torch.nn.Module):
        def __init__(self):
            super(SimpleAdd, self).__init__()

        def forward(self, x, y):
            return x + y

    out.parent.mkdir(parents=True, exist_ok=True)

    print(f'Creating {str(out)}')

    model = SimpleAdd()
    data1 = Variable(torch.FloatTensor([[i for i in range(vector_length)]]))
    data2 = Variable(torch.FloatTensor([[i for i in range(vector_length)]]))
    print(model)
    print(model.forward(data1, data2))
    input_names = ['input_0', 'input_1']
    output_names = ['output_0']
    torch.onnx.export(
        model,
        (data1, data2,),
        str(out),
        verbose=True,
        input_names=input_names,
        output_names=output_names
    )


def simple_conv2d_add_network(out: Path):
    class TestDelegateModel(torch.nn.Module):
        def __init__(self):
            # input: 1x5x5
            super(TestDelegateModel, self).__init__()
            self.l = {}
            self.l['x1'] = torch.nn.Conv2d(1, 3, 3, padding=1)  # 3x5x5
            self.l['x2'] = torch.nn.Conv2d(3, 3, 3, padding=1)  # 3x5x5
            self.l['x3'] = torch.nn.Conv2d(3, 5, 3)  # 5x3x3
            self.l['r1'] = torch.nn.Conv2d(5, 1, 3)  # 1x1x1
            for _, layer in self.l.items():
                layer.weight.requires_grad = False
                layer.bias.requires_grad = False

        def forward(self, x):
            rx1 = self.l['x1'](x)
            rx2 = self.l['x2'](rx1)
            rx3 = rx1 + rx2
            rx4 = self.l['x3'](rx3)
            return self.l['r1'](rx4)

    out.parent.mkdir(parents=True, exist_ok=True)

    model = TestDelegateModel()
    data = Variable(
        torch.FloatTensor([[[[i for i in range(5)] for _ in range(5)]]])
    )
    print(model.forward(data))
    torch.onnx.export(
        model,
        (data,),
        str(out)
    )


def simple_conv2d_add_network_v2(out: Path):
    class TestDelegateModel(torch.nn.Module):
        def __init__(self):
            # input: 1x5x5
            super(TestDelegateModel, self).__init__()
            self.l = {}
            self.l['x1'] = torch.nn.Conv2d(1, 3, 3)  # 3x5x5
            self.l['x2'] = torch.nn.Conv2d(1, 3, 3)  # 3x5x5
            self.l['x3'] = torch.nn.Conv2d(3, 5, 3)  # 5x3x3
            self.l['r1'] = torch.nn.Conv2d(5, 1, 3)  # 1x1x1
            for _, layer in self.l.items():
                layer.weight.requires_grad = False
                layer.bias.requires_grad = False

        def forward(self, x):
            rx1 = self.l['x1'](x)
            rx2 = self.l['x2'](x)
            rx3 = rx1 + rx2
            rx4 = self.l['x3'](rx3)
            return self.l['r1'](rx4)

    out.parent.mkdir(parents=True, exist_ok=True)

    model = TestDelegateModel()
    data = Variable(
        torch.FloatTensor([[[[i for i in range(7)] for _ in range(7)]]])
    )
    print(model.forward(data))
    torch.onnx.export(
        model,
        (data,),
        str(out)
    )


def create_network_with_two_inputs():
    class SimpleComp(torch.nn.Module):
        def __init__(self):
            super(SimpleComp, self).__init__()
            self.fulllayer = torch.nn.Linear(2, 1)
            self.fulllayer.weight.requires_grad = False
            self.fulllayer.weight[0, 0] = 3
            self.fulllayer.weight[0, 1] = 4
            self.fulllayer.bias.requires_grad = False
            self.fulllayer.bias[0] = 1
            self.convolution = torch.nn.Conv2d(1, 1, 3, 1, 1, bias=True)
            self.convolution.weight.requires_grad = False
            self.convolution.weight[:, :, :, :] = 0
            self.convolution.weight[0, 0, 0, 1] = 1
            self.convolution.weight[0, 0, 1, 0] = 1
            self.convolution.weight[0, 0, 2, 1] = 1
            self.convolution.weight[0, 0, 1, 2] = 1
            self.convolution.bias.requires_grad = False
            self.convolution.bias[:] = 0
            print(self.convolution.weight)
            print(self.convolution.bias)

        def forward(self, x, y):
            val1 = self.fulllayer(x)
            val2 = self.convolution(y)
            val = torch.mul(val2, val1)
            return val
    model = SimpleComp()
    data1 = Variable(torch.FloatTensor([[1.0, 2.5]]))
    data2 = Variable(torch.FloatTensor([
        [[[2, 3, 4, 1],  [0, 4, 5, 6],  [1, 8, 1, 1],  [0, 3, 0, 2]]]
    ]))
    print(model.forward(data1, data2))
    print(model)
    torch.onnx.export(model, (data1, data2,), "two-inputs.onnx", verbose=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument(
        'output_dir',
        type=Path,
        help='Output directory with the models'
    )

    args = parser.parse_args()

    args.output_dir.mkdir(parents=True, exist_ok=True)

    alu_vectors_sizes = [
        1, 4, 16, 25, 64, 256, 1024, 4096, 8192, 10000, 200007
    ]
    for size in alu_vectors_sizes:
        simple_add(args.output_dir / 'add' / f'add-{size}.onnx', size)

    # inputsize, inpchannels, outchannels, kernelsize, stride, padding

    inpsizes = [10, 224]
    inpchannels = [1, 3, 32]
    outchannels = [32, 256]
    kernelsizes = [3, 5, 11]
    strides = [1, 3]
    paddings = [0, 2]

    for variant in itertools.product(
            inpsizes,
            inpchannels,
            outchannels,
            kernelsizes,
            strides,
            paddings):
        isize, ichan, ochan, ksize, stride, padding = variant
        if padding > ksize // 2:
            continue
        if isize > 224 and ichan > 32:
            continue
        if isize <= ksize:
            continue
        name = f'conv2d-is{isize}_ic{ichan}_oc{ochan}_ks{ksize}_s{stride}_p{padding}.onnx'  # noqa: E501
        simple_conv2d(
            args.output_dir / 'conv2d' / name,
            isize,
            ichan,
            ochan,
            ksize,
            stride,
            padding
        )

    simple_conv2d_add_network(
        args.output_dir / 'simple-models' / 'simple-conv2d-add.onnx'
    )

    simple_conv2d_add_network_v2(
        args.output_dir / 'simple-models' / 'simple-conv2d-add-v2.onnx'
    )
