#ifndef RTN_VIDEO_AI_HPP
#define RTN_VIDEO_AI_HPP

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <future>

#ifdef __ANDROID__
    #include <jni.h>
    #include <android/log.h>
    #define LOG_TAG "RTN_Kronop"
    #define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
    #define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#elif defined(__APPLE__)
    #include <os/log.h>
    #include <Foundation/Foundation.h>
    static os_log_t os_log;
    #define LOGI(fmt, ...) os_log_with_type(os_log, OS_LOG_TYPE_INFO, fmt, ##__VA_ARGS__)
    #define LOGE(fmt, ...) os_log_with_type(os_log, OS_LOG_TYPE_ERROR, fmt, ##__VA_ARGS__)
#else
    #include <iostream>
    #define LOGI(...) std::cout << "[RTN_Kronop] " << __VA_ARGS__ << std::endl
    #define LOGE(...) std::cerr << "[RTN_Kronop] " << __VA_ARGS__ << std::endl
#endif

// Forward declarations
class Video_AI_Coordinator;
class ThermalMonitor;
class WorkerManager;
class SharedMemoryManager;

/**
 * React Native Bridge for Kronop Video AI
 * Cross-platform C++ core for Android & iOS
 */

enum ProcessingStatus {
    IDLE = 0,
    PROCESSING = 1,
    COMPLETED = 2,
    ERROR = 3,
    PAUSED = 4
};

struct VideoProcessingResult {
    std::string videoId;
    ProcessingStatus status;
    std::string outputPath;
    std::string errorMessage;
    float progressPercentage;
    int framesProcessed;
    int totalFrames;
    std::chrono::system_clock::time_point startTime;
    std::chrono::system_clock::time_point endTime;
};

struct ThermalStatus {
    float temperature;
    std::string status;
    std::vector<int> pausedWorkers;
    std::vector<int> activeWorkers;
    bool emergencyMode;
};

struct WorkerStatus {
    int workerId;
    std::string state;
    int currentChunkId;
    float progress;
    bool isPaused;
    std::string pauseReason;
};

class RTNVideoAI {
private:
    // Core components
    std::unique_ptr<Video_AI_Coordinator> coordinator_;
    std::unique_ptr<ThermalMonitor> thermalMonitor_;
    std::unique_ptr<WorkerManager> workerManager_;
    std::unique_ptr<SharedMemoryManager> sharedMemory_;
    
    // Platform-specific handles
#ifdef __ANDROID__
    JavaVM* javaVM_;
    jobject applicationContext_;
#elif defined(__APPLE__)
    void* iosContext_;
#endif
    
    // Callbacks for React Native
    std::function<void(const VideoProcessingResult&)> onProgressCallback_;
    std::function<void(const ThermalStatus&)> onThermalCallback_;
    std::function<void(const std::vector<WorkerStatus>&)> onWorkerCallback_;
    
    // State management
    std::atomic<bool> initialized_;
    std::atomic<bool> processingActive_;
    std::string currentProcessingVideo_;
    
    // Configuration
    std::string inputDirectory_;
    std::string outputDirectory_;
    std::string tempDirectory_;
    int maxConcurrentJobs_;
    
public:
    RTNVideoAI();
    ~RTNVideoAI();
    
    // Platform-specific initialization
#ifdef __ANDROID__
    bool initialize(JavaVM* vm, jobject context, const std::string& inputDir, const std::string& outputDir);
#elif defined(__APPLE__)
    bool initialize(void* iosContext, const std::string& inputDir, const std::string& outputDir);
#endif
    
    // Core video processing
    bool processVideo(const std::string& videoPath);
    bool processVideoAsync(const std::string& videoPath, std::function<void(const VideoProcessingResult&)> callback);
    bool cancelProcessing(const std::string& videoId);
    VideoProcessingResult getProcessingStatus(const std::string& videoId);
    
    // Thermal management
    ThermalStatus getThermalStatus();
    bool setThermalThresholds(float warning, float critical, float emergency);
    std::vector<int> getPausedWorkers();
    bool resumeWorker(int workerId);
    bool pauseWorker(int workerId);
    
    // Worker management
    std::vector<WorkerStatus> getAllWorkerStatus();
    int getActiveWorkerCount();
    bool setMaxWorkers(int maxWorkers);
    
    // Memory management
    double getMemoryUsage();
    bool clearCache();
    bool optimizeMemory();
    
    // Configuration
    bool setServerUrl(const std::string& serverUrl);
    bool setProcessingTimeout(int timeoutSeconds);
    bool enableDebugMode(bool enable);
    
    // Callback registration
    void setProgressCallback(std::function<void(const VideoProcessingResult&)> callback);
    void setThermalCallback(std::function<void(const ThermalStatus&)> callback);
    void setWorkerCallback(std::function<void(const std::vector<WorkerStatus>&)> callback);
    
    // System information
    std::string getSystemInfo();
    std::vector<std::string> getSupportedFormats();
    bool isInitialized() const { return initialized_; }
    bool isProcessing() const { return processingActive_; }

private:
    // Internal methods
    bool initializeCoreComponents();
    void setupCallbacks();
    void startMonitoring();
    void stopMonitoring();
    
    // Platform-specific implementations
#ifdef __ANDROID__
    bool initializeAndroidJNI(JavaVM* vm, jobject context);
    std::string getAndroidExternalPath();
#elif defined(__APPLE__)
    bool initializeiOSContext(void* context);
    std::string getiOSDocumentsPath();
    float getiOSTemperature();
#endif
    
    // Callback handlers
    void handleProgressUpdate(const VideoProcessingResult& result);
    void handleThermalAlert(const ThermalStatus& status);
    void handleWorkerUpdate(const std::vector<WorkerStatus>& workers);
    
    // Utility methods
    std::string generateVideoId(const std::string& videoPath);
    bool validateVideoPath(const std::string& videoPath);
    void logEvent(const std::string& event, const std::string& details);
};

// React Native JSI Interface
#ifdef JSI
#include <jsi/jsi.h>

class RTNVideoAIJSI {
public:
    static void registerJSIBindings(facebook::jsi::Runtime& runtime);
    
private:
    static facebook::jsi::Value processVideo(facebook::jsi::Runtime& runtime, 
                                             const facebook::jsi::Value& thisValue,
                                             const facebook::jsi::Value* args,
                                             size_t count);
    
    static facebook::jsi::Value getThermalStatus(facebook::jsi::Runtime& runtime,
                                                  const facebook::jsi::Value& thisValue,
                                                  const facebook::jsi::Value* args,
                                                  size_t count);
    
    static facebook::jsi::Value getWorkerStatus(facebook::jsi::Runtime& runtime,
                                                 const facebook::jsi::Value& thisValue,
                                                 const facebook::jsi::Value* args,
                                                 size_t count);
    
    static facebook::jsi::Value cancelProcessing(facebook::jsi::Runtime& runtime,
                                                const facebook::jsi::Value& thisValue,
                                                const facebook::jsi::Value* args,
                                                size_t count);
};

#endif

#endif // RTN_VIDEO_AI_HPP
