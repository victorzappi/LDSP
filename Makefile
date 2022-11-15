# Project Layout
BUILD_DIR := ./bin
SOURCES := $(wildcard ./core/*.cpp) $(wildcard ./libraries/**/*.cpp) ./libraries/Bela/Biquad/Biquad.cpp
INCLUDES := -I./include -I./libraries/tinyalsa/include -I./libraries/ -I./libraries -I.
LIBRARIES := -L./libraries/tinyalsa -lm -ltinyalsa -landroid

# Compiler Paths
NDK := /opt/homebrew/share/android-ndk# todo: this should be controlled via environment variable
HOST_SYSTEM := $(shell uname -s | awk '{print tolower($0)}')
HOST_ARCH := x86_64# Should maybe be (uname -m), but there's no darwin-arm64 support yet so my Mac runs this through Rosetta
HOST := $(HOST_SYSTEM)-$(HOST_ARCH)
CC := $(NDK)/toolchains/llvm/prebuilt/$(HOST)/bin/clang
CXX := $(NDK)/toolchains/llvm/prebuilt/$(HOST)/bin/clang++

# Parameters
ifndef VENDOR # The vendor of the phone
$(error VENDOR is not set)
endif

ifndef MODEL # The specific model of the phone
$(error MODEL is not set)
endif

ifndef API_LEVEL # The target Android API level
$(error API_LEVEL is not set)
endif

# Hardware Config
HW_CONFIG := ./phones/$(VENDOR)/$(MODEL)/ldsp_hw_config.json

# Target Architecture
ARCH_FULL := $(shell scripts/read_config.py "$(HW_CONFIG)" "target architecture")

# ARCH_SHORT one of "arm", "aarch64", "i686", "x86_64"
# EABI is either "" or the string "eabi"
ifneq (,$(findstring arm,$(ARCH_FULL)))
	ARCH_SHORT := arm
	EABI := eabi
else
	ARCH_SHORT := $(ARCH_FULL)
	EABI :=
endif

# Android Libraries
ANDROID_LIB_PATH := $(NDK)/toolchains/llvm/prebuilt/$(HOST)/sysroot/usr/lib/$(ARCH_SHORT)-linux-android$(EABI)/$(API_LEVEL)

# Compilation Target
TARGET := $(ARCH_FULL)-linux-android$(EABI)$(API_LEVEL)

# Support for NEON floating-point unit
ifeq ($(shell scripts/read_config.py "$(HW_CONFIG)" "supports neon floating point unit"), "True")
	NEON := -mfpu=neon-fp-armv8
else
	NEON :=
endif

# Compiler Flags
CXXFLAGS := -target $(TARGET) $(NEON) -g $(SOURCES) -o $(BUILD_DIR)/ldsp $(INCLUDES) -DAPI_LEVEL="$(API_LEVEL)" -L$(ANDROID_LIB_PATH) $(LIBRARIES) -ffast-math -static-libstdc++

all: clean build

build:
	$(CXX) $(CXXFLAGS)

push:
	adb push $(BUILD_DIR)/ldsp /data/devel/

clean:
	@rm -rf $(BUILD_DIR)
	@mkdir $(BUILD_DIR)
