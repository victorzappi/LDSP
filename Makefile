BUILD_DIR := ./bin
SOURCES := $(wildcard ./core/*.cpp) $(wildcard ./libraries/**/*.cpp) ./libraries/Bela/Biquad/Biquad.cpp
INCLUDES := -I./include -I./libraries/tinyalsa/include -I./libraries/ -I./libraries -I.
LIBRARIES := -L./libraries/tinyalsa -lm -ltinyalsa -landroid -landroid

NDK := /opt/homebrew/share/android-ndk
HOST := darwin-x86_64
CXX := $(NDK)/toolchains/llvm/prebuilt/$(HOST)/bin/clang++

CXXFLAGS := -target armv7a-linux-androideabi24 -g $(SOURCES) -o $(BUILD_DIR)/ldsp $(INCLUDES) $(LIBRARIES) -ffast-math -static-libstdc++


all: clean build push

build:
	$(CXX) $(CXXFLAGS)

push:
	adb push $(BUILD_DIR)/ldsp /data/devel/

clean:
	@rm -rf $(BUILD_DIR)
	@mkdir $(BUILD_DIR)
