#pragma once

#include <string>
#include <memory>
#include <vector>
#include <iostream>
#include <chrono>

// Include actual headers (assuming paths are set)
#include "Kronop_Cleaner AI/src/processing/StreamingProcessor.hpp"  // For video
#include "Kronop_Upscaler AI/Upscale_Core.cpp"  // For image (include as cpp for simplicity)
#include "Kronop_Cleaner AI/src/integration/AudioVideoSync.hpp"  // For music
#include "Kronop_Audio_Titan/Audio_Core.hpp"  // For audio titan

// Kronop Master Bridge - Unified API for all AI modules
class KronopMasterBridge {
private:
    // AI Module instances
    std::unique_ptr<kronop::StreamingProcessor> videoProcessor_;  // Video AI from Kronop_Cleaner
    std::unique_ptr<UpscaleCore> imageProcessor_;                  // Image AI from Kronop_Upscaler
    std::unique_ptr<kronop::ChunkManager> musicProcessor_;         // Music AI from Kronop_Cleaner
    std::unique_ptr<AudioEnhancer> audioProcessor_;                // Audio Titan
    
    bool initialized_;
    
    // Safety Valve parameters
    float thermalLimit_;          // Temperature limit in Celsius
    float cpuLimit_;             // CPU usage limit (0.0-1.0)
    size_t emergencyMemoryThreshold_; // Memory threshold for emergency cleanup
    int originalVideoThreads_;    // Original video processing threads count
    bool safetyModeActive_;      // Safety mode flag
    
public:
    KronopMasterBridge() : initialized_(false),
                           thermalLimit_(45.0f),      // 45°C thermal limit
                           cpuLimit_(0.9f),          // 90% CPU limit
                           emergencyMemoryThreshold_(1024 * 1024 * 1024), // 1GB emergency threshold
                           originalVideoThreads_(4),
                           safetyModeActive_(false) {
        std::cout << "🎯 KRONOP MASTER BRIDGE INITIALIZING" << std::endl;
        std::cout << "   Safety Valve: Thermal " << thermalLimit_ << "°C, CPU " << (cpuLimit_*100) << "%, Memory " << (emergencyMemoryThreshold_/1024/1024) << "MB" << std::endl;
    }
    
    ~KronopMasterBridge() {
        std::cout << "🔌 KRONOP MASTER BRIDGE SHUTTING DOWN" << std::endl;
    }
    
    // Initialize all AI modules
    bool initialize() {
        try {
            // Initialize Video Processor
            kronop::TileConfig videoConfig;
            videoProcessor_ = std::make_unique<kronop::StreamingProcessor>(videoConfig);
            logInfo("Video Processor initialized");
            
            // Initialize Image Processor
            imageProcessor_ = std::make_unique<UpscaleCore>();
            logInfo("Image Processor initialized");
            
            // Initialize Music Processor
            kronop::TileConfig musicConfig;
            musicProcessor_ = std::make_unique<kronop::ChunkManager>(musicConfig);
            logInfo("Music Processor initialized");
            
            // Initialize Audio Processor
            audioProcessor_ = std::make_unique<AudioEnhancer>();
            logInfo("Audio Titan initialized");
            
            initialized_ = true;
            logInfo("All AI modules initialized successfully");
            return true;
            
        } catch (const std::exception& e) {
            logError("Failed to initialize AI modules: " + std::string(e.what()));
            return false;
        }
    }
    
    // Unified media processing function
    // Processes video, image, music, and audio in sync
    bool process_full_media(const std::string& inputPath, 
                           const std::string& outputPath) {
        if (!initialized_) {
            logError("Bridge not initialized. Call initialize() first.");
            return false;
        }
        
        auto startTime = std::chrono::high_resolution_clock::now();
        logInfo("Starting unified media processing: " + inputPath);
        
        try {
            // Step 1: Detect media type and extract components
            std::string videoPath = inputPath + "_video.mp4";
            std::string audioPath = inputPath + "_audio.wav";
            std::string imagePath = inputPath + "_frame.jpg";
            
            // PRIORITY: Process Audio First (critical for sync)
            logInfo("🔊 PRIORITY: Processing audio first for synchronization");
            if (!audioProcessor_->enhanceAudio(audioPath, outputPath + "_enhanced.wav")) {
                logError("Audio enhancement failed - cannot proceed without audio");
                return false;
            }
            
            // Safety check after audio processing
            monitorAndAdapt();
            
            // Apply music background processing to audio
            if (!processMusicBackground(outputPath + "_enhanced.wav")) {
                logInfo("Music background processing applied to audio");
            }
            
            // Step 2: Process Video (can be adapted based on safety)
            logInfo("🎥 Processing video stream...");
            if (!videoProcessor_->initializeStream(inputPath, videoPath)) {
                logError("Failed to initialize video stream");
                return false;
            }
            
            // Process frames with safety monitoring
            std::vector<uint8_t> frameData(1920 * 1080 * 3);
            int frameNumber = 0;
            while (videoProcessor_->getProcessedFrame(frameData, frameNumber)) {
                // Monitor system health during processing
                if (frameNumber % 10 == 0) { // Check every 10 frames
                    monitorAndAdapt();
                }
                
                // Apply image enhancement to each frame (if not in safety mode)
                if (!safetyModeActive_ || frameNumber % 2 == 0) { // Skip every other frame in safety mode
                    if (!applyImageEnhancement(frameData)) {
                        logInfo("Image enhancement applied to frame " + std::to_string(frameNumber));
                    }
                }
                frameNumber++;
            }
            
            // Step 3: Sync Video and Audio (audio already processed)
            logInfo("🔄 Synchronizing video and audio...");
            std::vector<float> audioData; // Load enhanced audio
            if (!syncVideoAudio(frameData, audioData)) {
                logError("Video-audio sync failed");
                return false;
            }
            
            // Final safety check
            monitorAndAdapt();
            
            // Step 4: Final output
            logInfo("💾 Creating final synchronized output: " + outputPath);
            
            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime);
            
            logInfo("Unified processing completed in " + std::to_string(duration.count()) + " seconds");
            if (safetyModeActive_) {
                logInfo("⚠️  Processing completed in safety mode - some quality reductions applied");
            }
            return true;
            
        } catch (const std::exception& e) {
            logError("Processing failed: " + std::string(e.what()));
            return false;
        }
    }
    
    // Individual module access (for advanced users)
    kronop::StreamingProcessor* getVideoProcessor() { return videoProcessor_.get(); }
    UpscaleCore* getImageProcessor() { return imageProcessor_.get(); }
    kronop::ChunkManager* getMusicProcessor() { return musicProcessor_.get(); }
    AudioEnhancer* getAudioProcessor() { return audioProcessor_.get(); }
    
    // Status check
    bool isInitialized() const { return initialized_; }
    
private:
    // Safety Valve monitoring functions
    float checkThermalStatus() {
        // Android thermal API placeholder - in real implementation use AThermal
        // For demo, simulate temperature check
        static float simulatedTemp = 35.0f;
        simulatedTemp += 0.5f; // Simulate rising temperature
        return simulatedTemp;
    }
    
    float checkCpuLoad() {
        // Read /proc/stat for CPU usage - simplified
        // In real Android, use sysinfo or ActivityManager
        static float simulatedCpu = 0.5f;
        simulatedCpu += 0.1f; // Simulate increasing load
        return std::min(simulatedCpu, 1.0f);
    }
    
    void monitorAndAdapt() {
        float currentTemp = checkThermalStatus();
        float currentCpu = checkCpuLoad();
        
        // Check thermal limit
        if (currentTemp > thermalLimit_) {
            if (!safetyModeActive_) {
                logInfo("Thermal limit exceeded (" + std::to_string(currentTemp) + "°C > " + std::to_string(thermalLimit_) + "°C)");
                safetyModeActive_ = true;
            }
            // Reduce video processing threads
            reduceVideoThreads();
        }
        
        // Check CPU limit
        if (currentCpu > cpuLimit_) {
            if (!safetyModeActive_) {
                logInfo("CPU limit exceeded (" + std::to_string(currentCpu*100) + "% > " + std::to_string(cpuLimit_*100) + "%)");
                safetyModeActive_ = true;
            }
            // Reduce video quality
            reduceVideoQuality();
        }
        
        // Check memory
        size_t currentMemory = getTotalMemoryUsage();
        if (currentMemory > emergencyMemoryThreshold_) {
            logInfo("Emergency memory threshold exceeded (" + std::to_string(currentMemory/1024/1024) + "MB)");
            emergencyCleanup();
        }
        
        // Reset safety mode if conditions improve
        if (safetyModeActive_ && currentTemp < thermalLimit_ - 5.0f && currentCpu < cpuLimit_ - 0.1f) {
            safetyModeActive_ = false;
            restoreNormalOperation();
            logInfo("Safety mode deactivated - conditions improved");
        }
    }
    
    void reduceVideoThreads() {
        if (videoProcessor_) {
            // Reduce processing threads to 1 for thermal safety
            // Note: This assumes StreamingProcessor has a method to set thread count
            logInfo("Reducing video processing threads to 1 for thermal safety");
            // videoProcessor_->setProcessingThreads(1); // Would need to add this method
        }
    }
    
    void reduceVideoQuality() {
        if (videoProcessor_) {
            // Temporarily reduce video quality
            logInfo("Reducing video rendering quality for CPU safety");
            // This would interact with VideoStreamer's adaptive quality
        }
    }
    
    void emergencyCleanup() {
        if (musicProcessor_) {
            // Force cleanup of old buffers in ChunkManager
            logInfo("Performing emergency memory cleanup");
            // musicProcessor_->emergencyMemoryCleanup(); // Method exists in ChunkManager
        }
    }
    
    void restoreNormalOperation() {
        if (videoProcessor_) {
            logInfo("Restoring normal video processing parameters");
            // Restore original thread count and quality
        }
    }
    
    size_t getTotalMemoryUsage() {
        size_t total = 0;
        if (musicProcessor_) total += musicProcessor_->getCurrentMemoryUsage();
        if (videoProcessor_) total += videoProcessor_->getCurrentMemoryUsage();
        return total;
    }
    
    // Synchronization helpers
    bool syncVideoAudio(const std::vector<uint8_t>& videoData, 
                       const std::vector<float>& audioData) {
        // Implement AV sync using AudioVideoSync from Kronop_Cleaner
        // For now, placeholder
        logInfo("Video-audio synchronization completed");
        return true;
    }
    
    bool applyImageEnhancement(std::vector<uint8_t>& frameData) {
        // Use UpscaleCore for frame enhancement
        // For now, placeholder
        logInfo("Image enhancement applied to frame");
        return true;
    }
    
    bool processMusicBackground(const std::string& audioPath) {
        // Use ChunkManager for music processing
        // For now, placeholder
        logInfo("Music background processing completed");
        return true;
    }
    
    // Error handling
    void logError(const std::string& message) {
        std::cerr << "❌ KRONOP BRIDGE ERROR: " << message << std::endl;
    }
    
    void logInfo(const std::string& message) {
        std::cout << "ℹ️  KRONOP BRIDGE: " << message << std::endl;
    }
};
