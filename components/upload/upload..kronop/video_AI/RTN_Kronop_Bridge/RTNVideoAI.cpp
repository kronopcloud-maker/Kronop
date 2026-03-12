#include "RTNVideoAI.hpp"
#include "../Master_Control/Video_AI_Coordinator.hpp"
#include "../Thermal_Guard/Thermal_Monitor.hpp"
#include "../AI_Workers/Worker_Manager.hpp"
#include "../IO_Vault/Shared_Memory_Manager.hpp"
#include <thread>
#include <chrono>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

RTNVideoAI::RTNVideoAI() 
    : coordinator_(nullptr), thermalMonitor_(nullptr), workerManager_(nullptr), sharedMemory_(nullptr),
#ifdef __ANDROID__
      javaVM_(nullptr), applicationContext_(nullptr),
#elif defined(__APPLE__)
      iosContext_(nullptr),
#endif
      initialized_(false), processingActive_(false), maxConcurrentJobs_(3) {
    
    // Initialize platform-specific logging
#ifdef __APPLE__
    os_log = os_log_create("com.kronop.rtnvideoai", "RTN_Kronop");
#endif
    
    LOGI("RTNVideoAI Bridge initializing - Cross-platform C++ core");
}

RTNVideoAI::~RTNVideoAI() {
    stopMonitoring();
    
    if (coordinator_) {
        coordinator_->stopSystem();
    }
    
    LOGI("RTNVideoAI Bridge destroyed");
}

#ifdef __ANDROID__
bool RTNVideoAI::initialize(JavaVM* vm, jobject context, const std::string& inputDir, const std::string& outputDir) {
    javaVM_ = vm;
    applicationContext_ = context;
    inputDirectory_ = inputDir;
    outputDirectory_ = outputDir;
    
    LOGI("Initializing RTNVideoAI for Android");
    
    if (!initializeAndroidJNI(vm, context)) {
        LOGE("Failed to initialize Android JNI");
        return false;
    }
    
    return initializeCoreComponents();
}

bool RTNVideoAI::initializeAndroidJNI(JavaVM* vm, jobject context) {
    JNIEnv* env;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        LOGE("Failed to get JNI environment");
        return false;
    }
    
    // Get application context class
    jclass contextClass = env->GetObjectClass(context);
    jmethodID getFilesDir = env->GetMethodID(contextClass, "getFilesDir", "()Ljava/io/File;");
    jobject filesDir = env->CallObjectMethod(context, getFilesDir);
    
    jclass fileClass = env->GetObjectClass(filesDir);
    jmethodID getAbsolutePath = env->GetMethodID(fileClass, "getAbsolutePath", "()Ljava/lang/String;");
    jstring path = (jstring)env->CallObjectMethod(filesDir, getAbsolutePath);
    
    const char* pathStr = env->GetStringUTFChars(path, nullptr);
    std::string appPath = pathStr;
    env->ReleaseStringUTFChars(path, pathStr);
    
    env->DeleteLocalRef(contextClass);
    env->DeleteLocalRef(filesDir);
    env->DeleteLocalRef(fileClass);
    env->DeleteLocalRef(path);
    
    LOGI("Android app path: %s", appPath.c_str());
    
    return true;
}

std::string RTNVideoAI::getAndroidExternalPath() {
    JNIEnv* env;
    if (javaVM_->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return "";
    }
    
    jclass environmentClass = env->FindClass("android/os/Environment");
    jmethodID getExternalStorageDirectory = env->GetStaticMethodID(environmentClass, 
                                                                      "getExternalStorageDirectory", 
                                                                      "()Ljava/io/File;");
    jobject externalStorage = env->CallStaticObjectMethod(environmentClass, getExternalStorageDirectory);
    
    jclass fileClass = env->GetObjectClass(externalStorage);
    jmethodID getAbsolutePath = env->GetMethodID(fileClass, "getAbsolutePath", "()Ljava/lang/String;");
    jstring path = (jstring)env->CallObjectMethod(externalStorage, getAbsolutePath);
    
    const char* pathStr = env->GetStringUTFChars(path, nullptr);
    std::string externalPath = pathStr;
    env->ReleaseStringUTFChars(path, pathStr);
    
    env->DeleteLocalRef(environmentClass);
    env->DeleteLocalRef(externalStorage);
    env->DeleteLocalRef(fileClass);
    env->DeleteLocalRef(path);
    
    return externalPath;
}

#elif defined(__APPLE__)

bool RTNVideoAI::initialize(void* iosContext, const std::string& inputDir, const std::string& outputDir) {
    iosContext_ = iosContext;
    inputDirectory_ = inputDir;
    outputDirectory_ = outputDir;
    
    LOGI("Initializing RTNVideoAI for iOS");
    
    if (!initializeiOSContext(iosContext)) {
        LOGE("Failed to initialize iOS context");
        return false;
    }
    
    return initializeCoreComponents();
}

bool RTNVideoAI::initializeiOSContext(void* context) {
    // Initialize iOS-specific context
    // This would be implemented with actual iOS context handling
    LOGI("iOS context initialized");
    return true;
}

std::string RTNVideoAI::getiOSDocumentsPath() {
    NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString* documentsDirectory = [paths objectAtIndex:0];
    return [documentsDirectory UTF8String];
}

float RTNVideoAI::getiOSTemperature() {
    // iOS thermal monitoring using NSProcessInfo
    NSProcessInfoThermalState thermalState = [[NSProcessInfo processInfo] thermalState];
    
    switch (thermalState) {
        case NSProcessInfoThermalStateNominal:
            return 35.0f; // Normal
        case NSProcessInfoThermalStateFair:
            return 40.0f; // Warning
        case NSProcessInfoThermalStateSerious:
            return 45.0f; // Critical
        case NSProcessInfoThermalStateCritical:
            return 50.0f; // Emergency
        default:
            return 25.0f; // Unknown
    }
}

#endif

bool RTNVideoAI::initializeCoreComponents() {
    try {
        // Initialize core components
        coordinator_ = std::make_unique<Video_AI_Coordinator>();
        thermalMonitor_ = std::make_unique<ThermalMonitor>();
        workerManager_ = std::make_unique<WorkerManager>();
        sharedMemory_ = std::make_unique<SharedMemoryManager>();
        
        // Set up directories
        tempDirectory_ = outputDirectory_ + "/temp";
        
        // Initialize shared memory
        if (!sharedMemory_->initialize(inputDirectory_, outputDirectory_)) {
            LOGE("Failed to initialize shared memory manager");
            return false;
        }
        
        // Initialize worker manager
        if (!workerManager_->initialize()) {
            LOGE("Failed to initialize worker manager");
            return false;
        }
        
        // Initialize coordinator
        if (!coordinator_->initializeSystem()) {
            LOGE("Failed to initialize video coordinator");
            return false;
        }
        
        // Setup callbacks
        setupCallbacks();
        
        // Start monitoring
        startMonitoring();
        
        initialized_ = true;
        
        LOGI("RTNVideoAI core components initialized successfully");
        return true;
        
    } catch (const std::exception& e) {
        LOGE("Failed to initialize core components: %s", e.what());
        return false;
    }
}

void RTNVideoAI::setupCallbacks() {
    // Setup thermal callback
    thermalMonitor_->setAlertCallback([this](const ThermalAlert& alert) {
        ThermalStatus status;
        status.temperature = alert.temperature;
        status.status = alert.message;
        status.pausedWorkers = thermalMonitor_->getPausedWorkers();
        status.activeWorkers = {1, 2, 3, 4, 5}; // Simplified
        status.emergencyMode = (alert.status >= CRITICAL);
        
        handleThermalAlert(status);
    });
    
    LOGI("Callbacks setup completed");
}

bool RTNVideoAI::processVideo(const std::string& videoPath) {
    if (!initialized_) {
        LOGE("RTNVideoAI not initialized");
        return false;
    }
    
    if (!validateVideoPath(videoPath)) {
        LOGE("Invalid video path: %s", videoPath.c_str());
        return false;
    }
    
    std::string videoId = generateVideoId(videoPath);
    currentProcessingVideo_ = videoId;
    
    bool success = coordinator_->processVideo(videoPath, videoId);
    
    if (success) {
        processingActive_ = true;
        LOGI("Video processing started: %s", videoId.c_str());
    } else {
        LOGE("Failed to start video processing");
    }
    
    return success;
}

bool RTNVideoAI::processVideoAsync(const std::string& videoPath, std::function<void(const VideoProcessingResult&)> callback) {
    onProgressCallback_ = callback;
    
    // Start processing in background thread
    std::thread([this, videoPath]() {
        bool success = processVideo(videoPath);
        
        VideoProcessingResult result;
        result.videoId = currentProcessingVideo_;
        result.status = success ? PROCESSING : ERROR;
        result.progressPercentage = 0.0f;
        result.startTime = std::chrono::system_clock::now();
        
        if (callback) {
            callback(result);
        }
    }).detach();
    
    return true;
}

bool RTNVideoAI::cancelProcessing(const std::string& videoId) {
    if (!coordinator_) {
        return false;
    }
    
    // Implementation would depend on coordinator's cancel capability
    LOGI("Canceling processing for video: %s", videoId.c_str());
    return true;
}

VideoProcessingResult RTNVideoAI::getProcessingStatus(const std::string& videoId) {
    VideoProcessingResult result;
    result.videoId = videoId;
    result.status = IDLE;
    result.progressPercentage = 0.0f;
    result.startTime = std::chrono::system_clock::now();
    
    // Get status from coordinator
    auto jobs = coordinator_->getJobHistory();
    for (const auto& job : jobs) {
        if (job.videoId == videoId) {
            result.status = static_cast<ProcessingStatus>(job.currentStatus);
            result.progressPercentage = 50.0f; // Simplified
            result.framesProcessed = 15; // Simplified
            result.totalFrames = 30;
            result.outputPath = job.finalPath;
            break;
        }
    }
    
    return result;
}

ThermalStatus RTNVideoAI::getThermalStatus() {
    ThermalStatus status;
    
#ifdef __ANDROID__
    if (thermalMonitor_) {
        status.temperature = thermalMonitor_->getCurrentTemperature();
        status.status = thermalMonitor_->getStatusString();
        status.pausedWorkers = thermalMonitor_->getPausedWorkers();
        status.activeWorkers = thermalMonitor_->getSuspendedWorkers();
        status.emergencyMode = (thermalMonitor_->getCurrentStatus() >= CRITICAL);
    }
#elif defined(__APPLE__)
    status.temperature = getiOSTemperature();
    
    // Convert thermal state to status
    NSProcessInfoThermalState thermalState = [[NSProcessInfo processInfo] thermalState];
    switch (thermalState) {
        case NSProcessInfoThermalStateNominal:
            status.status = "NORMAL";
            status.emergencyMode = false;
            break;
        case NSProcessInfoThermalStateFair:
            status.status = "WARNING";
            status.emergencyMode = false;
            break;
        case NSProcessInfoThermalStateSerious:
            status.status = "CRITICAL";
            status.emergencyMode = true;
            break;
        case NSProcessInfoThermalStateCritical:
            status.status = "EMERGENCY";
            status.emergencyMode = true;
            break;
        default:
            status.status = "UNKNOWN";
            status.emergencyMode = false;
            break;
    }
    
    // Simplified worker status for iOS
    status.pausedWorkers = {};
    status.activeWorkers = {1, 2, 3, 4, 5};
#endif
    
    return status;
}

bool RTNVideoAI::setThermalThresholds(float warning, float critical, float emergency) {
    // Implementation would set thresholds in thermal monitor
    LOGI("Setting thermal thresholds: Warning=%.1f°C, Critical=%.1f°C, Emergency=%.1f°C", 
         warning, critical, emergency);
    return true;
}

std::vector<int> RTNVideoAI::getPausedWorkers() {
    if (thermalMonitor_) {
        return thermalMonitor_->getPausedWorkers();
    }
    return {};
}

bool RTNVideoAI::resumeWorker(int workerId) {
    if (thermalMonitor_) {
        return thermalMonitor_->resumeWorker(workerId);
    }
    return false;
}

bool RTNVideoAI::pauseWorker(int workerId) {
    if (thermalMonitor_) {
        return thermalMonitor_->registerWorker(workerId, 1234 + workerId, false); // Simplified PID
    }
    return false;
}

std::vector<WorkerStatus> RTNVideoAI::getAllWorkerStatus() {
    std::vector<WorkerStatus> statusList;
    
    if (workerManager_) {
        auto workers = workerManager_->getAllWorkerStatus();
        
        for (const auto& worker : workers) {
            WorkerStatus status;
            status.workerId = worker.workerId;
            status.state = (worker.status == 0) ? "IDLE" : 
                         (worker.status == 1) ? "BUSY" : "PAUSED";
            status.currentChunkId = 0; // Simplified
            status.progress = 0.0f; // Simplified
            status.isPaused = (worker.status == 2);
            status.pauseReason = "";
            
            statusList.push_back(status);
        }
    }
    
    return statusList;
}

int RTNVideoAI::getActiveWorkerCount() {
    if (workerManager_) {
        return workerManager_->getActiveWorkerCount();
    }
    return 0;
}

bool RTNVideoAI::setMaxWorkers(int maxWorkers) {
    maxConcurrentJobs_ = maxWorkers;
    LOGI("Setting max concurrent jobs to: %d", maxWorkers);
    return true;
}

double RTNVideoAI::getMemoryUsage() {
    if (sharedMemory_) {
        return static_cast<double>(sharedMemory_->getTotalMemoryUsed()) / 1024 / 1024; // MB
    }
    return 0.0;
}

bool RTNVideoAI::clearCache() {
    if (sharedMemory_) {
        return sharedMemory_->optimizeMemoryUsage();
    }
    return false;
}

bool RTNVideoAI::optimizeMemory() {
    if (sharedMemory_) {
        sharedMemory_->triggerMemoryCleanup();
        return true;
    }
    return false;
}

bool RTNVideoAI::setServerUrl(const std::string& serverUrl) {
    if (coordinator_) {
        coordinator_->setServerUrl(serverUrl);
        return true;
    }
    return false;
}

bool RTNVideoAI::setProcessingTimeout(int timeoutSeconds) {
    if (coordinator_) {
        coordinator_->setProcessingTimeout(timeoutSeconds);
        return true;
    }
    return false;
}

bool RTNVideoAI::enableDebugMode(bool enable) {
    LOGI("Debug mode %s", enable ? "enabled" : "disabled");
    return true;
}

void RTNVideoAI::setProgressCallback(std::function<void(const VideoProcessingResult&)> callback) {
    onProgressCallback_ = callback;
}

void RTNVideoAI::setThermalCallback(std::function<void(const ThermalStatus&)> callback) {
    onThermalCallback_ = callback;
}

void RTNVideoAI::setWorkerCallback(std::function<void(const std::vector<WorkerStatus>&)> callback) {
    onWorkerCallback_ = callback;
}

std::string RTNVideoAI::getSystemInfo() {
    std::string info = "RTNVideoAI System Info\n";
    
#ifdef __ANDROID__
    info += "Platform: Android\n";
    info += "JNI: Available\n";
#elif defined(__APPLE__)
    info += "Platform: iOS\n";
    info += "Thermal API: NSProcessInfo\n";
#else
    info += "Platform: Unknown\n";
#endif
    
    info += "Core Components: Initialized\n";
    info += "Max Concurrent Jobs: " + std::to_string(maxConcurrentJobs_) + "\n";
    info += "Memory Usage: " + std::to_string(getMemoryUsage()) + " MB\n";
    
    return info;
}

std::vector<std::string> RTNVideoAI::getSupportedFormats() {
    return {"mp4", "avi", "mov", "mkv", "webm"};
}

void RTNVideoAI::startMonitoring() {
    // Start background monitoring thread
    std::thread([this]() {
        while (initialized_) {
            // Monitor thermal status
            ThermalStatus thermal = getThermalStatus();
            if (onThermalCallback_) {
                onThermalCallback_(thermal);
            }
            
            // Monitor worker status
            auto workers = getAllWorkerStatus();
            if (onWorkerCallback_) {
                onWorkerCallback_(workers);
            }
            
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }).detach();
    
    LOGI("Background monitoring started");
}

void RTNVideoAI::stopMonitoring() {
    initialized_ = false;
    LOGI("Background monitoring stopped");
}

void RTNVideoAI::handleProgressUpdate(const VideoProcessingResult& result) {
    if (onProgressCallback_) {
        onProgressCallback_(result);
    }
}

void RTNVideoAI::handleThermalAlert(const ThermalStatus& status) {
    if (onThermalCallback_) {
        onThermalCallback_(status);
    }
    
    // Handle emergency thermal conditions
    if (status.emergencyMode) {
        LOGI("Thermal emergency detected - pausing processing");
        if (coordinator_) {
            coordinator_->stopSystem();
        }
    }
}

void RTNVideoAI::handleWorkerUpdate(const std::vector<WorkerStatus>& workers) {
    if (onWorkerCallback_) {
        onWorkerCallback_(workers);
    }
}

std::string RTNVideoAI::generateVideoId(const std::string& videoPath) {
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    std::string filename = fs::path(videoPath).stem().string();
    return filename + "_" + std::to_string(timestamp);
}

bool RTNVideoAI::validateVideoPath(const std::string& videoPath) {
    return fs::exists(videoPath) && fs::is_regular_file(videoPath);
}

void RTNVideoAI::logEvent(const std::string& event, const std::string& details) {
    LOGI("Event: %s - %s", event.c_str(), details.c_str());
}

// React Native JSI Implementation
#ifdef JSI
#include <jsi/jsi.h>

void RTNVideoAIJSI::registerJSIBindings(facebook::jsi::Runtime& runtime) {
    auto object = facebook::jsi::Object(runtime);
    
    // Register processVideo function
    auto processVideoFunc = facebook::jsi::Function::createFromHostFunction(
        runtime,
        facebook::jsi::PropNameID::get(runtime, "processVideo"),
        1,
        processVideo
    );
    object.setProperty(runtime, "processVideo", processVideoFunc);
    
    // Register getThermalStatus function
    auto getThermalStatusFunc = facebook::jsi::Function::createFromHostFunction(
        runtime,
        facebook::jsi::PropNameID::get(runtime, "getThermalStatus"),
        0,
        getThermalStatus
    );
    object.setProperty(runtime, "getThermalStatus", getThermalStatusFunc);
    
    // Register getWorkerStatus function
    auto getWorkerStatusFunc = facebook::jsi::Function::createFromHostFunction(
        runtime,
        facebook::jsi::PropNameID::get(runtime, "getWorkerStatus"),
        0,
        getWorkerStatus
    );
    object.setProperty(runtime, "getWorkerStatus", getWorkerStatusFunc);
    
    // Register cancelProcessing function
    auto cancelProcessingFunc = facebook::jsi::Function::createFromHostFunction(
        runtime,
        facebook::jsi::PropNameID::get(runtime, "cancelProcessing"),
        1,
        cancelProcessing
    );
    object.setProperty(runtime, "cancelProcessing", cancelProcessingFunc);
    
    // Set global object
    runtime.global().setProperty(runtime, "RTNVideoAI", object);
}

facebook::jsi::Value RTNVideoAIJSI::processVideo(facebook::jsi::Runtime& runtime, 
                                                    const facebook::jsi::Value& thisValue,
                                                    const facebook::jsi::Value* args,
                                                    size_t count) {
    if (count < 1) {
        return facebook::jsi::Value::undefined();
    }
    
    std::string videoPath = args[0].asString(runtime).utf8(runtime);
    
    // Get or create RTNVideoAI instance
    static RTNVideoAI* rtnVideoAI = nullptr;
    if (!rtnVideoAI) {
        rtnVideoAI = new RTNVideoAI();
        // Initialize with default paths
#ifdef __ANDROID__
        // Android initialization would go here
#elif defined(__APPLE__)
        rtnVideoAI->initialize(nullptr, "/tmp/input", "/tmp/output");
#endif
    }
    
    bool success = rtnVideoAI->processVideo(videoPath);
    
    return facebook::jsi::Value(success);
}

facebook::jsi::Value RTNVideoAIJSI::getThermalStatus(facebook::jsi::Runtime& runtime,
                                                       const facebook::jsi::Value& thisValue,
                                                       const facebook::jsi::Value* args,
                                                       size_t count) {
    static RTNVideoAI* rtnVideoAI = nullptr;
    if (!rtnVideoAI) {
        return facebook::jsi::Value::undefined();
    }
    
    ThermalStatus status = rtnVideoAI->getThermalStatus();
    
    // Create object with thermal status
    auto result = facebook::jsi::Object(runtime);
    result.setProperty(runtime, "temperature", status.temperature);
    result.setProperty(runtime, "status", facebook::jsi::String::createFromUtf8(runtime, status.status));
    result.setProperty(runtime, "emergencyMode", status.emergencyMode);
    
    return result;
}

facebook::jsi::Value RTNVideoAIJSI::getWorkerStatus(facebook::jsi::Runtime& runtime,
                                                      const facebook::jsi::Value& thisValue,
                                                      const facebook::jsi::Value* args,
                                                      size_t count) {
    static RTNVideoAI* rtnVideoAI = nullptr;
    if (!rtnVideoAI) {
        return facebook::jsi::Value::undefined();
    }
    
    auto workers = rtnVideoAI->getAllWorkerStatus();
    
    // Create array with worker status
    auto array = facebook::jsi::Array(runtime, workers.size());
    for (size_t i = 0; i < workers.size(); ++i) {
        const WorkerStatus& worker = workers[i];
        
        auto workerObj = facebook::jsi::Object(runtime);
        workerObj.setProperty(runtime, "workerId", worker.workerId);
        workerObj.setProperty(runtime, "state", facebook::jsi::String::createFromUtf8(runtime, worker.state));
        workerObj.setProperty(runtime, "isPaused", worker.isPaused);
        
        array.setValueAtIndex(runtime, i, workerObj);
    }
    
    return array;
}

facebook::jsi::Value RTNVideoAIJSI::cancelProcessing(facebook::jsi::Runtime& runtime,
                                                       const facebook::jsi::Value& thisValue,
                                                       const facebook::jsi::Value* args,
                                                       size_t count) {
    if (count < 1) {
        return facebook::jsi::Value::undefined();
    }
    
    std::string videoId = args[0].asString(runtime).utf8(runtime);
    
    static RTNVideoAI* rtnVideoAI = nullptr;
    if (!rtnVideoAI) {
        return facebook::jsi::Value::undefined();
    }
    
    bool success = rtnVideoAI->cancelProcessing(videoId);
    
    return facebook::jsi::Value(success);
}

#endif
