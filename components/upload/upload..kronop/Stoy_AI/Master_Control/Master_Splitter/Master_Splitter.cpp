#include "Master_Splitter.hpp"
#include "../Input_Collector/Input_Collector.hpp"
#include <iostream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

Master_Splitter::Master_Splitter(Input_Collector& inputCollector)
    : inputCollector_(inputCollector), splittingActive_(false), chunksDirectory_("Master_Chunks") {
    std::cout << "🔪 Master_Splitter initialized" << std::endl;
    
    // Create chunks directory
    try {
        fs::create_directories(chunksDirectory_);
    } catch (const fs::filesystem_error& e) {
        std::cerr << "❌ Master_Splitter: Cannot create chunks directory: " << e.what() << std::endl;
    }
    
    // Open log file
    logFile_.open("Master_Splitter_Log.txt", std::ios::app);
    if (logFile_.is_open()) {
        logFile_ << "\n=== Master_Splitter Session Started ===" << std::endl;
    }
}

Master_Splitter::~Master_Splitter() {
    stopSplitting();
    
    if (logFile_.is_open()) {
        logFile_ << "=== Master_Splitter Session Ended ===" << std::endl;
        logFile_.close();
    }
}

void Master_Splitter::startSplitting() {
    if (splittingActive_) {
        std::cout << "⚠️ Master_Splitter: Splitting already active" << std::endl;
        return;
    }
    
    splittingActive_ = true;
    splittingThread_ = std::thread(&Master_Splitter::splittingLoop, this);
    
    std::cout << "🚀 Master_Splitter: Splitting started" << std::endl;
    if (logFile_.is_open()) {
        logFile_ << "Splitting started" << std::endl;
    }
}

void Master_Splitter::stopSplitting() {
    if (!splittingActive_) {
        return;
    }
    
    splittingActive_ = false;
    jobsCondition_.notify_all();
    
    if (splittingThread_.joinable()) {
        splittingThread_.join();
    }
    
    std::cout << "🛑 Master_Splitter: Splitting stopped" << std::endl;
    if (logFile_.is_open()) {
        logFile_ << "Splitting stopped" << std::endl;
    }
}

bool Master_Splitter::splitVideo(const VideoInput& video) {
    std::cout << "🔪 Master_Splitter: Splitting video - " << video.videoId 
              << " (Seq: " << video.sequenceIndex << ")" << std::endl;
    
    // Create splitting job
    SplittingJob job;
    job.videoInput = video;
    job.isCompleted = false;
    job.startTime = std::chrono::system_clock::now();
    
    // Create video chunks
    if (!createVideoChunks(video, job.chunks)) {
        std::cerr << "❌ Master_Splitter: Failed to create chunks for video: " << video.videoId << std::endl;
        return false;
    }
    
    // Add to active jobs
    {
        std::lock_guard<std::mutex> lock(jobsMutex_);
        activeJobs_.push_back(job);
    }
    
    // Distribute chunks to AI1-AI5
    if (!distributeChunksToAI(job.chunks)) {
        std::cerr << "❌ Master_Splitter: Failed to distribute chunks for video: " << video.videoId << std::endl;
        cleanupJob(job);
        return false;
    }
    
    // Log the operation
    logSplittingOperation(video, job.chunks);
    
    // Mark job as completed
    {
        std::lock_guard<std::mutex> lock(jobsMutex_);
        auto it = std::find_if(activeJobs_.begin(), activeJobs_.end(),
            [&video](const SplittingJob& j) { return j.videoInput.videoId == video.videoId; });
        if (it != activeJobs_.end()) {
            it->isCompleted = true;
        }
    }
    
    std::cout << "✅ Master_Splitter: Video split successfully - " << video.videoId 
              << " (5 chunks created and distributed)" << std::endl;
    
    return true;
}

bool Master_Splitter::distributeChunksToAI(const std::vector<VideoChunk>& chunks) {
    if (chunks.size() != 5) {
        std::cerr << "❌ Master_Splitter: Expected 5 chunks, got " << chunks.size() << std::endl;
        return false;
    }
    
    // Send chunks to AI1-AI5 in correct sequence
    for (int i = 0; i < 5; ++i) {
        const VideoChunk& chunk = chunks[i];
        int aiNumber = i + 1; // AI1 to AI5
        
        sendChunkToAI(chunk, aiNumber);
        
        std::cout << "📤 Master_Splitter: Chunk " << chunk.chunkIndex 
                  << " sent to AI-" << aiNumber << " (Video: " << chunk.originalVideoId << ")" << std::endl;
        
        // Small delay to ensure proper sequencing
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    return true;
}

int Master_Splitter::getActiveJobs() const {
    std::lock_guard<std::mutex> lock(jobsMutex_);
    return static_cast<int>(activeJobs_.size());
}

bool Master_Splitter::isSplittingActive() const {
    return splittingActive_;
}

void Master_Splitter::getSplittingStatus(std::string& status) {
    if (splittingActive_) {
        status = "🔪 Master_Splitter: Active (Jobs: " + std::to_string(getActiveJobs()) + ")";
    } else {
        status = "🛑 Master_Splitter: Inactive";
    }
}

void Master_Splitter::splittingLoop() {
    while (splittingActive_) {
        // Get next video from input collector
        VideoInput video;
        if (inputCollector_.getNextVideo(video)) {
            splitVideo(video);
        }
        
        // Cleanup completed jobs
        {
            std::lock_guard<std::mutex> lock(jobsMutex_);
            activeJobs_.erase(
                std::remove_if(activeJobs_.begin(), activeJobs_.end(),
                    [](const SplittingJob& job) { 
                        return job.isCompleted && 
                               std::chrono::duration_cast<std::chrono::seconds>(
                                   std::chrono::system_clock::now() - job.startTime).count() > 60;
                    }),
                activeJobs_.end()
            );
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

bool Master_Splitter::createVideoChunks(const VideoInput& video, std::vector<VideoChunk>& chunks) {
    chunks.clear();
    chunks.resize(5);
    
    uint64_t fileSize = video.fileSize;
    uint64_t chunkSize = fileSize / 5;
    
    for (int i = 0; i < 5; ++i) {
        VideoChunk chunk;
        chunk.chunkIndex = i + 1;
        chunk.originalVideoId = video.videoId;
        chunk.startPosition = i * chunkSize;
        chunk.endPosition = (i == 4) ? fileSize : (i + 1) * chunkSize;
        chunk.chunkSize = chunk.endPosition - chunk.startPosition;
        chunk.assignedAI = i + 1;
        chunk.isProcessed = false;
        
        // Save chunk to file
        if (!saveChunkToFile(video, chunk)) {
            return false;
        }
        
        chunks[i] = chunk;
    }
    
    return true;
}

bool Master_Splitter::saveChunkToFile(const VideoInput& video, VideoChunk& chunk) {
    chunk.chunkPath = getChunkPath(video, chunk.chunkIndex);
    
    // ⚠️ FIXED: RAII for automatic file handle cleanup
    std::ifstream inputFile;
    std::ofstream chunkFile;
    
    try {
        inputFile.open(video.videoPath, std::ios::binary);
        if (!inputFile.is_open()) {
            std::cerr << "❌ Master_Splitter: Cannot open input video: " << video.videoPath << std::endl;
            return false;
        }
        
        chunkFile.open(chunk.chunkPath, std::ios::binary);
        if (!chunkFile.is_open()) {
            std::cerr << "❌ Master_Splitter: Cannot create chunk file: " << chunk.chunkPath << std::endl;
            inputFile.close();
            return false;
        }
        
        // Seek to start position
        inputFile.seekg(chunk.startPosition);
        
        // Read and write chunk data
        std::vector<char> buffer(chunk.chunkSize);
        inputFile.read(buffer.data(), chunk.chunkSize);
        chunkFile.write(buffer.data(), chunk.chunkSize);
        
        // Files automatically closed by RAII destructors
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Master_Splitter: File operation failed: " << e.what() << std::endl;
        // Files automatically closed by RAII destructors
        return false;
    }
}

void Master_Splitter::sendChunkToAI(const VideoChunk& chunk, int aiNumber) {
    // This would interface with the AI system
    // For now, we simulate the AI assignment
    std::cout << "🤖 AI-" << aiNumber << ": Assigned chunk " << chunk.chunkIndex 
              << " of video " << chunk.originalVideoId << std::endl;
    
    if (logFile_.is_open()) {
        logFile_ << "Chunk " << chunk.chunkIndex << " -> AI-" << aiNumber 
                << " (Video: " << chunk.originalVideoId << ")" << std::endl;
    }
}

void Master_Splitter::logSplittingOperation(const VideoInput& video, const std::vector<VideoChunk>& chunks) {
    if (logFile_.is_open()) {
        auto time_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        logFile_ << "Video Split: " << video.videoId 
                << " | Sequence: " << video.sequenceIndex
                << " | Size: " << (video.fileSize / 1024 / 1024) << " MB"
                << " | Chunks: " << chunks.size()
                << " | Time: " << std::ctime(&time_t);
        
        for (const auto& chunk : chunks) {
            logFile_ << "  Chunk " << chunk.chunkIndex << ": " 
                    << (chunk.chunkSize / 1024 / 1024) << " MB -> AI-" << chunk.assignedAI << std::endl;
        }
    }
}

std::string Master_Splitter::getChunkPath(const VideoInput& video, int chunkIndex) {
    return chunksDirectory_ + "/" + video.videoId + "_chunk_" + std::to_string(chunkIndex) + ".mp4";
}

void Master_Splitter::cleanupJob(const SplittingJob& job) {
    // Remove chunk files if job failed
    for (const auto& chunk : job.chunks) {
        try {
            if (fs::exists(chunk.chunkPath)) {
                fs::remove(chunk.chunkPath);
            }
        } catch (const fs::filesystem_error& e) {
            std::cerr << "⚠️ Could not remove chunk file: " << chunk.chunkPath << std::endl;
        }
    }
}
