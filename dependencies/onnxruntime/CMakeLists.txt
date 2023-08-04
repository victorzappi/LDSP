set(ONNXRUNTIME_LIB "/Users/jhoopes/code/libraries/copies/LDSP/dependencies/libonnxruntime.so")

project(ldsp)

add_library(onnxruntime SHARED IMPORTED)
set_target_properties ( onnxruntime
                        PROPERTIES 
                        IMPORTED_LOCATION 
                        /Users/jhoopes/code/libraries/copies/LDSP/dependencies/libonnxruntime.so)
