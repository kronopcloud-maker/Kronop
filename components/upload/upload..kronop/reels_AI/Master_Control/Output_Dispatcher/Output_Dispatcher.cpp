#include "Output_Dispatcher.hpp"
#include <iostream>
#include <filesystem>
#include <chrono>
#include <curl/curl.h>
#include <sstream>
#include <shared_mutex>
#include <atomic>
#include <optional>
#include <memory>

namespace fs = std::filesystem;

// Modern C++17/20 RAII CURL Handle with crash-proof wrapper
class ModernCurlRAII {
private:
    std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl_;
    std::unique_ptr<curl_mime, decltype(&curl_mime_free)> mime_;
    bool isInitialized_;
    
public:
    ModernCurlRAII() : curl_(nullptr, &curl_easy_cleanup), mime_(nullptr, &curl_mime_free), isInitialized_(false) {
        try {
            curl_.reset(curl_easy_init());
            isInitialized_ = (curl_ != nullptr);
            
            if (isInitialized_) {
                // Set default options for safety
                curl_easy_setopt(curl_.get(), CURLOPT_TIMEOUT, 300L);
                curl_easy_setopt(curl_.get(), CURLOPT_CONNECTTIMEOUT, 30L);
                curl_easy_setopt(curl_.get(), CURLOPT_FOLLOWLOCATION, 1L);
                curl_easy_setopt(curl_.get(), CURLOPT_MAXREDIRS, 5L);
                curl_easy_setopt(curl_.get(), CURLOPT_SSL_VERIFYPEER, 1L);
                curl_easy_setopt(curl_.get(), CURLOPT_SSL_VERIFYHOST, 2L);
            }
        } catch (const std::exception& e) {
            std::cerr << "❌ ModernCurlRAII: Initialization failed: " << e.what() << std::endl;
            isInitialized_ = false;
        } catch (...) {
            std::cerr << "❌ ModernCurlRAII: Unknown initialization error" << std::endl;
            isInitialized_ = false;
        }
    }
    
    ~ModernCurlRAII() {
        // RAII: Automatic cleanup
        try {
            if (mime_) {
                mime_.reset();
            }
            if (curl_) {
                curl_.reset();
            }
        } catch (...) {
            // Ignore exceptions in destructor
        }
    }
    
    bool isValid() const noexcept { 
        return isInitialized_ && curl_ != nullptr; 
    }
    
    CURL* get() const noexcept { 
        return curl_ ? curl_.get() : nullptr; 
    }
    
    // Modern C++17 move semantics
    ModernCurlRAII(ModernCurlRAII&& other) noexcept 
        : curl_(std::move(other.curl_)), mime_(std::move(other.mime_)), 
          isInitialized_(other.isInitialized_) {
        other.isInitialized_ = false;
    }
    
    ModernCurlRAII& operator=(ModernCurlRAII&& other) noexcept {
        if (this != &other) {
            curl_ = std::move(other.curl_);
            mime_ = std::move(other.mime_);
            isInitialized_ = other.isInitialized_;
            other.isInitialized_ = false;
        }
        return *this;
    }
    
    // Delete copy operations
    ModernCurlRAII(const ModernCurlRAII&) = delete;
    ModernCurlRAII& operator=(const ModernCurlRAII&) = delete;
    
    // Safe operations with error handling
    bool setUrl(const std::string& url) {
        if (!isValid()) return false;
        
        try {
            CURLcode res = curl_easy_setopt(curl_.get(), CURLOPT_URL, url.c_str());
            return (res == CURLE_OK);
        } catch (...) {
            return false;
        }
    }
    
    bool setTimeout(long timeoutSeconds) {
        if (!isValid()) return false;
        
        try {
            CURLcode res = curl_easy_setopt(curl_.get(), CURLOPT_TIMEOUT, timeoutSeconds);
            return (res == CURLE_OK);
        } catch (...) {
            return false;
        }
    }
    
    bool setWriteCallback(size_t (*callback)(void*, size_t, size_t, std::string*), std::string* response) {
        if (!isValid()) return false;
        
        try {
            CURLcode res1 = curl_easy_setopt(curl_.get(), CURLOPT_WRITEFUNCTION, callback);
            CURLcode res2 = curl_easy_setopt(curl_.get(), CURLOPT_WRITEDATA, response);
            return (res1 == CURLE_OK && res2 == CURLE_OK);
        } catch (...) {
            return false;
        }
    }
    
    std::optional<std::string> perform() {
        if (!isValid()) return std::nullopt;
        
        try {
            CURLcode res = curl_easy_perform(curl_.get());
            if (res == CURLE_OK) {
                long response_code;
                curl_easy_getinfo(curl_.get(), CURLINFO_RESPONSE_CODE, &response_code);
                return std::to_string(response_code);
            } else {
                std::cerr << "❌ ModernCurlRAII: CURL error: " << curl_easy_strerror(res) << std::endl;
                return std::nullopt;
            }
        } catch (const std::exception& e) {
            std::cerr << "❌ ModernCurlRAII: Exception during perform: " << e.what() << std::endl;
            return std::nullopt;
        } catch (...) {
            std::cerr << "❌ ModernCurlRAII: Unknown exception during perform" << std::endl;
            return std::nullopt;
        }
    }
    
    // MIME form handling
    bool initializeMime() {
        if (!isValid()) return false;
        
        try {
            mime_.reset(curl_mime_init(curl_.get()));
            return (mime_ != nullptr);
        } catch (...) {
            return false;
        }
    }
    
    bool addFileToMime(const std::string& fieldName, const std::string& filePath) {
        if (!isValid() || !mime_) return false;
        
        try {
            curl_mimepart* part = curl_mime_addpart(mime_.get());
            if (!part) return false;
            
            CURLcode res1 = curl_mime_filedata(part, filePath.c_str());
            CURLcode res2 = curl_mime_name(part, fieldName.c_str());
            
            return (res1 == CURLE_OK && res2 == CURLE_OK);
        } catch (...) {
            return false;
        }
    }
    
    bool addDataToMime(const std::string& fieldName, const std::string& data) {
        if (!isValid() || !mime_) return false;
        
        try {
            curl_mimepart* part = curl_mime_addpart(mime_.get());
            if (!part) return false;
            
            CURLcode res1 = curl_mime_data(part, data.c_str(), CURL_ZERO_TERMINATED);
            CURLcode res2 = curl_mime_name(part, fieldName.c_str());
            
            return (res1 == CURLE_OK && res2 == CURLE_OK);
        } catch (...) {
            return false;
        }
    }
    
    bool setMimePost() {
        if (!isValid() || !mime_) return false;
        
        try {
            CURLcode res = curl_easy_setopt(curl_.get(), CURLOPT_MIMEPOST, mime_.get());
            return (res == CURLE_OK);
        } catch (...) {
            return false;
        }
    }
};

// Modern C++17/20 Thread-safe File Handle with atomic operations
class ModernThreadSafeFileHandle {
private:
    std::string filePath_;
    mutable std::shared_mutex fileMutex_;
    std::atomic<bool> isOpen_{false};
    
public:
    explicit ModernThreadSafeFileHandle(const std::string& path) : filePath_(path) {}
    
    std::optional<std::unique_ptr<FILE, decltype(&std::fclose)>> openForRead() {
        std::unique_lock<std::shared_mutex> lock(fileMutex_);
        
        auto file = std::unique_ptr<FILE, decltype(&std::fclose)>(
            std::fopen(filePath_.c_str(), "rb"), &std::fclose);
        
        if (file) {
            isOpen_.store(true, std::memory_order_release);
            return std::move(file);
        }
        
        return std::nullopt;
    }
    
    bool exists() const {
        std::shared_lock<std::shared_mutex> lock(fileMutex_);
        return fs::exists(filePath_);
    }
    
    std::optional<uint64_t> getSize() const {
        std::shared_lock<std::shared_mutex> lock(fileMutex_);
        
        std::error_code ec;
        auto size = fs::file_size(filePath_, ec);
        if (ec) {
            return std::nullopt;
        }
        return size;
    }
    
    const std::string& getPath() const noexcept { return filePath_; }
};

// Modern C++17/20 Output Dispatcher with advanced safety
Output_Dispatcher::Output_Dispatcher() 
    : dispatchingActive_(false), dispatchTimeout_(300), maxRetries_(3),
      dispatchedCount_(0), failedCount_(0), serverUrl_("https://api.kronop.com/videos/upload") {
    std::cout << "📤 Output_Dispatcher initializing - Modern C++17/20 Edition" << std::endl;
    
    // Initialize CURL with modern error handling
    auto curlCode = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (curlCode != CURLE_OK) {
        std::cerr << "❌ Failed to initialize CURL: " << curl_easy_strerror(curlCode) << std::endl;
    }
    
    // Open log file with modern RAII
    logFile_.open("Output_Dispatcher_Log.txt", std::ios::app);
    if (logFile_.is_open()) {
        logFile_ << "\n=== Output_Dispatcher Session Started - Modern C++17/20 ===" << std::endl;
    }
}

Output_Dispatcher::~Output_Dispatcher() {
    stopDispatching();
    
    // Cleanup CURL
    curl_global_cleanup();
    
    if (logFile_.is_open()) {
        logFile_ << "=== Output_Dispatcher Session Ended ===" << std::endl;
        logFile_.close();
    }
}

void Output_Dispatcher::startDispatching() {
    if (dispatchingActive_.load(std::memory_order_acquire)) {
        std::cout << "⚠️ Output_Dispatcher: Dispatching already active" << std::endl;
        return;
    }
    
    dispatchingActive_.store(true, std::memory_order_release);
    dispatchingThread_ = std::thread(&Output_Dispatcher::dispatchingLoop, this);
    
    std::cout << "🚀 Output_Dispatcher: Dispatching started" << std::endl;
    if (logFile_.is_open()) {
        logFile_ << "Dispatching started with modern C++17/20 safety" << std::endl;
    }
}

void Output_Dispatcher::stopDispatching() {
    if (!dispatchingActive_.load(std::memory_order_acquire)) {
        return;
    }
    
    dispatchingActive_.store(false, std::memory_order_release);
    queueCondition_.notify_all();
    
    if (dispatchingThread_.joinable()) {
        dispatchingThread_.join();
    }
    
    std::cout << "🛑 Output_Dispatcher: Dispatching stopped" << std::endl;
    if (logFile_.is_open()) {
        logFile_ << "Dispatching stopped" << std::endl;
    }
}

bool Output_Dispatcher::receiveMergedVideo(const std::string& videoId, const std::string& videoPath, int sequenceIndex) {
    // Validate video file with modern error handling
    if (!validateVideoFile(videoPath)) {
        std::cerr << "❌ Output_Dispatcher: Invalid merged video file: " << videoPath << std::endl;
        return false;
    }
    
    // Create video output structure
    VideoOutput videoOutput;
    videoOutput.videoId = videoId;
    videoOutput.videoPath = videoPath;
    videoOutput.serverUrl = serverUrl_;
    
    // Get file size with modern error handling
    auto fileSize = fs::file_size(videoPath);
    videoOutput.fileSize = fileSize;
    
    videoOutput.sequenceIndex = sequenceIndex;
    videoOutput.processedTime = std::chrono::system_clock::now();
    videoOutput.isDispatched.store(false, std::memory_order_release);
    
    // Create dispatch job
    DispatchJob job;
    job.videoOutput = std::move(videoOutput);
    job.isCompleted.store(false, std::memory_order_release);
    job.dispatchTime = std::chrono::system_clock::now();
    job.retryCount.store(0, std::memory_order_release);
    
    // Add to queue with modern thread safety
    {
        auto lock = std::unique_lock<std::shared_mutex>(queueMutex_);
        dispatchQueue_.push(std::move(job));
    }
    
    queueCondition_.notify_one();
    
    std::cout << "📹 Output_Dispatcher: Merged video received - " << videoId 
              << " (Seq: " << sequenceIndex << ", Size: " 
              << (fileSize / 1024 / 1024) << " MB)" << std::endl;
    
    return true;
}

bool Output_Dispatcher::dispatchToServer(const VideoOutput& videoOutput) {
    std::cout << "📤 Output_Dispatcher: Dispatching video to server - " << videoOutput.videoId << std::endl;
    
    std::string serverResponse;
    bool success = uploadToServer(videoOutput, serverResponse);
    
    if (success) {
        dispatchedCount_.fetch_add(1, std::memory_order_acq_rel);
        std::cout << "✅ Output_Dispatcher: Video dispatched successfully - " << videoOutput.videoId << std::endl;
        logServerResponse(videoOutput.videoId, serverResponse);
    } else {
        failedCount_.fetch_add(1, std::memory_order_acq_rel);
        std::cerr << "❌ Output_Dispatcher: Failed to dispatch video - " << videoOutput.videoId << std::endl;
        logServerResponse(videoOutput.videoId, "FAILED: " + serverResponse);
    }
    
    return success;
}

bool Output_Dispatcher::uploadToServer(const VideoOutput& videoOutput, std::string& response) {
    try {
        // FIXED: Use RAII CURL wrapper for automatic cleanup
        ModernCurlRAII curl;
        if (!curl.isValid()) {
            response = "Failed to initialize CURL";
            return false;
        }
        
        // Validate video file first
        if (!validateVideoFile(videoOutput.videoPath)) {
            response = "Invalid video file";
            return false;
        }
        
        // Set server URL and timeout
        if (!curl.setUrl(serverUrl_)) {
            response = "Failed to set server URL";
            return false;
        }
        
        if (!curl.setTimeout(dispatchTimeout_)) {
            response = "Failed to set timeout";
            return false;
        }
        
        // Initialize MIME form
        if (!curl.initializeMime()) {
            response = "Failed to initialize MIME form";
            return false;
        }
        
        // Add video file to MIME
        if (!curl.addFileToMime("video", videoOutput.videoPath)) {
            response = "Failed to add video file to MIME";
            return false;
        }
        
        // Add metadata
        std::string metadata = "video_id=" + videoOutput.videoId + 
                              "&sequence=" + std::to_string(videoOutput.sequenceIndex) +
                              "&size=" + std::to_string(videoOutput.fileSize);
        
        if (!curl.addDataToMime("metadata", metadata)) {
            response = "Failed to add metadata to MIME";
            return false;
        }
        
        // Set MIME POST
        if (!curl.setMimePost()) {
            response = "Failed to set MIME POST";
            return false;
        }
        
        // Set write callback for response
        if (!curl.setWriteCallback(WriteCallback, &response)) {
            response = "Failed to set write callback";
            return false;
        }
        
        // Perform upload with RAII safety
        auto result = curl.perform();
        if (!result) {
            response = "Upload failed - network error";
            return false;
        }
        
        // Check server response
        return checkServerResponse(response);
        
    } catch (const std::exception& e) {
        response = "Exception during upload: " + std::string(e.what());
        return false;
    } catch (...) {
        response = "Unknown exception during upload";
        return false;
    }
}

bool Output_Dispatcher::validateVideoFile(const std::string& videoPath) {
    // Modern C++17 validation with error_code
    std::error_code ec;
    
    if (!fs::exists(videoPath, ec)) {
        if (ec) {
            std::cerr << "❌ Error checking file existence: " << ec.message() << std::endl;
        } else {
            std::cerr << "❌ Video file does not exist: " << videoPath << std::endl;
        }
        return false;
    }
    
    auto fileSize = fs::file_size(videoPath, ec);
    if (ec) {
        std::cerr << "❌ Error getting file size: " << ec.message() << std::endl;
        return false;
    }
    
    if (fileSize == 0) {
        std::cerr << "❌ Video file is empty: " << videoPath << std::endl;
        return false;
    }
    
    // Check if it's an MP4 file
    std::string extension = fs::path(videoPath).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    if (extension != ".mp4") {
        std::cerr << "❌ Expected MP4 file, got: " << extension << std::endl;
        return false;
    }
    
    return true;
}

void Output_Dispatcher::dispatchingLoop() {
    while (dispatchingActive_.load(std::memory_order_acquire)) {
        std::unique_lock<std::shared_mutex> lock(queueMutex_);
        
        // Wait for dispatch jobs
        queueCondition_.wait(lock, [this] { 
            return !dispatchQueue_.empty() || !dispatchingActive_.load(std::memory_order_acquire); 
        });
        
        if (!dispatchingActive_.load(std::memory_order_acquire)) break;
        
        if (dispatchQueue_.empty()) continue;
        
        // Get next job
        auto job = std::move(dispatchQueue_.front());
        dispatchQueue_.pop();
        lock.unlock();
        
        // Attempt dispatch
        bool success = dispatchToServer(job.videoOutput);
        
        if (success) {
            job.isCompleted.store(true, std::memory_order_release);
            logDispatchOperation(job);
            cleanupDispatchedJob(job);
        } else {
            // Handle failure with retry
            handleDispatchFailure(job);
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void Output_Dispatcher::handleDispatchFailure(DispatchJob& job) {
    auto currentRetry = job.retryCount.fetch_add(1, std::memory_order_acq_rel) + 1;
    
    if (currentRetry <= maxRetries_) {
        std::cout << "🔄 Output_Dispatcher: Retrying dispatch for " << job.videoOutput.videoId 
                  << " (Attempt " << currentRetry << "/" << maxRetries_ << ")" << std::endl;
        
        // Add delay before retry with exponential backoff
        auto delay = std::chrono::seconds(currentRetry * 5);
        std::this_thread::sleep_for(delay);
        
        // Re-queue for retry
        {
            auto lock = std::unique_lock<std::shared_mutex>(queueMutex_);
            dispatchQueue_.push(std::move(job));
        }
        queueCondition_.notify_one();
    } else {
        std::cerr << "❌ Output_Dispatcher: Max retries exceeded for " << job.videoOutput.videoId << std::endl;
        logDispatchOperation(job);
    }
}

void Output_Dispatcher::cleanupDispatchedJob(const DispatchJob& job) {
    // Optionally remove the local file after successful dispatch
    try {
        std::error_code ec;
        if (fs::exists(job.videoOutput.videoPath, ec)) {
            // Uncomment the line below to automatically delete files after dispatch
            // fs::remove(job.videoOutput.videoPath, ec);
            if (ec) {
                std::cerr << "⚠️ Error checking file existence for cleanup: " << ec.message() << std::endl;
            } else {
                std::cout << "📁 Output_Dispatcher: Keeping local file: " << job.videoOutput.videoPath << std::endl;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "⚠️ Exception during cleanup: " << e.what() << std::endl;
    }
}

// Modern C++17/20 CURL callback function
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* response) {
    try {
        size_t totalSize = size * nmemb;
        response->append(static_cast<char*>(contents), totalSize);
        return totalSize;
    } catch (...) {
        return 0; // Signal error to CURL
    }
}

bool Output_Dispatcher::checkServerResponse(const std::string& response) {
    // Check if server response indicates success
    if (response.find("success") != std::string::npos ||
        response.find("\"status\":\"ok\"") != std::string::npos ||
        response.find("\"code\":200") != std::string::npos) {
        return true;
    }
    
    return false;
}

void Output_Dispatcher::logDispatchOperation(const DispatchJob& job) {
    if (logFile_.is_open()) {
        auto time_t = std::chrono::system_clock::to_time_t(job.dispatchTime);
        logFile_ << "Video Dispatched: " << job.videoOutput.videoId 
                << " | Path: " << job.videoOutput.videoPath
                << " | Sequence: " << job.videoOutput.sequenceIndex
                << " | Size: " << (job.videoOutput.fileSize / 1024 / 1024) << " MB"
                << " | Server: " << job.videoOutput.serverUrl
                << " | Retries: " << job.retryCount.load(std::memory_order_acquire)
                << " | Time: " << std::ctime(&time_t);
    }
}

void Output_Dispatcher::logServerResponse(const std::string& videoId, const std::string& response) {
    if (logFile_.is_open()) {
        auto time_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        logFile_ << "Server Response for " << videoId << ": " << response 
                << " | Time: " << std::ctime(&time_t);
    }
}

// Modern C++17/20 status methods with atomic operations
void Output_Dispatcher::setServerUrl(const std::string& serverUrl) {
    serverUrl_ = serverUrl;
    std::cout << "🌐 Output_Dispatcher: Server URL updated - " << serverUrl_ << std::endl;
}

void Output_Dispatcher::setDispatchTimeout(int timeoutSeconds) {
    dispatchTimeout_ = timeoutSeconds;
    std::cout << "⏱️ Output_Dispatcher: Dispatch timeout set to " << timeoutSeconds << " seconds" << std::endl;
}

int Output_Dispatcher::getQueueSize() const {
    auto lock = std::shared_lock<std::shared_mutex>(queueMutex_);
    return static_cast<int>(dispatchQueue_.size());
}

int Output_Dispatcher::getDispatchedCount() const {
    return dispatchedCount_.load(std::memory_order_acquire);
}

int Output_Dispatcher::getFailedCount() const {
    return failedCount_.load(std::memory_order_acquire);
}

bool Output_Dispatcher::isDispatchingActive() const {
    return dispatchingActive_.load(std::memory_order_acquire);
}

void Output_Dispatcher::getDispatchStatus(std::string& status) {
    if (dispatchingActive_.load(std::memory_order_acquire)) {
        status = "📤 Output_Dispatcher: Active (Queue: " + std::to_string(getQueueSize()) + 
                ", Success: " + std::to_string(getDispatchedCount()) + 
                ", Failed: " + std::to_string(getFailedCount()) + ")";
    } else {
        status = "🛑 Output_Dispatcher: Inactive";
    }
}

std::string Output_Dispatcher::getServerEndpoint() {
    return serverUrl_;
}
