
project(ldsp)

add_library(onnxruntime SHARED IMPORTED)
set_target_properties ( onnxruntime
                        PROPERTIES 
                        IMPORTED_LOCATION 
                        ${CMAKE_SOURCE_DIR}/dependencies/onnxruntime/arm7/libonnxruntime.so)
