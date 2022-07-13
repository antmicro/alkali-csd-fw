import torch
import argparse
from pathlib import Path
from torch.autograd import Variable


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


def create_simple_conv_large():
    model = torch.nn.Sequential(torch.nn.Conv2d(1, 32, 9))
    for param in model.parameters():
        param.requires_grad = False
    print(model)
    data = Variable(torch.zeros([1, 1, 256, 256]))
    print(data)
    print(model(data).numpy())
    torch.onnx.export(model, data, "conv.onnx", verbose=True)


def simple_add(out: Path, vector_length: int):
    class SimpleAdd(torch.nn.Module):
        def __init__(self):
            super(SimpleAdd, self).__init__()

        def forward(self, x, y):
            return x + y

    model = SimpleAdd()
    data1 = Variable(torch.FloatTensor([[i for i in range(vector_length)]]))
    data2 = Variable(torch.FloatTensor([[i for i in range(vector_length)]]))
    print(model)
    print(model.forward(data1, data2))
    torch.onnx.export(model, (data1, data2,), str(out), verbose=True)


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
    parser.add_argument(
        '--alu-vectors-sizes',
        type=int,
        nargs='+',
        default=[1, 4, 16, 64, 256, 1024, 4096, 8192],
        help='Sizes of ALU vectors to investigate'
    )

    args = parser.parse_args()

    args.output_dir.mkdir(parents=True, exist_ok=True)

    for size in args.alu_vectors_sizes:
        simple_add(args.output_dir / f'add-{size}.onnx', size)
