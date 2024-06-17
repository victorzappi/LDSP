add_library(onnxruntime SHARED IMPORTED)
set(ONNX_LIB_PATH "${CMAKE_SOURCE_DIR}/dependencies/onnxruntime/${DEVICE_ARCH}/${ONNX_VERSION}/libonnxruntime.so")
set_target_properties ( onnxruntime
                        PROPERTIES 
                        IMPORTED_LOCATION ${ONNX_LIB_PATH})
