#ifndef SHARED_MEMORY_MANAGER_HPP
#define SHARED_MEMORY_MANAGER_HPP

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <map>
#include <queue>
#include <atomic>
#include <thread>
#include <fstream>
#include <filesystem>

/**
 * IO Vault - Data Treasury Department
 * यह फोल्डर 'Shared' होगा। सारे AI इसी तिजोरी से वीडियो उठाएंगे और इसी में अपना साफ़ किया हुआ डेटा डालेंगे
 */

namespace fs = std::filesystem;

enum DataType {
    VIDEO_CHUNK = 0,
    PROCESSED_TILE = 1,
    AUDIO_SEGMENT = 2,
    MERGED_FRAME = 3,
    FINAL_OUTPUT = 4
};

struct DataPacket {
    std::string packetId;
    DataType type;
    std::string sourcePath;
    std::string targetPath;
    std::vector<uint8_t> data;
    size_t dataSize;
    std::chrono::system_clock::time_point timestamp;
    int priority;  // 1=highest, 5=lowest
    bool isProcessed;
    std::string processingWorker;
};

struct MemorySlot {
    std::string slotId;
    DataType type;
    size_t maxSize;
    size_t currentSize;
    bool isOccupied;
    std::string currentOwner;
    std::chrono::system_clock::time_point lastAccess;
};

class SharedMemoryManager {
private:
    // Memory management with shared_mutex for better concurrency
    std::map<std::string, MemorySlot> memorySlots_;
    std::shared_mutex memoryMutex_;
    std::condition_variable memoryCondition_;
    
    // Data queues for different types with shared_mutex
    std::queue<DataPacket> videoInputQueue_;
    std::queue<DataPacket> processedTileQueue_;
    std::queue<DataPacket> audioQueue_;
    std::queue<DataPacket> outputQueue_;
    
    std::shared_mutex videoMutex_, tileMutex_, audioMutex_, outputMutex_;
    std::condition_variable videoCondition_, tileCondition_, audioCondition_, outputCondition_, queueFullCondition_;
    
    // File system paths
    std::string inputDirectory_;
    std::string outputDirectory_;
    std::string tempDirectory_;
    
    // Memory limits
    const size_t MAX_MEMORY_USAGE = 4ULL * 1024 * 1024 * 1024; // 4GB
    const size_t SLOT_SIZE = 64 * 1024 * 1024; // 64MB per slot
    const int MAX_SLOTS = 64;
    
    // Statistics
    std::atomic<size_t> totalMemoryUsed_;
    std::atomic<int> totalPacketsProcessed_;
    std::atomic<int> totalPacketsQueued_;
    std::atomic<int> memoryOverflowCount_;
    
    // Cleanup thread
    std::thread cleanupThread_;
    std::atomic<bool> cleanupActive_;
    
public:
    SharedMemoryManager();
    ~SharedMemoryManager();
    
    // Initialization
    bool initialize(const std::string& inputDir, const std::string& outputDir);
    void shutdown();
    
    // Data input/output
    bool storeDataPacket(const DataPacket& packet);
    bool retrieveDataPacket(const std::string& packetId, DataPacket& packet);
    
    // Queue management
    bool enqueueVideoInput(const DataPacket& packet);
    bool dequeueVideoInput(DataPacket& packet);
    
    bool enqueueProcessedTile(const DataPacket& packet);
    bool dequeueProcessedTile(DataPacket& packet);
    
    bool enqueueAudioSegment(const DataPacket& packet);
    bool dequeueAudioSegment(DataPacket& packet);
    
    bool enqueueOutput(const DataPacket& packet);
    bool dequeueOutput(DataPacket& packet);
    
    // Memory slot management
    std::string allocateMemorySlot(DataType type, size_t size, const std::string& owner);
    bool releaseMemorySlot(const std::string& slotId);
    bool accessMemorySlot(const std::string& slotId, std::vector<uint8_t>& data);
    
    // File system operations
    bool storeToFile(const DataPacket& packet, const std::string& filePath);
    bool loadFromFile(const std::string& filePath, DataPacket& packet);
    
    // Status and monitoring
    size_t getTotalMemoryUsed() const { return totalMemoryUsed_; }
    size_t getAvailableMemory() const { return MAX_MEMORY_USAGE - totalMemoryUsed_; }
    int getQueueSize(DataType type) const;
    int getActiveSlotCount() const;
    
    // Statistics
    int getTotalPacketsProcessed() const { return totalPacketsProcessed_; }
    int getTotalPacketsQueued() const { return totalPacketsQueued_; }
    int getMemoryOverflowCount() const { return memoryOverflowCount_; }
    double getMemoryUtilization() const;
    
    // Health monitoring
    bool isMemoryHealthy() const;
    void triggerMemoryCleanup();
    void optimizeMemoryUsage();

private:
    // Internal methods
    std::string generatePacketId(DataType type);
    std::string generateSlotId();
    std::string getQueueName(DataType type) const;
    
    // Memory management
    bool checkMemoryAvailability(size_t requiredSize);
    void performMemoryCleanup();
    void cleanupExpiredSlots();
    void optimizeSlotAllocation();
    
    // File operations
    bool createDirectories();
    bool validateFilePath(const std::string& path);
    void cleanupTempFiles();
    
    // Cleanup thread
    void cleanupLoop();
    
    // Logging
    void logDataOperation(const std::string& operation, const DataPacket& packet);
    void logMemoryOperation(const std::string& operation, const std::string& slotId, size_t size);
};

#endif // SHARED_MEMORY_MANAGER_HPP
