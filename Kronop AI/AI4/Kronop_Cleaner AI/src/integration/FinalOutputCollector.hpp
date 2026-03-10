/**
 * FinalOutputCollector.hpp
 * One-Way Output Collector for Processed Video Data
 * Receives processed data from AI Engine without back-delivery
 */

#ifndef FINAL_OUTPUT_COLLECTOR_HPP
#define FINAL_OUTPUT_COLLECTOR_HPP

#include <vector>
#include <queue>
#include <mutex>
#include <atomic>
#include <string>
#include <memory>

namespace kronop {

struct ProcessedFrame {
    int frameId;
    int width;
    int height;
    int channels;
    std::vector<uint8_t> data;
    double processingTime;
    std::string mode; // "GPU", "CPU", "NPU", etc.

    ProcessedFrame(int id = 0)
        : frameId(id), width(0), height(0), channels(3), processingTime(0.0) {}
};

struct ProcessedBatch {
    int batchId;
    int frameCount;
    std::vector<ProcessedFrame> frames;
    double totalProcessingTime;
    double avgFps;
    std::string mode;

    ProcessedBatch(int id = 0)
        : batchId(id), frameCount(0), totalProcessingTime(0.0), avgFps(0.0) {}
};

/**
 * Final Output Collector
 * Receives processed video data in one-way pipeline
 * Stores data for retrieval by external systems
 */
class FinalOutputCollector {
public:
    static FinalOutputCollector& getInstance();

    // Disable copy and assignment
    FinalOutputCollector(const FinalOutputCollector&) = delete;
    FinalOutputCollector& operator=(const FinalOutputCollector&) = delete;

    // Receive processed data (one-way from AI Engine)
    void receiveProcessedFrame(const ProcessedFrame& frame);
    void receiveProcessedBatch(const ProcessedBatch& batch);

    // Retrieval methods for external access
    bool getProcessedFrame(int frameId, ProcessedFrame& frame);
    bool getProcessedBatch(int batchId, ProcessedBatch& batch);
    bool getNextProcessedFrame(ProcessedFrame& frame);
    bool getNextProcessedBatch(ProcessedBatch& batch);

    // Status and statistics
    size_t getQueuedFrameCount() const;
    size_t getQueuedBatchCount() const;
    void clearAllData();

    // Configuration
    void setMaxQueueSize(size_t maxFrames, size_t maxBatches);
    void setOutputDirectory(const std::string& dir);

private:
    FinalOutputCollector();
    ~FinalOutputCollector();

    // Queues for processed data
    std::queue<ProcessedFrame> frameQueue_;
    std::queue<ProcessedBatch> batchQueue_;

    // Mutexes for thread safety
    mutable std::mutex frameMutex_;
    mutable std::mutex batchMutex_;

    // Configuration
    size_t maxFrameQueueSize_;
    size_t maxBatchQueueSize_;
    std::string outputDirectory_;

    // Statistics
    std::atomic<size_t> totalFramesReceived_;
    std::atomic<size_t> totalBatchesReceived_;
};

} // namespace kronop

#endif // FINAL_OUTPUT_COLLECTOR_HPP
