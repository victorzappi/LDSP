# Example Usage:
#  export TOOLCHAIN_PATH=/opt/homebrew/share/android-ndk/toolchains/llvm/prebuilt/darwin-x86_64
#  make VENDOR=Huawei MODEL="P8 Lite (alice)" ANDROID_VERSION=7.0

# Phone Parameters
ifdef VENDOR # The vendor of the phone
ifdef MODEL # The specific model of the phone

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

# Project Layout
BUILD_DIR := ./bin/$(VENDOR)/$(MODEL)
OBJECT_DIR := ./obj/$(VENDOR)/$(MODEL)

# Android Version
ifdef ANDROID_VERSION

# compute API_LEVEL

ANDROID_MAJOR := $(shell echo $(ANDROID_VERSION) | cut -d . -f 1)
ANDROID_MINOR := $(shell echo $(ANDROID_VERSION) | cut -d . -f 2)
ANDROID_PATCH := $(shell echo $(ANDROID_VERSION) | cut -d . -f 3)

include ./android-versions.mk

$(info Detected Android API level $(API_LEVEL))

# Compilation Target
TARGET := $(ARCH_FULL)-linux-android$(EABI)$(API_LEVEL)

# Support for NEON floating-point unit
NEON_SUPPORT := $(shell grep 'supports neon floating point unit' "$(HW_CONFIG)" | cut -d \" -f 4)

ifeq ($(ARCH_FULL),aarch64) # aarch64 had neon active by default, no need to set flag
	NEON :=
else ifneq (,$(findstring $(NEON_SUPPORT), true yes 1 True Yes))
	NEON := -mfpu=neon-fp16
endif

# Compiler Paths
ifdef TOOLCHAIN_PATH

CC := $(TOOLCHAIN_PATH)/bin/clang
CXX := $(TOOLCHAIN_PATH)/bin/clang++

# Android Libraries
ANDROID_LIB_PATH := $(TOOLCHAIN_PATH)/sysroot/usr/lib/$(ARCH_SHORT)-linux-android$(EABI)/$(API_LEVEL)

# Project Layout
INCLUDES := -I./include -I./libraries/tinyalsa/include -I./libraries -I.
LIBRARIES := -lm -landroid -static-libstdc++ -L$(ANDROID_LIB_PATH)
CSOURCES := $(wildcard core/*.c) $(wildcard libraries/*/*.c) $(wildcard libraries/*/*/*.c)
CPPSOURCES := $(wildcard core/*.cpp) $(wildcard libraries/*/*.cpp) $(wildcard libraries/*/*/*.cpp)

# We can't use quotes to escape makefile rule names, so we backslash-escape all the spaces
OBJECT_DIR_ESCAPED := $(subst $() ,\ ,$(OBJECT_DIR))
# We define OBJECT_RULES with the escaping needed to reference the .o makefile rules
# i.e., spaces backslash-escaped, parens not escaped
OBJECT_RULES := $(addprefix $(OBJECT_DIR_ESCAPED)/, $(CSOURCES:.c=.o) $(CPPSOURCES:.cpp=.o))
# We define OBJECT_PATHS with the escaping needed to pass the list of .o files to the linker
# i.e., each path surrounded by quotes, no other escaping done
OBJECT_PATHS := $(addprefix "$(OBJECT_DIR)/, $(addsuffix ", $(CSOURCES:.c=.o) $(CPPSOURCES:.cpp=.o)))

# Compiler Flags
CCFLAGS := -target $(TARGET) $(NEON) -ffast-math
CPPFLAGS := $(INCLUDES) -DAPI_LEVEL="$(API_LEVEL)"
CXXFLAGS := -target $(TARGET) $(NEON) -ffast-math

# We can't use $(dir ...) because it splits on spaces (even escaped ones :/)
# so we use `dirname` instead
$(OBJECT_DIR_ESCAPED)/%.o: %.c
	@mkdir -p "$(shell dirname "$@")"
	$(CC) $(CPPFLAGS) $(CCFLAGS) -c "$^" -o "$@"

$(OBJECT_DIR_ESCAPED)/%.o: %.cpp
	@mkdir -p "$(shell dirname "$@")"
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c "$^" -o "$@"

build: $(OBJECT_RULES)
	@mkdir  -p "$(BUILD_DIR)"
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o "$(BUILD_DIR)/ldsp" $(OBJECT_PATHS) $(LIBRARIES)

else
build:
	@echo "TOOLCHAIN_PATH is not set"
endif
else
build:
	@echo "API_LEVEL is not set"
endif

push:
	@adb root
	@adb push "$(HW_CONFIG)" /data/ldsp/ldsp_hw_config.json
	@adb push "$(BUILD_DIR)/ldsp" /data/ldsp/ldsp

push_shell:
  @adb push "$(HW_CONFIG)" /sdcard/ldsp/ldsp_hw_config.json
  @adb push "$(BUILD_DIR)/ldsp" /sdcard/ldsp/ldsp

clean:
	@rm -rf "$(BUILD_DIR)"
	@rm -rf "$(OBJECT_DIR)"

else
build:
	@echo "MODEL is not set"
push:
	@echo "MODEL is not set"
push_shell:
	@echo "MODEL is not set"
clean:
	@echo "MODEL is not set"
endif
else
build:
	@echo "VENDOR is not set"
push:
	@echo "VENDOR is not set"
push_shell:
	@echo "VENDOR is not set"
clean:
	@echo "VENDOR is not set"
endif

cleanAll:
	@rm -rf ./bin
	@rm -rf ./obj

run:
	adb shell "cd /data/ldsp/ && ./ldsp"

.PHONY: build push push_shell clean cleanAll run
