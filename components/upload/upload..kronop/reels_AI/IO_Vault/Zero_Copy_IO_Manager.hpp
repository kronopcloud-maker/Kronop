#ifndef ZERO_COPY_IO_MANAGER_HPP
#define ZERO_COPY_IO_MANAGER_HPP

#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <filesystem>
#include <chrono>
#include <functional>
#include <map>

/**
 * Zero_Copy_IO_Manager.hpp
 * Zero-Copy Architecture with Memory Mapping
 * Eliminates RAM buffering for mobile safety - Direct disk access only
 */

namespace fs = std::filesystem;

struct MappedRegion {
    void* address;
    size_t size;
    size_t offset;
    bool isWriteable;
    std::string filePath;
    int fd;
};

struct BlockWriteRequest {
    std::string workerId;
    int chunkIndex;
    void* data;
    size_t dataSize;
    size_t diskOffset;
    std::string targetPath;
    std::function<void(bool)> callback;
};

class Zero_Copy_IO_Manager {
private:
    // Memory-mapped files registry
    std::map<std::string, MappedRegion> mappedRegions_;
    std::mutex regionsMutex_;
    
    // Block-level writing system
    std::vector<BlockWriteRequest> pendingWrites_;
    std::mutex writesMutex_;
    std::condition_variable writesCondition_;
    
    // Writer thread for sequential block writes
    std::thread blockWriterThread_;
    std::atomic<bool> writerActive_;
    
    // Memory monitoring (Low Memory Footprint)
    const size_t MAX_MEMORY_USAGE = 200 * 1024 * 1024;  // 200MB limit
    std::atomic<size_t> currentMemoryUsage_;
    std::thread memoryMonitorThread_;
    std::atomic<bool> memoryMonitorActive_;
    
    // Disk space monitoring
    const size_t MIN_DISK_SPACE = 2ULL * 1024 * 1024 * 1024;  // 2GB minimum
    std::string workingDirectory_;
    
    // Statistics
    std::atomic<size_t> totalBytesMapped_;
    std::atomic<size_t> totalBytesWritten_;
    std::atomic<int> activeMappings_;
    
    // Callbacks for integration
    std::function<void()> memoryPressureCallback_;
    std::function<void(const std::string&)> alertCallback_;

public:
    Zero_Copy_IO_Manager();
    ~Zero_Copy_IO_Manager();
    
    // Initialization
    bool initialize(const std::string& workingDirectory);
    void shutdown();
    
    // Zero-Copy Memory Mapping (mmap)
    MappedRegion* mapFileForReading(const std::string& filePath);
    MappedRegion* mapFileForWriting(const std::string& filePath, size_t expectedSize);
    void unmapFile(const std::string& filePath);
    bool resizeMappedFile(const std::string& filePath, size_t newSize);
    
    // Direct Block-Level Writing (No RAM buffering)
    bool queueBlockWrite(const BlockWriteRequest& request);
    bool writeDirectToDisk(const std::string& workerId, int chunkIndex, 
                          void* data, size_t dataSize, size_t diskOffset);
    bool writeChunkDirect(const std::string& workerId, int chunkIndex, 
                         void* data, size_t dataSize);
    
    // Memory management (Low Memory Footprint)
    size_t getCurrentMemoryUsage() const { return currentMemoryUsage_; }
    size_t getMaxMemoryUsage() const { return MAX_MEMORY_USAGE; }
    bool isMemoryPressureHigh() const;
    void forceMemoryCleanup();
    
    // File operations with zero-copy
    bool createSparseFile(const std::string& filePath, size_t size);
    bool allocateFileSpace(const std::string& filePath, size_t size);
    size_t getFileSize(const std::string& filePath);
    
    // Status and monitoring
    int getActiveMappingsCount() const { return activeMappings_; }
    size_t getTotalBytesMapped() const { return totalBytesMapped_; }
    size_t getTotalBytesWritten() const { return totalBytesWritten_; }
    double getMemoryEfficiency() const;
    
    // Callbacks for system integration
    void setMemoryPressureCallback(std::function<void()> callback) { 
        memoryPressureCallback_ = callback; 
    }
    void setAlertCallback(std::function<void(const std::string&)> callback) { 
        alertCallback_ = callback; 
    }
    
    // Advanced operations
    bool syncMappedFile(const std::string& filePath);
    bool prefetchFileRegion(const std::string& filePath, size_t offset, size_t size);
    void adviseMemoryUsage(const std::string& filePath, int advice);

private:
    // Core block writer thread
    void blockWriterLoop();
    bool processBlockWrite(const BlockWriteRequest& request);
    
    // Memory mapping operations
    void* mapFileDescriptor(int fd, size_t size, bool writeable);
    bool unmapRegion(MappedRegion& region);
    
    // Memory monitoring
    void memoryMonitorLoop();
    void updateMemoryUsage(size_t delta);
    bool checkMemoryPressure();
    void handleMemoryPressure();
    
    // Disk operations
    bool checkDiskSpace();
    size_t getAvailableDiskSpace();
    
    // Utility methods
    std::string getWorkerChunkPath(const std::string& workerId, int chunkIndex);
    std::string getChunkTempPath(const std::string& workerId, int chunkIndex);
    bool ensureDirectoryExists(const std::string& filePath);
    void sendAlert(const std::string& message);
    
    // Memory optimization
    void cleanupUnusedMappings();
    void compactMemoryUsage();
    
    // Error handling
    void handleMappingError(const std::string& filePath, const std::string& error);
    void handleWriteError(const BlockWriteRequest& request, const std::string& error);
    
    // Logging
    void logMemoryOperation(const std::string& operation, size_t size);
    void logBlockWrite(const BlockWriteRequest& request, bool success);
};

#endif // ZERO_COPY_IO_MANAGER_HPP
