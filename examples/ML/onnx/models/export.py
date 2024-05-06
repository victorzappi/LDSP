import onnx
import torch
import torch.onnx
from dummy import Dummy
from large_dummy import LargeDummy







def export_to_onnx(model):


    batch_size = 1    # just a random number

    # Initialize the model
    model.eval()


    # Input to the model
    x = torch.randn(batch_size, model.input_size, requires_grad=True)
    torch_out = model(x)
    print(torch_out)

    # Export the model
    torch.onnx.export(model,                   # model being run
                    x,                         # model input (or a tuple for multiple inputs)
                    "model.onnx",              # where to save the model (can be a file or file-like object)
                    export_params=True,        # store the trained parameter weights inside the model file
                    opset_version=10,          # the ONNX version to export the model to
                    do_constant_folding=True,  # whether to execute constant folding for optimization
                    input_names = ['input'],   # the model's input names
                    output_names = ['output'], # the model's output names
    )



if __name__ == '__main__':

    model = Dummy()

    export_to_onnx(model)

    onnx_model = onnx.load("model.onnx")

