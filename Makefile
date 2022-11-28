# Example Usage:
#  export TOOLCHAIN_PATH=/opt/homebrew/share/android-ndk/toolchains/llvm/prebuilt/darwin-x86_64
#  make VENDOR=Huawei MODEL="P8 Lite (alice)" API_LEVEL=24

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

# Project Layout
BUILD_DIR := ./bin
INCLUDES := -I./include -I./libraries/tinyalsa/include -I./libraries -I.
LIBRARIES := -lm -landroid -static-libstdc++ -L$(ANDROID_LIB_PATH)
CSOURCES := $(wildcard core/*.c) $(wildcard libraries/*/*.c) $(wildcard libraries/*/*/*.c)
CPPSOURCES := $(wildcard core/*.cpp) $(wildcard libraries/*/*.cpp) $(wildcard libraries/*/*/*.cpp)
OBJECT_DIR = obj
OBJECTS := $(addprefix $(OBJECT_DIR)/, $(CSOURCES:.c=.o) $(CPPSOURCES:.cpp=.o))

# Compiler Flags
CCFLAGS := -target $(TARGET) $(NEON) -ffast-math
CPPFLAGS := $(INCLUDES) -DAPI_LEVEL="$(API_LEVEL)"
CXXFLAGS := -target $(TARGET) $(NEON) -ffast-math

build: $(OBJECTS)
	@mkdir  -p $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $(BUILD_DIR)/ldsp $^ $(LIBRARIES)

obj/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CCFLAGS) -c $^ -o $@

obj/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $^ -o $@

.PHONY: push clean

push:
	adb push $(BUILD_DIR)/ldsp /data/devel/

clean:
	@rm -rf $(BUILD_DIR)
	@rm -rf $(OBJECT_DIR)
