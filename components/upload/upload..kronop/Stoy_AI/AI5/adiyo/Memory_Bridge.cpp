#include <memory>
#include <atomic>
#include <iostream>
#include <cstring>
#include <android/log.h>

#define LOG_TAG "Memory_Bridge"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

class MemoryBridge {
private:
    // Aligned buffer for 64-byte cache line alignment (optimal for mobile CPUs)
    void* alignedBuffer;
    size_t bufferSize;
    size_t alignment = 64;  // 64-byte alignment for L1/L2 cache efficiency

    // Ring buffer parameters
    std::atomic<size_t> writePos;
    std::atomic<size_t> readPos;
    size_t chunkSize;
    size_t ringBufferSize;

    // DMA-like zero-copy access (simulated for mobile RAM-CPU)
    void* dmaBuffer;

public:
    MemoryBridge(size_t totalSize, size_t audioChunkSize)
        : bufferSize(totalSize), chunkSize(audioChunkSize), writePos(0), readPos(0) {

        // Allocate 64-byte aligned buffer for maximum cache performance
        alignedBuffer = std::aligned_alloc(alignment, bufferSize);
        if (!alignedBuffer) {
            LOGI("Failed to allocate aligned buffer");
            return;
        }

        // Initialize DMA buffer (zero-copy access to RAM)
        dmaBuffer = alignedBuffer;
        ringBufferSize = bufferSize / chunkSize;

        LOGI("Memory Bridge initialized: %zu bytes aligned buffer, %zu chunk size", bufferSize, chunkSize);
    }

    ~MemoryBridge() {
        if (alignedBuffer) {
            std::free(alignedBuffer);
        }
    }

    // DMA write: Direct memory access write (no CPU wait)
    bool dmaWriteAudioChunk(const void* audioData, size_t size) {
        if (size != chunkSize) return false;

        size_t nextWritePos = (writePos.load() + 1) % ringBufferSize;

        // Check if buffer is full
        if (nextWritePos == readPos.load()) {
            LOGI("Ring buffer full - audio lag prevention");
            return false;  // Buffer full, prevent lag by not overwriting
        }

        // DMA-like copy: direct memory transfer
        void* dest = static_cast<char*>(dmaBuffer) + (writePos.load() * chunkSize);
        std::memcpy(dest, audioData, size);

        writePos.store(nextWritePos);
        return true;
    }

    // DMA read: Direct memory access read (no CPU wait)
    bool dmaReadAudioChunk(void* audioData, size_t size) {
        if (size != chunkSize) return false;

        if (writePos.load() == readPos.load()) {
            return false;  // Buffer empty
        }

        // DMA-like copy: direct memory transfer
        void* src = static_cast<char*>(dmaBuffer) + (readPos.load() * chunkSize);
        std::memcpy(audioData, src, size);

        readPos.store((readPos.load() + 1) % ringBufferSize);
        return true;
    }

    // Get buffer utilization for performance monitoring
    float getBufferUtilization() {
        size_t used = (writePos.load() - readPos.load() + ringBufferSize) % ringBufferSize;
        return static_cast<float>(used) / ringBufferSize;
    }

    // Flush ring buffer (emergency clear)
    void flush() {
        writePos.store(0);
        readPos.store(0);
        std::memset(dmaBuffer, 0, bufferSize);
        LOGI("Ring buffer flushed");
    }

    // Check if buffer has data available
    bool hasData() {
        return writePos.load() != readPos.load();
    }
};

// Global memory bridge instance for audio processing
static MemoryBridge* audioMemoryBridge = nullptr;

// Initialize memory bridge for audio
extern "C" bool initializeAudioMemoryBridge(size_t bufferSize, size_t chunkSize) {
    if (audioMemoryBridge) delete audioMemoryBridge;
    audioMemoryBridge = new MemoryBridge(bufferSize, chunkSize);
    return true;
}

// Get memory bridge instance
extern "C" MemoryBridge* getAudioMemoryBridge() {
    return audioMemoryBridge;
}
