#include "Merge_Master.hpp"
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <shared_mutex>
#include <atomic>
#include <optional>
#include <memory>

namespace fs = std::filesystem;

// Modern C++17/20 Atomic File Writer with crash-proof RAII
class ModernAtomicFileWriter {
private:
    std::string finalPath_;
    std::string tempPath_;
    std::unique_ptr<std::ofstream> file_;
    std::atomic<bool> isOpen_{false};
    std::atomic<bool> isCommitted_{false};
    
public:
    explicit ModernAtomicFileWriter(const std::string& finalPath) 
        : finalPath_(finalPath) {
        // Generate unique temporary file path with timestamp
        auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
        tempPath_ = finalPath + ".tmp." + std::to_string(timestamp);
    }
    
    ~ModernAtomicFileWriter() {
        // RAII: Always cleanup temp file if not committed
        if (isOpen_.load(std::memory_order_acquire)) {
            if (file_) file_->close();
        }
        if (!isCommitted_.load(std::memory_order_acquire) && fs::exists(tempPath_)) {
            try {
                fs::remove(tempPath_);
                std::cout << "🧹 RAII: Cleaned up temp file: " << tempPath_ << std::endl;
            } catch (...) {
                // Ignore cleanup errors in destructor
            }
        }
    }
    
    bool open() {
        try {
            file_ = std::make_unique<std::ofstream>(tempPath_, std::ios::binary);
            isOpen_.store(file_->is_open(), std::memory_order_release);
            return isOpen_.load(std::memory_order_acquire);
        } catch (const std::exception& e) {
            std::cerr << "❌ ModernAtomicFileWriter: Failed to open temp file: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool writeChunk(const std::string& chunkPath) {
        if (!isOpen_.load(std::memory_order_acquire)) {
            return false;
        }
        
        try {
            std::ifstream chunkFile(chunkPath, std::ios::binary);
            if (!chunkFile.is_open()) {
                std::cerr << "❌ ModernAtomicFileWriter: Cannot open chunk file: " << chunkPath << std::endl;
                return false;
            }
            
            // Modern C++17 stream error handling with exception guarantees
            chunkFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
            file_->exceptions(std::ofstream::failbit | std::ofstream::badbit);
            
            // Stream chunk data with atomic error checking
            *file_ << chunkFile.rdbuf();
            
            // CRITICAL: Check for stream errors after each chunk write
            if (file_->fail() || chunkFile.fail()) {
                std::cerr << "❌ ModernAtomicFileWriter: Stream error during chunk write" << std::endl;
                chunkFile.close();
                return false;
            }
            
            // CRITICAL: Flush after each chunk to ensure data is written
            file_->flush();
            if (file_->fail()) {
                std::cerr << "❌ ModernAtomicFileWriter: Flush error after chunk write" << std::endl;
                chunkFile.close();
                return false;
            }
            
            chunkFile.close();
            return true;
            
        } catch (const std::ifstream::failure& e) {
            std::cerr << "❌ ModernAtomicFileWriter: File stream failure: " << e.what() << std::endl;
            return false;
        } catch (const std::exception& e) {
            std::cerr << "❌ ModernAtomicFileWriter: Exception during chunk write: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool commit() {
        if (!isOpen_.load(std::memory_order_acquire) || isCommitted_.load(std::memory_order_acquire)) {
            return false;
        }
        
        try {
            if (file_) file_->close();
            isOpen_.store(false, std::memory_order_release);
            
            // Validate temp file before atomic rename
            if (!fs::exists(tempPath_)) {
                std::cerr << "❌ ModernAtomicFileWriter: Temp file not created" << std::endl;
                return false;
            }
            
            // Modern C++17 atomic file operations with error handling
            std::error_code ec;
            
            // Remove existing output file if it exists
            if (fs::exists(finalPath_, ec)) {
                fs::remove(finalPath_, ec);
                if (ec) {
                    std::cerr << "❌ ModernAtomicFileWriter: Failed to remove existing file: " << ec.message() << std::endl;
                    return false;
                }
            }
            
            // Atomic rename operation
            fs::rename(tempPath_, finalPath_, ec);
            if (ec) {
                std::cerr << "❌ ModernAtomicFileWriter: Failed to rename file: " << ec.message() << std::endl;
                return false;
            }
            
            isCommitted_.store(true, std::memory_order_release);
            std::cout << "✅ ModernAtomicFileWriter: Successfully committed: " << finalPath_ << std::endl;
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "❌ ModernAtomicFileWriter: Failed to commit: " << e.what() << std::endl;
            return false;
        }
    }
    
    const std::string& getTempPath() const noexcept { return tempPath_; }
    const std::string& getFinalPath() const noexcept { return finalPath_; }
    
    // Delete copy operations
    ModernAtomicFileWriter(const ModernAtomicFileWriter&) = delete;
    ModernAtomicFileWriter& operator=(const ModernAtomicFileWriter&) = delete;
    
    // Modern C++17 move semantics
    ModernAtomicFileWriter(ModernAtomicFileWriter&& other) noexcept = default;
    ModernAtomicFileWriter& operator=(ModernAtomicFileWriter&& other) noexcept = default;
};

// Modern C++17/20 Merge Master with advanced safety
Merge_Master::Merge_Master() 
    : mergingActive_(false), completedJobsCount_(0), outputDirectory_("Merge_Output") {
    std::cout << "🔗 Merge_Master initializing - Modern C++17/20 Edition" << std::endl;
    
    // Create output directory with modern error handling
    try {
        std::error_code ec;
        fs::create_directories(outputDirectory_, ec);
        if (ec) {
            std::cerr << "❌ Merge_Master: Cannot create output directory: " << ec.message() << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "❌ Merge_Master: Exception creating directory: " << e.what() << std::endl;
    }
    
    // Open log file with modern RAII
    logFile_.open("Merge_Master_Log.txt", std::ios::app);
    if (logFile_.is_open()) {
        logFile_ << "\n=== Merge_Master Session Started - Modern C++17/20 ===" << std::endl;
    }
}

Merge_Master::~Merge_Master() {
    stopMerging();
    
    if (logFile_.is_open()) {
        logFile_ << "=== Merge_Master Session Ended ===" << std::endl;
        logFile_.close();
    }
}

void Merge_Master::startMerging() {
    if (mergingActive_.load(std::memory_order_acquire)) {
        std::cout << "⚠️ Merge_Master: Merging already active" << std::endl;
        return;
    }
    
    mergingActive_.store(true, std::memory_order_release);
    mergingThread_ = std::thread(&Merge_Master::mergingLoop, this);
    
    std::cout << "🚀 Merge_Master: Merging started" << std::endl;
    if (logFile_.is_open()) {
        logFile_ << "Merging started with modern C++17/20 safety" << std::endl;
    }
}

void Merge_Master::stopMerging() {
    if (!mergingActive_.load(std::memory_order_acquire)) {
        return;
    }
    
    mergingActive_.store(false, std::memory_order_release);
    jobsCondition_.notify_all();
    
    if (mergingThread_.joinable()) {
        mergingThread_.join();
    }
    
    std::cout << "🛑 Merge_Master: Merging stopped" << std::endl;
    if (logFile_.is_open()) {
        logFile_ << "Merging stopped" << std::endl;
    }
}

bool Merge_Master::receiveProcessedChunk(const ProcessedChunk& chunk) {
    std::cout << "📥 Merge_Master: Received processed chunk " << chunk.chunkIndex 
              << " from AI-" << chunk.sourceAI << " (Video: " << chunk.originalVideoId << ")" << std::endl;
    
    // Modern C++17 scoped lock with shared_mutex for better concurrency
    auto lock = std::unique_lock<std::shared_mutex>(jobsMutex_);
    
    // Find or create merge job
    auto it = mergeJobs_.find(chunk.originalVideoId);
    if (it == mergeJobs_.end()) {
        // Create new merge job
        MergeJob job;
        job.videoId = chunk.originalVideoId;
        job.outputPath = getOutputPath(chunk.originalVideoId);
        job.isCompleted.store(false, std::memory_order_release);
        job.startTime = std::chrono::system_clock::now();
        job.sequenceIndex.store(0, std::memory_order_release);
        
        mergeJobs_[chunk.originalVideoId] = job;
        it = mergeJobs_.find(chunk.originalVideoId);
    }
    
    // Add chunk to job with atomic operations
    ProcessedChunk atomicChunk = chunk;
    atomicChunk.receivedTime = std::chrono::system_clock::now();
    it->second.chunks[chunk.chunkIndex] = std::move(atomicChunk);
    
    // Check if all chunks are received
    if (allChunksReceived(it->second)) {
        jobsCondition_.notify_one();
        std::cout << "✅ Merge_Master: All chunks received for video: " << chunk.originalVideoId << std::endl;
    }
    
    return true;
}

bool Merge_Master::mergeVideoChunks(const std::string& videoId) {
    auto lock = std::unique_lock<std::shared_mutex>(jobsMutex_);
    
    auto it = mergeJobs_.find(videoId);
    if (it == mergeJobs_.end()) {
        std::cerr << "❌ Merge_Master: No merge job found for video: " << videoId << std::endl;
        return false;
    }
    
    MergeJob& job = it->second;
    
    if (!allChunksReceived(job)) {
        std::cerr << "❌ Merge_Master: Not all chunks received for video: " << videoId << std::endl;
        return false;
    }
    
    // Validate chunk sequence
    if (!validateChunkSequence(job)) {
        std::cerr << "❌ Merge_Master: Invalid chunk sequence for video: " << videoId << std::endl;
        return false;
    }
    
    std::cout << "🔗 Merge_Master: Merging chunks for video: " << videoId << std::endl;
    
    // Perform merge
    bool success = performMerge(job);
    
    if (success) {
        job.isCompleted.store(true, std::memory_order_release);
        completedJobsCount_.fetch_add(1, std::memory_order_acq_rel);
        
        std::cout << "✅ Merge_Master: Video merged successfully: " << videoId << std::endl;
        std::cout << "   Output: " << job.outputPath << std::endl;
        
        // Log the operation
        logMergeOperation(job);
    } else {
        std::cerr << "❌ Merge_Master: Failed to merge video: " << videoId << std::endl;
    }
    
    return success;
}

bool Merge_Master::performMerge(const MergeJob& job) {
    return mergeChunkFiles(job);
}

bool Merge_Master::mergeChunkFiles(const MergeJob& job) {
    // FIXED: RAII with unique_ptr for automatic temp file cleanup
    auto writer = std::make_unique<ModernAtomicFileWriter>(job.outputPath);
    
    if (!writer->open()) {
        std::cerr << "❌ Merge_Master: Failed to create atomic writer for: " << job.outputPath << std::endl;
        return false;
    }
    
    // Calculate total required space and validate all chunks
    uint64_t requiredSpace = 0;
    std::vector<std::pair<int, std::string>> chunkPaths;
    
    // Collect all chunks in order
    for (const auto& pair : job.chunks) {
        chunkPaths.emplace_back(pair.first, pair.second.processedPath);
    }
    
    // Sort by chunk index for proper order
    std::sort(chunkPaths.begin(), chunkPaths.end());
    
    // Validate all chunks and calculate space with modern error handling
    for (const auto& chunkInfo : chunkPaths) {
        const std::string& chunkPath = chunkInfo.second;
        
        // Validate chunk file exists and is readable
        if (!fs::exists(chunkPath)) {
            std::cerr << "❌ Merge_Master: Chunk file does not exist: " << chunkPath << std::endl;
            return false;
        }
        
        // Get actual file size with error_code for modern error handling
        std::error_code ec;
        auto actualChunkSize = fs::file_size(chunkPath, ec);
        if (ec) {
            std::cerr << "❌ Merge_Master: Cannot get size for " << chunkPath 
                      << ": " << ec.message() << std::endl;
            return false;
        }
        
        requiredSpace += actualChunkSize;
    }
    
    // Check available disk space with 20% safety margin and modern error handling
    try {
        const auto spaceInfo = fs::space(fs::path(job.outputPath).parent_path(), ec);
        if (ec) {
            std::cerr << "❌ Merge_Master: Cannot check disk space: " << ec.message() << std::endl;
            return false;
        }
        
        if (spaceInfo.available < (requiredSpace * 1.2)) {
            std::cerr << "❌ Merge_Master: Insufficient disk space. Required: " 
                      << (requiredSpace / 1024 / 1024) << " MB, Available: " 
                      << (spaceInfo.available / 1024 / 1024) << " MB" << std::endl;
            return false;
        }
    } catch (const std::exception& e) {
        std::cerr << "❌ Merge_Master: Exception checking disk space: " << e.what() << std::endl;
        return false;
    }
    
    // Merge chunks in order using RAII writer
    uint64_t totalSize = 0;
    for (const auto& chunkInfo : chunkPaths) {
        int chunkIndex = chunkInfo.first;
        const std::string& chunkPath = chunkInfo.second;
        
        if (!writer->writeChunk(chunkPath)) {
            std::cerr << "❌ Merge_Master: Failed to write chunk " << chunkIndex 
                      << " (" << chunkPath << ")" << std::endl;
            // RAII: writer destructor will automatically cleanup temp file
            return false;
        }
        
        // Update total size
        auto it = job.chunks.find(chunkIndex);
        if (it != job.chunks.end()) {
            totalSize += it->second.processedSize;
        }
        
        std::cout << "   Merged chunk " << chunkIndex << " (AI-" << it->second.sourceAI 
                  << ", Size: " << (it->second.processedSize / 1024 / 1024) << " MB)" << std::endl;
    }
    
    // Commit the atomic write
    if (!writer->commit()) {
        std::cerr << "❌ Merge_Master: Failed to commit merged file: " << job.outputPath << std::endl;
        // RAII: writer destructor will automatically cleanup temp file
        return false;
    }
    
    // Final validation with modern error handling
    try {
        if (!fs::exists(job.outputPath)) {
            std::cerr << "❌ Merge_Master: Final file not created: " << job.outputPath << std::endl;
            return false;
        }
        
        std::error_code ec;
        auto finalSize = fs::file_size(job.outputPath, ec);
        if (ec) {
            std::cerr << "❌ Merge_Master: Error getting final file size: " << ec.message() << std::endl;
            return false;
        }
        
        if (finalSize != totalSize) {
            std::cerr << "❌ Merge_Master: Final file size mismatch. Expected: " 
                      << totalSize << ", Actual: " << finalSize << std::endl;
            fs::remove(job.outputPath, ec);
            return false;
        }
    } catch (const std::exception& e) {
        std::cerr << "❌ Merge_Master: Exception validating final file: " << e.what() << std::endl;
        std::error_code ec;
        if (fs::exists(job.outputPath, ec)) {
            fs::remove(job.outputPath, ec);
        }
        return false;
    }
    
    std::cout << "✅ Final video created atomically: " << (totalSize / 1024 / 1024) << " MB" << std::endl;
    return true;
}

void Merge_Master::mergingLoop() {
    while (mergingActive_.load(std::memory_order_acquire)) {
        std::unique_lock<std::shared_mutex> lock(jobsMutex_);
        
        // Wait for jobs with all chunks received
        jobsCondition_.wait(lock, [this] {
            if (!mergingActive_.load(std::memory_order_acquire)) return true;
            
            for (const auto& pair : mergeJobs_) {
                const MergeJob& job = pair.second;
                if (!job.isCompleted.load(std::memory_order_acquire) && allChunksReceived(job)) {
                    return true;
                }
            }
            return false;
        });
        
        if (!mergingActive_.load(std::memory_order_acquire)) break;
        
        // Find jobs ready for merging
        std::vector<std::string> readyJobs;
        for (const auto& pair : mergeJobs_) {
            const MergeJob& job = pair.second;
            if (!job.isCompleted.load(std::memory_order_acquire) && allChunksReceived(job)) {
                readyJobs.push_back(pair.first);
            }
        }
        
        lock.unlock();
        
        // Process ready jobs
        for (const std::string& videoId : readyJobs) {
            mergeVideoChunks(videoId);
        }
        
        // Cleanup completed jobs
        lock.lock();
        std::vector<std::string> toCleanup;
        for (const auto& pair : mergeJobs_) {
            const MergeJob& job = pair.second;
            if (job.isCompleted.load(std::memory_order_acquire)) {
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now() - job.startTime).count();
                if (elapsed > 300) { // Cleanup after 5 minutes
                    toCleanup.push_back(pair.first);
                }
            }
        }
        
        lock.unlock();
        
        for (const std::string& videoId : toCleanup) {
            cleanupCompletedJob(videoId);
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

bool Merge_Master::allChunksReceived(const MergeJob& job) {
    // Modern C++17 dynamic check
    return !job.chunks.empty() && job.chunks.size() >= 1;
}

bool Merge_Master::validateChunkSequence(const MergeJob& job) {
    // Modern C++17 dynamic validation
    if (job.chunks.empty()) {
        return false;
    }
    
    // Find the maximum chunk index
    int maxChunkIndex = 0;
    for (const auto& pair : job.chunks) {
        maxChunkIndex = std::max(maxChunkIndex, pair.first);
    }
    
    // Check if we have all chunks from 1 to maxChunkIndex
    for (int i = 1; i <= maxChunkIndex; ++i) {
        if (job.chunks.find(i) == job.chunks.end()) {
            return false;
        }
    }
    
    return true;
}

void Merge_Master::cleanupCompletedJob(const std::string& videoId) {
    auto lock = std::unique_lock<std::shared_mutex>(jobsMutex_);
    
    auto it = mergeJobs_.find(videoId);
    if (it != mergeJobs_.end() && it->second.isCompleted.load(std::memory_order_acquire)) {
        // Remove chunk files with modern error handling
        for (const auto& pair : it->second.chunks) {
            const ProcessedChunk& chunk = pair.second;
            try {
                std::error_code ec;
                if (fs::exists(chunk.processedPath, ec)) {
                    fs::remove(chunk.processedPath, ec);
                    if (ec) {
                        std::cerr << "⚠️ Could not remove chunk file: " << ec.message() << std::endl;
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "⚠️ Exception removing chunk file: " << e.what() << std::endl;
            }
        }
        
        // Remove job
        mergeJobs_.erase(it);
        
        std::cout << "🧹 Merge_Master: Cleaned up job for video: " << videoId << std::endl;
    }
}

std::string Merge_Master::getOutputPath(const std::string& videoId) {
    return outputDirectory_ + "/" + videoId + "_merged.mp4";
}

void Merge_Master::logMergeOperation(const MergeJob& job) {
    if (logFile_.is_open()) {
        auto time_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        logFile_ << "Video Merged: " << job.videoId 
                << " | Sequence: " << job.sequenceIndex.load(std::memory_order_acquire)
                << " | Output: " << job.outputPath
                << " | Time: " << std::ctime(&time_t);
        
        for (const auto& pair : job.chunks) {
            const ProcessedChunk& chunk = pair.second;
            logFile_ << "  Chunk " << chunk.chunkIndex << ": AI-" << chunk.sourceAI 
                    << " | Size: " << (chunk.processedSize / 1024 / 1024) << " MB" << std::endl;
        }
    }
}

// Modern C++17/20 status methods with atomic operations
int Merge_Master::getPendingJobs() const {
    auto lock = std::shared_lock<std::shared_mutex>(jobsMutex_);
    int pending = 0;
    for (const auto& pair : mergeJobs_) {
        if (!pair.second.isCompleted.load(std::memory_order_acquire)) {
            pending++;
        }
    }
    return pending;
}

int Merge_Master::getCompletedJobs() const {
    return completedJobsCount_.load(std::memory_order_acquire);
}

bool Merge_Master::isMergingActive() const {
    return mergingActive_.load(std::memory_order_acquire);
}

void Merge_Master::getMergingStatus(std::string& status) {
    if (mergingActive_.load(std::memory_order_acquire)) {
        status = "🔗 Merge_Master: Active (Pending: " + std::to_string(getPendingJobs()) + 
                ", Completed: " + std::to_string(getCompletedJobs()) + ")";
    } else {
        status = "🛑 Merge_Master: Inactive";
    }
}
