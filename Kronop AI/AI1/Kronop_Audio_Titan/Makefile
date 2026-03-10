# Android NDK Makefile for Kronop Audio Titan
# Compile all .cpp files into a single native binary with -O3 optimization
# Targets ARM64 architecture for mobile hardware

# Set NDK path (update this to your NDK installation path)
NDK_PATH = /opt/android-ndk-r25b  # Example path; adjust as needed

# Compiler
CC = $(NDK_PATH)/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android21-clang++

# Compilation flags: -O3 for maximum optimization, -std=c++17, -fPIE for Android executables
CFLAGS = -O3 -std=c++17 -fPIE -pie -march=armv8-a+crypto -fopenmp -ffast-math

# Include paths for NDK headers
INCLUDES = -I$(NDK_PATH)/sysroot/usr/include -I$(NDK_PATH)/sysroot/usr/include/aarch64-linux-android

# Libraries: link with Android logging, math, and NEON support
LIBS = -L$(NDK_PATH)/sysroot/usr/lib/aarch64-linux-android/21 -llog -landroid -lm

# Source files
SOURCES = Hardware_Initialize.cpp Memory_Bridge.cpp Enhancement_Engine.cpp Titan_Controller.cpp Titan_Output_Saver.cpp

# Target binary name
TARGET = kronop_audio_titan

# Build rule
$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(TARGET) $(SOURCES) $(LIBS)
	@echo "Built $(TARGET) with -O3 optimization for maximum mobile hardware utilization"

# Clean rule
clean:
	rm -f $(TARGET)
	@echo "Cleaned build artifacts"

# Phony targets
.PHONY: clean
