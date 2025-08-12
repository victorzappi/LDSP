cmake_minimum_required(VERSION 3.10)
project(rapidLib)

# The version number
set (${PROJECT_NAME}_VERSION_MAJOR 2)
set (${PROJECT_NAME}_VERSION_MINOR 3)

# Main lib
include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/dependencies/bayesfilter/src)

# RapidLib
file(GLOB RAPIDLIB_SRC "${PROJECT_SOURCE_DIR}/src/*.cpp")
file(GLOB RAPIDLIB_DEP "${PROJECT_SOURCE_DIR}/dependencies/libsvm/libsvm.cpp")

# Third party
file(GLOB JSON_SRC "${PROJECT_SOURCE_DIR}/dependencies/jsoncpp.cpp")
file(GLOB BAYES_SRC "${PROJECT_SOURCE_DIR}/dependencies/bayesfilter/src/*.cpp")

# Set the source for the main library, using the groups defined above
set(RAPIDLIB_FULL_SRC 
		${RAPIDLIB_SRC}
		${RAPIDLIB_DEP}
		${JSON_SRC}
		${BAYES_SRC}
	)

# Build as static library
add_library(${PROJECT_NAME} STATIC ${RAPIDLIB_FULL_SRC})

# Make inlcude directories visible to super-projects
target_include_directories(${PROJECT_NAME}  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)