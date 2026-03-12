#include "Zero_Copy_IO_Manager.hpp"
#include <iostream>
#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <errno.h>

Zero_Copy_IO_Manager::Zero_Copy_IO_Manager() 
    : writerActive_(false), memoryMonitorActive_(false),
      currentMemoryUsage_(0), totalBytesMapped_(0), totalBytesWritten_(0),
      activeMappings_(0) {
    
    std::cout << "🚀 Zero_Copy_IO_Manager initializing - Zero-Copy Architecture" << std::endl;
    std::cout << "   Memory Limit: " << (MAX_MEMORY_USAGE / 1024 / 1024) << "MB" << std::endl;
    std::cout << "   Disk Threshold: " << (MIN_DISK_SPACE / 1024 / 1024 / 1024) << "GB" << std::endl;
    std::cout << "   No RAM Buffering: Enabled" << std::endl;
    std::cout << "   Memory Mapping: Enabled" << std::endl;
    std::cout << "   Block-Level Writing: Enabled" << std::endl;
}

Zero_Copy_IO_Manager::~Zero_Copy_IO_Manager() {
    shutdown();
    std::cout << "🚀 Zero_Copy_IO_Manager destroyed" << std::endl;
}

bool Zero_Copy_IO_Manager::initialize(const std::string& workingDirectory) {
    workingDirectory_ = workingDirectory;
    
    // Create working directory if it doesn't exist
    if (!ensureDirectoryExists(workingDirectory)) {
        std::cerr << "❌ Failed to create working directory: " << workingDirectory << std::endl;
        return false;
    }
    
    // Check initial disk space
    if (!checkDiskSpace()) {
        sendAlert("CRITICAL: Insufficient disk space for operation");
        return false;
    }
    
    // Start block writer thread (No RAM buffering)
    writerActive_ = true;
    blockWriterThread_ = std::thread(&Zero_Copy_IO_Manager::blockWriterLoop, this);
    
    // Start memory monitor thread (Low Memory Footprint)
    memoryMonitorActive_ = true;
    memoryMonitorThread_ = std::thread(&Zero_Copy_IO_Manager::memoryMonitorLoop, this);
    
    std::cout << "✅ Zero_Copy_IO_Manager initialized" << std::endl;
    std::cout << "   Working Directory: " << workingDirectory << std::endl;
    std::cout << "   Available Disk Space: " << (getAvailableDiskSpace() / 1024 / 1024 / 1024) << "GB" << std::endl;
    std::cout << "   Block Writer Thread: Started" << std::endl;
    std::cout << "   Memory Monitor Thread: Started" << std::endl;
    
    return true;
}

void Zero_Copy_IO_Manager::shutdown() {
    // Stop block writer thread
    writerActive_ = false;
    writesCondition_.notify_all();
    if (blockWriterThread_.joinable()) {
        blockWriterThread_.join();
    }
    
    // Stop memory monitor thread
    memoryMonitorActive_ = false;
    if (memoryMonitorThread_.joinable()) {
        memoryMonitorThread_.join();
    }
    
    // Unmap all regions (Zero-Copy cleanup)
    std::lock_guard<std::mutex> lock(regionsMutex_);
    for (auto& pair : mappedRegions_) {
        unmapRegion(pair.second);
        close(pair.second.fd);
    }
    mappedRegions_.clear();
    activeMappings_ = 0;
    currentMemoryUsage_ = 0;
    
    std::cout << "🛑 Zero_Copy_IO_Manager shutdown complete" << std::endl;
}

// Zero-Copy Memory Mapping (mmap)
MappedRegion* Zero_Copy_IO_Manager::mapFileForReading(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(regionsMutex_);
    
    // Check if already mapped
    if (mappedRegions_.find(filePath) != mappedRegions_.end()) {
        return &mappedRegions_[filePath];
    }
    
    // Check file exists and get size
    struct stat fileStat;
    if (stat(filePath.c_str(), &fileStat) != 0) {
        std::cerr << "❌ File not found for mapping: " << filePath << std::endl;
        return nullptr;
    }
    
    size_t fileSize = fileStat.st_size;
    
    // Check memory pressure (Low Memory Footprint)
    if (currentMemoryUsage_ + fileSize > MAX_MEMORY_USAGE) {
        handleMemoryPressure();
        if (currentMemoryUsage_ + fileSize > MAX_MEMORY_USAGE) {
            sendAlert("WARNING: Cannot map file - memory pressure too high");
            return nullptr;
        }
    }
    
    // Open file
    int fd = open(filePath.c_str(), O_RDONLY);
    if (fd == -1) {
        std::cerr << "❌ Failed to open file for reading: " << filePath << std::endl;
        return nullptr;
    }
    
    // Map file (Zero-Copy - no RAM copying)
    void* address = mapFileDescriptor(fd, fileSize, false);
    if (!address) {
        close(fd);
        return nullptr;
    }
    
    // Create mapped region
    MappedRegion region;
    region.address = address;
    region.size = fileSize;
    region.offset = 0;
    region.isWriteable = false;
    region.filePath = filePath;
    region.fd = fd;
    
    mappedRegions_[filePath] = region;
    activeMappings_++;
    updateMemoryUsage(fileSize);
    totalBytesMapped_ += fileSize;
    
    std::cout << "🗺️ Mapped file for reading: " << filePath 
              << " (Size: " << (fileSize / 1024 / 1024) << "MB)" << std::endl;
    
    return &mappedRegions_[filePath];
}

MappedRegion* Zero_Copy_IO_Manager::mapFileForWriting(const std::string& filePath, size_t expectedSize) {
    std::lock_guard<std::mutex> lock(regionsMutex_);
    
    // Check if already mapped
    if (mappedRegions_.find(filePath) != mappedRegions_.end()) {
        return &mappedRegions_[filePath];
    }
    
    // Check memory pressure
    if (currentMemoryUsage_ + expectedSize > MAX_MEMORY_USAGE) {
        handleMemoryPressure();
        if (currentMemoryUsage_ + expectedSize > MAX_MEMORY_USAGE) {
            sendAlert("WARNING: Cannot map file for writing - memory pressure");
            return nullptr;
        }
    }
    
    // Create sparse file first
    if (!createSparseFile(filePath, expectedSize)) {
        return nullptr;
    }
    
    // Open file for writing
    int fd = open(filePath.c_str(), O_RDWR);
    if (fd == -1) {
        std::cerr << "❌ Failed to open file for writing: " << filePath << std::endl;
        return nullptr;
    }
    
    // Map file (Zero-Copy)
    void* address = mapFileDescriptor(fd, expectedSize, true);
    if (!address) {
        close(fd);
        return nullptr;
    }
    
    // Create mapped region
    MappedRegion region;
    region.address = address;
    region.size = expectedSize;
    region.offset = 0;
    region.isWriteable = true;
    region.filePath = filePath;
    region.fd = fd;
    
    mappedRegions_[filePath] = region;
    activeMappings_++;
    updateMemoryUsage(expectedSize);
    totalBytesMapped_ += expectedSize;
    
    std::cout << "🗺️ Mapped file for writing: " << filePath 
              << " (Expected Size: " << (expectedSize / 1024 / 1024) << "MB)" << std::endl;
    
    return &mappedRegions_[filePath];
}

void Zero_Copy_IO_Manager::unmapFile(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(regionsMutex_);
    
    auto it = mappedRegions_.find(filePath);
    if (it != mappedRegions_.end()) {
        MappedRegion& region = it->second;
        
        // Sync to disk if writeable
        if (region.isWriteable) {
            msync(region.address, region.size, MS_SYNC);
        }
        
        // Unmap and close
        unmapRegion(region);
        close(region.fd);
        
        updateMemoryUsage(-region.size);
        activeMappings_--;
        
        mappedRegions_.erase(it);
        
        std::cout << "🗺️ Unmapped file: " << filePath << std::endl;
    }
}

bool Zero_Copy_IO_Manager::resizeMappedFile(const std::string& filePath, size_t newSize) {
    std::lock_guard<std::mutex> lock(regionsMutex_);
    
    auto it = mappedRegions_.find(filePath);
    if (it == mappedRegions_.end()) {
        return false;
    }
    
    MappedRegion& region = it->second;
    
    // Check memory pressure
    size_t sizeDiff = (newSize > region.size) ? (newSize - region.size) : 0;
    if (sizeDiff > 0 && currentMemoryUsage_ + sizeDiff > MAX_MEMORY_USAGE) {
        handleMemoryPressure();
        if (currentMemoryUsage_ + sizeDiff > MAX_MEMORY_USAGE) {
            return false;
        }
    }
    
    // Unmap current region
    unmapRegion(region);
    
    // Resize file
    if (ftruncate(region.fd, newSize) != 0) {
        std::cerr << "❌ Failed to resize file: " << filePath << std::endl;
        return false;
    }
    
    // Remap with new size
    void* newAddress = mapFileDescriptor(region.fd, newSize, region.isWriteable);
    if (!newAddress) {
        return false;
    }
    
    // Update region
    size_t oldSize = region.size;
    region.address = newAddress;
    region.size = newSize;
    
    // Update memory usage
    if (newSize > oldSize) {
        updateMemoryUsage(newSize - oldSize);
    } else {
        updateMemoryUsage(-(oldSize - newSize));
    }
    
    std::cout << "🔄 Resized mapped file: " << filePath 
              << " (" << (oldSize / 1024 / 1024) << "MB → " << (newSize / 1024 / 1024) << "MB)" << std::endl;
    
    return true;
}

// Direct Block-Level Writing (No RAM buffering)
bool Zero_Copy_IO_Manager::queueBlockWrite(const BlockWriteRequest& request) {
    // Check disk space
    if (!hasEnoughSpace(request.dataSize)) {
        sendAlert("WARNING: Low disk space - rejecting block write");
        return false;
    }
    
    std::lock_guard<std::mutex> lock(writesMutex_);
    pendingWrites_.push_back(request);
    writesCondition_.notify_one();
    
    logBlockWrite(request, false);
    return true;
}

bool Zero_Copy_IO_Manager::writeDirectToDisk(const std::string& workerId, int chunkIndex, 
                                           void* data, size_t dataSize, size_t diskOffset) {
    BlockWriteRequest request;
    request.workerId = workerId;
    request.chunkIndex = chunkIndex;
    request.data = data;
    request.dataSize = dataSize;
    request.diskOffset = diskOffset;
    request.targetPath = getWorkerChunkPath(workerId, chunkIndex);
    request.callback = nullptr;
    
    return queueBlockWrite(request);
}

bool Zero_Copy_IO_Manager::writeChunkDirect(const std::string& workerId, int chunkIndex, 
                                          void* data, size_t dataSize) {
    // Create temporary file for this chunk
    std::string tempPath = getChunkTempPath(workerId, chunkIndex);
    
    // Use direct I/O for block-level writing
    int fd = open(tempPath.c_str(), O_CREAT | O_WRONLY | O_DIRECT, 0644);
    if (fd == -1) {
        std::cerr << "❌ Failed to open file for direct I/O: " << tempPath << std::endl;
        return false;
    }
    
    // Write directly to disk blocks (No RAM buffering)
    ssize_t written = write(fd, data, dataSize);
    close(fd);
    
    if (written == -1 || static_cast<size_t>(written) != dataSize) {
        std::cerr << "❌ Direct I/O write failed for chunk: " << workerId << "_" << chunkIndex << std::endl;
        return false;
    }
    
    totalBytesWritten_ += dataSize;
    std::cout << "💾 Direct block write: " << workerId << "_chunk_" << chunkIndex 
              << " (" << (dataSize / 1024 / 1024) << "MB)" << std::endl;
    
    return true;
}

// Memory management (Low Memory Footprint)
bool Zero_Copy_IO_Manager::isMemoryPressureHigh() const {
    return currentMemoryUsage_ > (MAX_MEMORY_USAGE * 0.8);  // 80% threshold
}

void Zero_Copy_IO_Manager::forceMemoryCleanup() {
    std::lock_guard<std::mutex> lock(regionsMutex_);
    cleanupUnusedMappings();
    compactMemoryUsage();
}

// File operations with zero-copy
bool Zero_Copy_IO_Manager::createSparseFile(const std::string& filePath, size_t size) {
    ensureDirectoryExists(filePath);
    
    int fd = open(filePath.c_str(), O_CREAT | O_RDWR, 0644);
    if (fd == -1) {
        std::cerr << "❌ Failed to create sparse file: " << filePath << std::endl;
        return false;
    }
    
    // Allocate space without writing zeros (sparse file)
    if (fallocate(fd, FALLOC_FL_ZERO_RANGE, 0, size) != 0) {
        // Fallback to ftruncate if fallocate not supported
        if (ftruncate(fd, size) != 0) {
            close(fd);
            return false;
        }
    }
    
    close(fd);
    return true;
}

bool Zero_Copy_IO_Manager::allocateFileSpace(const std::string& filePath, size_t size) {
    int fd = open(filePath.c_str(), O_RDWR);
    if (fd == -1) {
        return false;
    }
    
    // Pre-allocate space for performance
    int result = fallocate(fd, 0, 0, size);
    close(fd);
    
    return result == 0;
}

size_t Zero_Copy_IO_Manager::getFileSize(const std::string& filePath) {
    struct stat fileStat;
    if (stat(filePath.c_str(), &fileStat) == 0) {
        return fileStat.st_size;
    }
    return 0;
}

// Advanced operations
bool Zero_Copy_IO_Manager::syncMappedFile(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(regionsMutex_);
    
    auto it = mappedRegions_.find(filePath);
    if (it != mappedRegions_.end() && it->second.isWriteable) {
        return msync(it->second.address, it->second.size, MS_SYNC) == 0;
    }
    
    return false;
}

bool Zero_Copy_IO_Manager::prefetchFileRegion(const std::string& filePath, size_t offset, size_t size) {
    std::lock_guard<std::mutex> lock(regionsMutex_);
    
    auto it = mappedRegions_.find(filePath);
    if (it != mappedRegions_.end()) {
        // Use madvise for prefetching
        return madvise(static_cast<char*>(it->second.address) + offset, size, MADV_WILLNEED) == 0;
    }
    
    return false;
}

void Zero_Copy_IO_Manager::adviseMemoryUsage(const std::string& filePath, int advice) {
    std::lock_guard<std::mutex> lock(regionsMutex_);
    
    auto it = mappedRegions_.find(filePath);
    if (it != mappedRegions_.end()) {
        madvise(it->second.address, it->second.size, advice);
    }
}

// Private methods

void Zero_Copy_IO_Manager::blockWriterLoop() {
    std::cout << "💾 Block writer thread started - Zero-Copy Architecture" << std::endl;
    
    while (writerActive_) {
        std::unique_lock<std::mutex> lock(writesMutex_);
        
        // Wait for write requests
        writesCondition_.wait(lock, [this] { return !pendingWrites_.empty() || !writerActive_; });
        
        if (!writerActive_) break;
        
        // Process block writes sequentially (no race conditions)
        while (!pendingWrites_.empty()) {
            BlockWriteRequest request = pendingWrites_.front();
            pendingWrites_.erase(pendingWrites_.begin());
            lock.unlock();
            
            // Process the direct block write
            bool success = processBlockWrite(request);
            
            // Execute callback if provided
            if (request.callback) {
                request.callback(success);
            }
            
            lock.lock();
        }
    }
    
    std::cout << "💾 Block writer thread stopped" << std::endl;
}

bool Zero_Copy_IO_Manager::processBlockWrite(const BlockWriteRequest& request) {
    // Use direct I/O for block-level writing
    int flags = O_CREAT | O_WRONLY;
    
    // Try direct I/O if data is aligned and size is multiple of block size
    if (request.dataSize % 512 == 0 && reinterpret_cast<uintptr_t>(request.data) % 512 == 0) {
        flags |= O_DIRECT;
    }
    
    int fd = open(request.targetPath.c_str(), flags, 0644);
    if (fd == -1) {
        handleWriteError(request, "Failed to open file");
        return false;
    }
    
    // Seek to offset if specified
    if (request.diskOffset > 0) {
        if (lseek(fd, request.diskOffset, SEEK_SET) == -1) {
            close(fd);
            handleWriteError(request, "Failed to seek to offset");
            return false;
        }
    }
    
    // Write directly to disk blocks (Zero-Copy)
    ssize_t written = write(fd, request.data, request.dataSize);
    close(fd);
    
    if (written == -1 || static_cast<size_t>(written) != request.dataSize) {
        handleWriteError(request, "Write operation failed");
        return false;
    }
    
    totalBytesWritten_ += request.dataSize;
    logBlockWrite(request, true);
    
    return true;
}

void* Zero_Copy_IO_Manager::mapFileDescriptor(int fd, size_t size, bool writeable) {
    int prot = PROT_READ;
    if (writeable) {
        prot |= PROT_WRITE;
    }
    
    void* address = mmap(nullptr, size, prot, MAP_SHARED, fd, 0);
    
    if (address == MAP_FAILED) {
        std::cerr << "❌ mmap failed: " << strerror(errno) << std::endl;
        return nullptr;
    }
    
    return address;
}

bool Zero_Copy_IO_Manager::unmapRegion(MappedRegion& region) {
    if (region.address && region.address != MAP_FAILED) {
        if (munmap(region.address, region.size) == -1) {
            std::cerr << "❌ munmap failed: " << strerror(errno) << std::endl;
            return false;
        }
        region.address = nullptr;
    }
    return true;
}

void Zero_Copy_IO_Manager::memoryMonitorLoop() {
    std::cout << "🧠 Memory monitor thread started - Low Memory Footprint" << std::endl;
    
    while (memoryMonitorActive_) {
        std::this_thread::sleep_for(std::chrono::seconds(10));  // Check every 10 seconds
        
        if (!memoryMonitorActive_) break;
        
        // Check memory pressure
        if (checkMemoryPressure()) {
            handleMemoryPressure();
        }
        
        // Log memory usage periodically
        static auto lastLog = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::minutes>(now - lastLog).count() >= 1) {
            std::cout << "🧠 Memory Usage: " << (currentMemoryUsage_ / 1024 / 1024) << "MB / " 
                      << (MAX_MEMORY_USAGE / 1024 / 1024) << "MB (" << activeMappings_ << " mappings)" << std::endl;
            lastLog = now;
        }
    }
    
    std::cout << "🧠 Memory monitor thread stopped" << std::endl;
}

void Zero_Copy_IO_Manager::updateMemoryUsage(size_t delta) {
    if (delta > 0) {
        currentMemoryUsage_ += delta;
    } else {
        size_t absDelta = -delta;
        if (absDelta <= currentMemoryUsage_) {
            currentMemoryUsage_ -= absDelta;
        } else {
            currentMemoryUsage_ = 0;
        }
    }
}

bool Zero_Copy_IO_Manager::checkMemoryPressure() {
    return currentMemoryUsage_ > (MAX_MEMORY_USAGE * 0.7);  // 70% threshold
}

void Zero_Copy_IO_Manager::handleMemoryPressure() {
    std::cout << "⚠️ Memory pressure detected - triggering cleanup" << std::endl;
    
    if (memoryPressureCallback_) {
        memoryPressureCallback_();
    }
    
    // Force cleanup
    forceMemoryCleanup();
    
    if (isMemoryPressureHigh()) {
        sendAlert("CRITICAL: Memory pressure remains high after cleanup");
    }
}

bool Zero_Copy_IO_Manager::checkDiskSpace() {
    size_t availableSpace = getAvailableDiskSpace();
    
    if (availableSpace < MIN_DISK_SPACE) {
        sendAlert("CRITICAL: Disk space below minimum threshold");
        return false;
    }
    
    return true;
}

size_t Zero_Copy_IO_Manager::getAvailableDiskSpace() {
    try {
        fs::space_info space = fs::space(workingDirectory_);
        return space.available;
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to get disk space: " << e.what() << std::endl;
        return 0;
    }
}

bool Zero_Copy_IO_Manager::hasEnoughSpace(size_t requiredBytes) {
    size_t availableSpace = getAvailableDiskSpace();
    return availableSpace >= requiredBytes + MIN_DISK_SPACE;
}

void Zero_Copy_IO_Manager::cleanupUnusedMappings() {
    std::lock_guard<std::mutex> lock(regionsMutex_);
    
    // Simple cleanup strategy - remove oldest mappings if under pressure
    if (activeMappings_ > 10) {  // Keep max 10 mappings
        auto it = mappedRegions_.begin();
        if (it != mappedRegions_.end()) {
            unmapFile(it->first);
        }
    }
}

void Zero_Copy_IO_Manager::compactMemoryUsage() {
    // Use madvise to hint kernel about memory usage patterns
    std::lock_guard<std::mutex> lock(regionsMutex_);
    
    for (auto& pair : mappedRegions_) {
        // Hint that memory might not be needed soon
        adviseMemoryUsage(pair.first, MADV_DONTNEED);
    }
}

std::string Zero_Copy_IO_Manager::getWorkerChunkPath(const std::string& workerId, int chunkIndex) {
    return workingDirectory_ + "/chunks/worker_" + workerId + "_chunk_" + std::to_string(chunkIndex) + ".bin";
}

std::string Zero_Copy_IO_Manager::getChunkTempPath(const std::string& workerId, int chunkIndex) {
    return workingDirectory_ + "/temp/worker_" + workerId + "_chunk_" + std::to_string(chunkIndex) + ".tmp";
}

bool Zero_Copy_IO_Manager::ensureDirectoryExists(const std::string& filePath) {
    try {
        fs::path path(filePath);
        fs::path dir = path.parent_path();
        
        if (!dir.empty() && !fs::exists(dir)) {
            return fs::create_directories(dir);
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to create directory: " << e.what() << std::endl;
        return false;
    }
}

void Zero_Copy_IO_Manager::sendAlert(const std::string& message) {
    std::cout << "🚨 ZERO_COPY_IO ALERT: " << message << std::endl;
    
    if (alertCallback_) {
        alertCallback_(message);
    }
}

void Zero_Copy_IO_Manager::handleMappingError(const std::string& filePath, const std::string& error) {
    std::cerr << "❌ Mapping error for " << filePath << ": " << error << std::endl;
    sendAlert("File mapping failed: " + error);
}

void Zero_Copy_IO_Manager::handleWriteError(const BlockWriteRequest& request, const std::string& error) {
    std::cerr << "❌ Write error for " << request.targetPath << ": " << error << std::endl;
    sendAlert("Block write failed: " + error);
}

void Zero_Copy_IO_Manager::logMemoryOperation(const std::string& operation, size_t size) {
    std::cout << "🧠 Memory " << operation << ": " << (size / 1024 / 1024) << "MB" << std::endl;
}

void Zero_Copy_IO_Manager::logBlockWrite(const BlockWriteRequest& request, bool success) {
    std::string status = success ? "✅" : "❌";
    std::cout << status << " Block Write: " << request.targetPath 
              << " (" << (request.dataSize / 1024 / 1024) << "MB)";
    
    if (request.diskOffset > 0) {
        std::cout << " @ offset " << request.diskOffset;
    }
    
    std::cout << std::endl;
}

double Zero_Copy_IO_Manager::getMemoryEfficiency() const {
    if (totalBytesMapped_ == 0) return 0.0;
    
    // Efficiency = (bytes written / bytes mapped) * 100
    return static_cast<double>(totalBytesWritten_) / totalBytesMapped_ * 100.0;
}
