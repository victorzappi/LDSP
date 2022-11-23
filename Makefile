# Example Usage:
#  export TOOLCHAIN_PATH=/opt/homebrew/share/android-ndk/toolchains/llvm/prebuilt/darwin-x86_64
#  make VENDOR=Huawei MODEL="P8 Lite (alice)" API_LEVEL=24

# Project Layout
BUILD_DIR := ./bin
SOURCES := $(wildcard ./core/*.cpp) $(wildcard ./libraries/*/*.cpp) $(wildcard ./libraries/*/*/*.cpp)
INCLUDES := -I./include -I./libraries/tinyalsa/include -I./libraries -I.
LIBRARIES := -L./libraries/tinyalsa -lm -ltinyalsa -landroid

# Compiler Paths
ifndef TOOLCHAIN_PATH # Path to the NDK toolchain
$(error TOOLCHAIN_PATH is not set)
endif

CC := $(TOOLCHAIN_PATH)/bin/clang
CXX := $(TOOLCHAIN_PATH)/bin/clang++

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
ARCH_FULL := $(shell grep 'target architecture' "$(HW_CONFIG)" | cut -d \" -f 4)

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
ANDROID_LIB_PATH := $(TOOLCHAIN_PATH)/sysroot/usr/lib/$(ARCH_SHORT)-linux-android$(EABI)/$(API_LEVEL)

# Compilation Target
TARGET := $(ARCH_FULL)-linux-android$(EABI)$(API_LEVEL)

# Support for NEON floating-point unit
ifeq ($(shellgrep 'target architecture' "$(HW_CONFIG)" | cut -d \" -f 4), "True")
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
