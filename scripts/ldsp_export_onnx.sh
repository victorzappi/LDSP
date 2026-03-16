#!/bin/sh
#
# Script needed when running ONNX projects from the device (e.g., via Termux)
# Exports the need library path variable 
# Needs to be called like this
# source ldsp_export_onnx.sh

export LD_LIBRARY_PATH="/data/ldsp/onnxruntime/"
echo "LD_LIBRARY_PATH set to $LD_LIBRARY_PATH"
