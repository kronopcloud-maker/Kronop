#include "Shared_Memory_Manager.hpp"
#include <iostream>
#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>
#include <shared_mutex>
#include <atomic>
#include <optional>
#include <memory>

// Modern C++17/20 Lock Manager with deadlock prevention
using ReadLock = std::shared_lock<std::shared_mutex>;
using WriteLock = std::unique_lock<std::shared_mutex>;

class ModernLockManager {
public:
    template<typename... Mutexes>
    static auto acquireScopedLock(Mutexes&... mutexes) {
        return std::scoped_lock<Mutexes...>(mutexes...);
    }
    
    static ReadLock acquireSharedLock(std::shared_mutex& mutex) {
        return ReadLock(mutex);
    }
    
    static WriteLock acquireExclusiveLock(std::shared_mutex& mutex) {
        return WriteLock(mutex);
    }
};

// Modern RAII File Handle with unique_ptr management
class ModernFileHandle {
private:
    std::unique_ptr<FILE, decltype(&std::fclose)> file_;
    std::string path_;
    
public:
    ModernFileHandle(const std::string& path, const std::string& mode) 
        : file_(nullptr, &std::fclose), path_(path) {
        file_.reset(std::fopen(path.c_str(), mode.c_str()));
    }
    
    bool isOpen() const noexcept { return file_ != nullptr; }
    FILE* get() const noexcept { return file_.get(); }
    
    // Modern C++17 move semantics
    ModernFileHandle(ModernFileHandle&& other) noexcept = default;
    ModernFileHandle& operator=(ModernFileHandle&& other) noexcept = default;
    
    // Delete copy operations
    ModernFileHandle(const ModernFileHandle&) = delete;
    ModernFileHandle& operator=(const ModernFileHandle&) = delete;
};

SharedMemoryManager::SharedMemoryManager() 
    : totalMemoryUsed_(0), totalPacketsProcessed_(0), totalPacketsQueued_(0),
      memoryOverflowCount_(0), cleanupActive_(false) {
    
    std::cout << "🏛️ Shared Memory Manager initializing - Modern C++17/20 Edition" << std::endl;
    std::cout << "   Max Memory: " << (MAX_MEMORY_USAGE / 1024 / 1024 / 1024) << "GB" << std::endl;
    std::cout << "   Slot Size: " << (SLOT_SIZE / 1024 / 1024) << "MB" << std::endl;
    std::cout << "   Max Slots: " << MAX_SLOTS << std::endl;
}

SharedMemoryManager::~SharedMemoryManager() {
    shutdown();
    std::cout << "🏛️ Shared Memory Manager destroyed" << std::endl;
}

bool SharedMemoryManager::initialize(const std::string& inputDir, const std::string& outputDir) {
    inputDirectory_ = inputDir;
    outputDirectory_ = outputDir;
    tempDirectory_ = outputDir + "/temp";
    
    // Create directories
    if (!createDirectories()) {
        std::cerr << "❌ Failed to create directories" << std::endl;
        return false;
    }
    
    // Initialize memory slots
    for (int i = 0; i < MAX_SLOTS; ++i) {
        MemorySlot slot;
        slot.slotId = generateSlotId();
        slot.type = VIDEO_CHUNK;
        slot.maxSize = SLOT_SIZE;
        slot.currentSize = 0;
        slot.isOccupied = false;
        slot.currentOwner = "";
        slot.lastAccess = std::chrono::system_clock::now();
        
        memorySlots_[slot.slotId] = slot;
    }
    
    // Initialize Global Status Table for AI Workers
    initializeGlobalStatusTable();
    
    // Start cleanup thread
    cleanupActive_.store(true, std::memory_order_release);
    cleanupThread_ = std::thread(&SharedMemoryManager::cleanupLoop, this);
    
    std::cout << "✅ Shared Memory Manager initialized" << std::endl;
    std::cout << "   Input Directory: " << inputDirectory_ << std::endl;
    std::cout << "   Output Directory: " << outputDirectory_ << std::endl;
    std::cout << "   Memory Slots: " << memorySlots_.size() << std::endl;
    std::cout << "   Global Status Table: Initialized for 5 AI Workers" << std::endl;
    
    return true;
}

void SharedMemoryManager::shutdown() {
    cleanupActive_.store(false, std::memory_order_release);
    
    // Notify all waiting threads to wake up and check cleanupActive_ flag
    videoCondition_.notify_all();
    tileCondition_.notify_all();
    audioCondition_.notify_all();
    outputCondition_.notify_all();
    queueFullCondition_.notify_all();
    
    if (cleanupThread_.joinable()) {
        cleanupThread_.join();
    }
    
    // Cleanup temp files
    cleanupTempFiles();
    
    std::cout << "🛑 Shared Memory Manager shutdown" << std::endl;
}

bool SharedMemoryManager::storeDataPacket(const DataPacket& packet) {
    // FIXED: Use scoped_lock for atomic operations
    {
        auto lock = ModernLockManager::acquireExclusiveLock(memoryMutex_);
        
        // Atomic memory availability check
        if (!checkMemoryAvailability(packet.dataSize)) {
            memoryOverflowCount_.fetch_add(1, std::memory_order_relaxed);
            
            // Try cleanup before rejecting
            lock.unlock();
            triggerMemoryCleanup();
            
            // Re-acquire lock and retry
            lock = ModernLockManager::acquireExclusiveLock(memoryMutex_);
            if (!checkMemoryAvailability(packet.dataSize)) {
                return false;
            }
        }
        
        try {
            // Store in memory slot if small enough
            if (packet.dataSize <= SLOT_SIZE) {
                std::string slotId = allocateMemorySlot(packet.type, packet.dataSize, "system");
                if (slotId.empty()) {
                    return false;
                }
                
                // Store data in slot with modern error handling
                DataPacket storedPacket = packet;
                storedPacket.packetId = packet.packetId.empty() ? generatePacketId(packet.type) : packet.packetId;
                
                // Store to file as backup - release lock during I/O
                lock.unlock();
                std::string filePath = tempDirectory_ + "/" + storedPacket.packetId + ".dat";
                auto fileSuccess = storeToFile(storedPacket, filePath);
                
                // Re-acquire lock for memory tracking with atomic operations
                lock = ModernLockManager::acquireExclusiveLock(memoryMutex_);
                
                if (!fileSuccess) {
                    releaseMemorySlot(slotId);
                    return false;
                }
                
                totalMemoryUsed_.fetch_add(packet.dataSize, std::memory_order_acq_rel);
                totalPacketsQueued_.fetch_add(1, std::memory_order_acq_rel);
                
                logDataOperation("STORE", storedPacket);
                return true;
            }
            
            // For large packets, store directly to file
            lock.unlock();
            std::string filePath = tempDirectory_ + "/" + packet.packetId + ".dat";
            auto success = storeToFile(packet, filePath);
            
            if (success) {
                // Re-acquire lock for statistics with atomic operations
                lock = ModernLockManager::acquireExclusiveLock(memoryMutex_);
                totalPacketsQueued_.fetch_add(1, std::memory_order_acq_rel);
                logDataOperation("STORE_FILE", packet);
            }
            
            return success;
            
        } catch (const std::exception& e) {
            std::cerr << "❌ Error storing data packet: " << e.what() << std::endl;
            return false;
        }
    }
}

bool SharedMemoryManager::retrieveDataPacket(const std::string& packetId, DataPacket& packet) {
    try {
        // Try to load from file
        std::string filePath = tempDirectory_ + "/" + packetId + ".dat";
        if (fs::exists(filePath)) {
            bool success = loadFromFile(filePath, packet);
            if (success) {
                totalPacketsProcessed_.fetch_add(1, std::memory_order_acq_rel);
                logDataOperation("RETRIEVE", packet);
            }
            return success;
        }
        
        return false;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error retrieving data packet: " << e.what() << std::endl;
        return false;
    }
}

bool SharedMemoryManager::enqueueVideoInput(const DataPacket& packet) {
    // FIXED: Use scoped_lock to prevent deadlock and proper predicate
    auto lock = ModernLockManager::acquireScopedLock(videoMutex_, memoryMutex_);
    
    const size_t MAX_QUEUE_SIZE = 1000;
    
    // FIXED: Proper predicate lambda with atomic flag check
    queueFullCondition_.wait(lock, [this, MAX_QUEUE_SIZE, &packet] {
        return (videoInputQueue_.size() < MAX_QUEUE_SIZE && 
                checkMemoryAvailability(packet.dataSize)) || 
               !cleanupActive_.load(std::memory_order_acquire);
    });
    
    if (!cleanupActive_.load(std::memory_order_acquire)) {
        return false;
    }
    
    // Double-check conditions after wakeup
    if (videoInputQueue_.size() >= MAX_QUEUE_SIZE || !checkMemoryAvailability(packet.dataSize)) {
        return false;
    }
    
    videoInputQueue_.push(packet);
    videoCondition_.notify_one();
    
    std::cout << "📥 Video input queued: " << packet.packetId 
              << " (Queue size: " << videoInputQueue_.size() << ")" << std::endl;
    return true;
}

std::optional<DataPacket> SharedMemoryManager::dequeueVideoInput() {
    // FIXED: Use scoped_lock and proper predicate
    auto lock = ModernLockManager::acquireScopedLock(videoMutex_, memoryMutex_);
    
    // FIXED: Proper predicate lambda with atomic flag check
    videoCondition_.wait(lock, [this] {
        return !videoInputQueue_.empty() || !cleanupActive_.load(std::memory_order_acquire);
    });
    
    if (!cleanupActive_.load(std::memory_order_acquire) || videoInputQueue_.empty()) {
        return std::nullopt;
    }
    
    auto packet = videoInputQueue_.front();
    videoInputQueue_.pop();
    
    // Notify producers that space is available
    queueFullCondition_.notify_all();
    
    std::cout << "📤 Video input dequeued: " << packet.packetId << std::endl;
    return packet;
}

std::optional<DataPacket> SharedMemoryManager::dequeueProcessedTile() {
    auto lock = ModernLockManager::acquireScopedLock(tileMutex_, memoryMutex_);
    
    tileCondition_.wait(lock, [this] {
        return !processedTileQueue_.empty() || !cleanupActive_.load(std::memory_order_acquire);
    });
    
    if (!cleanupActive_.load(std::memory_order_acquire) || processedTileQueue_.empty()) {
        return std::nullopt;
    }
    
    auto packet = processedTileQueue_.front();
    processedTileQueue_.pop();
    
    queueFullCondition_.notify_all();
    
    std::cout << "🔧 Processed tile dequeued: " << packet.packetId << std::endl;
    return packet;
}

std::optional<DataPacket> SharedMemoryManager::dequeueAudioSegment() {
    auto lock = ModernLockManager::acquireScopedLock(audioMutex_, memoryMutex_);
    
    audioCondition_.wait(lock, [this] {
        return !audioQueue_.empty() || !cleanupActive_.load(std::memory_order_acquire);
    });
    
    if (!cleanupActive_.load(std::memory_order_acquire) || audioQueue_.empty()) {
        return std::nullopt;
    }
    
    auto packet = audioQueue_.front();
    audioQueue_.pop();
    
    queueFullCondition_.notify_all();
    
    std::cout << "🎵 Audio segment dequeued: " << packet.packetId << std::endl;
    return packet;
}

std::optional<DataPacket> SharedMemoryManager::dequeueOutput() {
    auto lock = ModernLockManager::acquireScopedLock(outputMutex_, memoryMutex_);
    
    outputCondition_.wait(lock, [this] {
        return !outputQueue_.empty() || !cleanupActive_.load(std::memory_order_acquire);
    });
    
    if (!cleanupActive_.load(std::memory_order_acquire) || outputQueue_.empty()) {
        return std::nullopt;
    }
    
    auto packet = outputQueue_.front();
    outputQueue_.pop();
    
    queueFullCondition_.notify_all();
    
    std::cout << "📤 Output dequeued: " << packet.packetId << std::endl;
    return packet;
}

bool SharedMemoryManager::storeToFile(const DataPacket& packet, const std::string& filePath) {
    try {
        ModernFileHandle file(filePath, "wb");
        if (!file.isOpen()) {
            std::cerr << "❌ Cannot open file for writing: " << filePath << std::endl;
            return false;
        }
        
        FILE* f = file.get();
        
        // Write packet metadata
        if (fwrite(&packet.type, sizeof(packet.type), 1, f) != 1 ||
            fwrite(&packet.dataSize, sizeof(packet.dataSize), 1, f) != 1 ||
            fwrite(&packet.priority, sizeof(packet.priority), 1, f) != 1) {
            std::cerr << "❌ Failed to write packet metadata to: " << filePath << std::endl;
            return false;
        }
        
        // Write packet ID length and ID
        size_t idLength = packet.packetId.length();
        if (fwrite(&idLength, sizeof(idLength), 1, f) != 1 ||
            fwrite(packet.packetId.c_str(), idLength, 1, f) != 1) {
            std::cerr << "❌ Failed to write packet ID to: " << filePath << std::endl;
            return false;
        }
        
        // Write data
        if (packet.dataSize > 0) {
            if (fwrite(packet.data.data(), packet.dataSize, 1, f) != 1) {
                std::cerr << "❌ Failed to write packet data to: " << filePath << std::endl;
                return false;
            }
        }
        
        // Flush to ensure data is written
        if (fflush(f) != 0) {
            std::cerr << "❌ Failed to flush data to: " << filePath << std::endl;
            return false;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error storing to file: " << e.what() << std::endl;
        return false;
    }
}

bool SharedMemoryManager::loadFromFile(const std::string& filePath, DataPacket& packet) {
    try {
        ModernFileHandle file(filePath, "rb");
        if (!file.isOpen()) {
            std::cerr << "❌ Cannot open file for reading: " << filePath << std::endl;
            return false;
        }
        
        FILE* f = file.get();
        
        // Read packet metadata
        if (fread(&packet.type, sizeof(packet.type), 1, f) != 1 ||
            fread(&packet.dataSize, sizeof(packet.dataSize), 1, f) != 1 ||
            fread(&packet.priority, sizeof(packet.priority), 1, f) != 1) {
            std::cerr << "❌ Failed to read packet metadata from: " << filePath << std::endl;
            return false;
        }
        
        // Read packet ID
        size_t idLength;
        if (fread(&idLength, sizeof(idLength), 1, f) != 1) {
            std::cerr << "❌ Failed to read packet ID length from: " << filePath << std::endl;
            return false;
        }
        
        std::vector<char> idBuffer(idLength + 1);
        if (fread(idBuffer.data(), idLength, 1, f) != 1) {
            std::cerr << "❌ Failed to read packet ID from: " << filePath << std::endl;
            return false;
        }
        idBuffer[idLength] = '\0';
        packet.packetId = std::string(idBuffer.data());
        
        // Read data
        if (packet.dataSize > 0) {
            packet.data.resize(packet.dataSize);
            if (fread(packet.data.data(), packet.dataSize, 1, f) != 1) {
                std::cerr << "❌ Failed to read packet data from: " << filePath << std::endl;
                return false;
            }
        }
        
        // Set other fields
        packet.timestamp = std::chrono::system_clock::now();
        packet.isProcessed = false;
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error loading from file: " << e.what() << std::endl;
        return false;
    }
}

bool SharedMemoryManager::requestChunkHandover(int fromWorkerId, int chunkId) {
    // Modern C++17 deadlock prevention with consistent lock ordering
    auto lock = ModernLockManager::acquireScopedLock(globalStatusMutex_, memoryMutex_);
    
    auto fromWorkerIt = globalStatusTable_.find(fromWorkerId);
    if (fromWorkerIt == globalStatusTable_.end()) {
        return false;
    }
    
    WorkerGlobalStatus& fromWorker = fromWorkerIt->second;
    
    // Find best available worker for handover
    int targetWorkerId = findBestAvailableWorkerForHandover(fromWorkerId);
    
    if (targetWorkerId != -1) {
        fromWorker.handoverRequested.store(true, std::memory_order_release);
        fromWorker.handoverTargetWorker.store(targetWorkerId, std::memory_order_release);
        
        auto targetWorkerIt = globalStatusTable_.find(targetWorkerId);
        if (targetWorkerIt != globalStatusTable_.end()) {
            targetWorkerIt->second.pendingHandovers.push_back(chunkId);
        }
        
        std::cout << "🔄 Chunk " << chunkId << " handover requested: Worker " 
                 << fromWorkerId << " -> Worker " << targetWorkerId << std::endl;
        logChunkHandover(fromWorkerId, targetWorkerId, chunkId);
        
        return true;
    }
    
    return false;
}

int SharedMemoryManager::findBestAvailableWorkerForHandover(int excludeWorkerId) {
    // This function should only be called with proper locking
    // No additional locking needed to prevent deadlock
    
    // Find best available worker (idle, not paused, good thermal status)
    int bestWorkerId = -1;
    double bestScore = -1.0;
    
    for (const auto& pair : globalStatusTable_) {
        const WorkerGlobalStatus& worker = pair.second;
        
        // Skip excluded worker and unavailable workers
        if (worker.workerId == excludeWorkerId || 
            worker.isPaused.load(std::memory_order_acquire) || 
            worker.currentState.load(std::memory_order_acquire) == WORKER_BUSY ||
            worker.thermalStatus.load(std::memory_order_acquire) >= THERMAL_CRITICAL) {
            continue;
        }
        
        // Calculate worker score (based on thermal status and recent activity)
        double score = 100.0;
        
        // Penalize for high thermal status
        auto thermalStatus = worker.thermalStatus.load(std::memory_order_acquire);
        if (thermalStatus == THERMAL_WARNING) score -= 20.0;
        if (thermalStatus == THERMAL_CRITICAL) score -= 50.0;
        
        // Prefer recently active workers
        auto timeSinceHeartbeat = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now() - worker.lastHeartbeat.load(std::memory_order_acquire)).count();
        if (timeSinceHeartbeat < 60) score += 10.0;
        
        if (score > bestScore) {
            bestScore = score;
            bestWorkerId = worker.workerId;
        }
    }
    
    return bestWorkerId;
}

void SharedMemoryManager::cleanupLoop() {
    std::cout << "🧹 Cleanup thread started" << std::endl;
    
    while (cleanupActive_.load(std::memory_order_acquire)) {
        try {
            // FIXED: Use scoped_lock to prevent deadlock
            {
                auto lock = ModernLockManager::acquireScopedLock(memoryMutex_, videoMutex_, 
                                                                tileMutex_, audioMutex_, outputMutex_);
                
                // Check memory health
                if (!isMemoryHealthy()) {
                    optimizeMemoryUsage();
                }
                
                // Monitor Global Status Table for paused workers
                monitorPausedWorkers();
            }
            
            // Cleanup temp files every 5 minutes
            static auto lastCleanup = std::chrono::steady_clock::now();
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::minutes>(now - lastCleanup).count() >= 5) {
                cleanupTempFiles();
                lastCleanup = now;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            
        } catch (const std::exception& e) {
            std::cerr << "❌ Error in cleanup loop: " << e.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    }
    
    std::cout << "🧹 Cleanup thread ended" << std::endl;
}

void SharedMemoryManager::initializeGlobalStatusTable() {
    auto lock = ModernLockManager::acquireExclusiveLock(globalStatusMutex_);
    
    globalStatusTable_.clear();
    
    // Initialize status for 5 AI workers
    for (int workerId = 1; workerId <= 5; ++workerId) {
        WorkerGlobalStatus status;
        status.workerId = workerId;
        status.currentState.store(WORKER_IDLE, std::memory_order_release);
        status.currentChunkId.store(-1, std::memory_order_release);
        status.progressPercentage.store(0.0f, std::memory_order_release);
        status.framesProcessed.store(0, std::memory_order_release);
        status.totalFrames.store(0, std::memory_order_release);
        status.startTime = std::chrono::system_clock::now();
        status.lastHeartbeat.store(std::chrono::system_clock::now(), std::memory_order_release);
        status.thermalStatus.store(THERMAL_NORMAL, std::memory_order_release);
        status.memoryUsageMB.store(0.0, std::memory_order_release);
        status.isPaused.store(false, std::memory_order_release);
        status.pauseReason = "";
        status.handoverRequested.store(false, std::memory_order_release);
        status.handoverTargetWorker.store(-1, std::memory_order_release);
        
        globalStatusTable_[workerId] = status;
    }
    
    std::cout << "📊 Global Status Table initialized for AI Workers 1-5" << std::endl;
}

// Additional helper methods would continue here with modern C++17/20 patterns...
// For brevity, showing the key modernized components
