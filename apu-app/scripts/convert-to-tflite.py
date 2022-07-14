import tensorflow as tf
import numpy as np
from tqdm import tqdm
import argparse
from onnx_tf.backend import prepare
import onnx
from pathlib import Path
from datetime import datetime
import shutil


def onnxconversion(modelpath: Path):
    onnxmodel = onnx.load(modelpath)
    initializernames = set(
        [node.name for node in onnxmodel.graph.initializer]
    )
    inputs = {}
    for graphinput in onnxmodel.graph.input:
        if graphinput.name in initializernames:
            continue
        dimensions = []
        for d in graphinput.type.tensor_type.shape.dim:
            if d.HasField('dim_value'):
                dimensions.append(int(d.dim_value))
            else:
                print('Unsupported dimension field')
                raise NotImplementedError
        inputs[graphinput.name] = dimensions
    model = prepare(onnxmodel)
    convertedpath = str(Path(modelpath).with_suffix(f'.{datetime.now().strftime("%Y%m%d-%H%M%S")}.pb'))  # noqa: E501
    model.export_graph(convertedpath)
    converter = tf.lite.TFLiteConverter.from_saved_model(convertedpath)
    return converter, inputs, convertedpath


def save_to_tflite(inputpath: Path, outputpath: Path):
    print(f'{str(inputpath)} --> {str(outputpath)}')
    outputpath.parent.mkdir(parents=True, exist_ok=True)
    converter, inputs, convertedpath = onnxconversion(str(inputpath))

    def calibration_dataset_generator():
        for _ in tqdm(range(100)):
            sampledata = {}
            for inp, dim in inputs.items():
                sampledata[inp] = np.random.normal(
                    size=dim
                ).astype(np.float32)
            yield sampledata

    converter.optimizations = [tf.lite.Optimize.DEFAULT]
    converter.representative_dataset = calibration_dataset_generator
    converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
    converter.inference_input_type = tf.int8
    converter.inference_output_type = tf.int8
    tflite_model = converter.convert()
    with open(outputpath, 'wb') as f:
        f.write(tflite_model)
    shutil.rmtree(convertedpath)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        'input',
        type=Path,
        help='ONNX input file or a directory with ONNX files'
    )
    parser.add_argument(
        'output',
        type=Path,
        help='TFLite output file or a directory where ONNX files (with same names as converted models) should be saved'  # noqa: E501
    )
    args = parser.parse_args()

    if not args.input.exists():
        print('No inputs provided')
        return -1

    if args.input.is_file():
        save_to_tflite(args.input, args.output)
    elif args.input.is_dir():
        for path in args.input.rglob('*.onnx'):
            relpath = path.relative_to(args.input).with_suffix('.tflite')
            outpath = args.output / relpath
            save_to_tflite(path, outpath)
    else:
        print('Unsupported input')

if __name__ == '__main__':
    main()
