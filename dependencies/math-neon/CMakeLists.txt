# CMakeLists.txt made to integrate math-neon into LDSP with inline conversion function

cmake_minimum_required(VERSION 3.5)
project(math_neon_static C)

# ------------------------------------------------------------------------
# Conversion function: GAS -> Clang-compatible NEON assembly for Android
# ------------------------------------------------------------------------
function(convert_gas gas_files android_files)
  set(android_tmp)
  foreach(gf IN LISTS ${gas_files})
    get_filename_component(fname ${gf} NAME_WE)
    set(out "${CMAKE_CURRENT_BINARY_DIR}/${fname}_android.neon.s")
    list(APPEND android_tmp "${out}")
    add_custom_command(
      OUTPUT "${out}"
      COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/tools/gas2android_convert.py"
              "${gf}" "${out}"
      DEPENDS "${gf}"
      COMMENT "Converting ${fname}.neon.s for Android"
    )
  endforeach()
  set(${android_files} "${android_tmp}" PARENT_SCOPE)
endfunction()

# ------------------------------------------------------------------------
# Collect sources
# ------------------------------------------------------------------------
# All C source files (pure-C implementations and soft/hard float stubs)
file(GLOB NEON_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

# Exclude the debug‐only file
list(REMOVE_ITEM NEON_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/math_debug.c")

# All assembly sources (.neon.s) for NEON kernels
file(GLOB ASM_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.neon.s")

# Convert assembly files to Clang-compatible on ARM32 soft-float
convert_gas(ASM_SOURCES CONV_ASM)

# ------------------------------------------------------------------------
# Build static library
# ------------------------------------------------------------------------
add_library(mathneon STATIC
  ${NEON_SOURCES}
  ${CONV_ASM}
)

# ------------------------------------------------------------------------
# Public include directory
# ------------------------------------------------------------------------
target_include_directories(mathneon PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# ------------------------------------------------------------------------
# Silence “missing return” warning only for mathneon 
# ------------------------------------------------------------------------
#VIC several assembly implementations of math-neon hfp functions have no 
# return statement for the C compiler, yet they are totally safe, due to 
# how data is moved around in registers. so we silence the warnings here
# i assume andrepuschmann did this to really minimize the number of 
# assembly operations wihthin the functions!
# NOTE: this does not hold up for sfp implementations! i had to patch them!
target_compile_options(mathneon PRIVATE -Wno-return-type)