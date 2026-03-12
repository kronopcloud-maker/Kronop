#include "Main_Video_Maker.hpp"
#include <iostream>

Main_Video_Maker::Main_Video_Maker()
    : ai1Receiver(), ai2Receiver(), ai3Receiver(), ai4Receiver(), ai5Receiver(),
      inputManager(ai1Receiver, ai2Receiver, ai3Receiver, ai4Receiver, ai5Receiver),
      nextExpectedIndex_(1), logFile_("Index_Log.txt"),
      tempFolder_("Kronop_Temp") {
    // Constructor - configure based on hardware
    AIMode mode = optimizer.selectAI();
    bool enableAI1 = true;
    bool enableAI2 = (mode == TWO_AI || mode == THREE_AI || mode == FOUR_AI || mode == FIVE_AI);
    bool enableAI3 = (mode == THREE_AI || mode == FOUR_AI || mode == FIVE_AI);
    bool enableAI4 = (mode == FOUR_AI || mode == FIVE_AI);
    bool enableAI5 = (mode == FIVE_AI);

    inputManager.setEnabledAIs(enableAI1, enableAI2, enableAI3, enableAI4, enableAI5);

    // Open log file
    if (!logFile_.is_open()) {
        std::cerr << "Warning: Could not open Index_Log.txt" << std::endl;
    }

    // Initialize cache monitor
    cacheMonitor_ = new Kronop_Cache_Monitor(tempFolder_, inputManager.getPauseSignal());

    std::cout << "Main_Video_Maker initialized. ";
    if (enableAI1) std::cout << "AI-1 enabled. ";
    if (enableAI2) std::cout << "AI-2 enabled. ";
    if (enableAI3) std::cout << "AI-3 enabled. ";
    if (enableAI4) std::cout << "AI-4 enabled. ";
    if (enableAI5) std::cout << "AI-5 enabled.";
    std::cout << std::endl;
}

Main_Video_Maker::~Main_Video_Maker() {
    // Destructor
}

void Main_Video_Maker::processVideos() {
    inputManager.startProcessing();
    cacheMonitor_->startMonitoring();

    // Continuously check for next expected video in cache folder and send to player
    while (true) {
        processHoldingArea();
        // Small delay to prevent busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void Main_Video_Maker::addVideo(const std::string& videoPath) {
    inputManager.enqueueVideo(videoPath);
    std::cout << "Video added to processing queue: " << videoPath << std::endl;
}

void Main_Video_Maker::processHoldingArea() {
    while (canSendVideo(nextExpectedIndex_)) {
        auto it = holdingArea_.find(nextExpectedIndex_);
        if (it != holdingArea_.end()) {
            sendToPlayer(it->second);
            holdingArea_.erase(it);
            nextExpectedIndex_++;
        }
    }
}

bool Main_Video_Maker::canSendVideo(int index) {
    return holdingArea_.find(index) != holdingArea_.end();
}

void Main_Video_Maker::sendToPlayer(const VideoAssignment& assignment) {
    // Convert to VideoFrame for Player
    VideoFrame frame;
    frame.timestamp = assignment.videoIndex * 33000; // Simulate timestamp
    frame.width = 1920;
    frame.height = 1080;
    frame.channels = 3;
    frame.data.assign(assignment.processedData.begin(), assignment.processedData.end());

    player.receiveVideoFrames({frame});

    // Log sending
    if (logFile_.is_open()) {
        logFile_ << "Sent video index " << assignment.videoIndex 
                << " to Player (was processed by AI-" << static_cast<int>(assignment.assignedAI) << ")" << std::endl;
    }

    std::cout << "Video index " << assignment.videoIndex << " sent to Player in order" << std::endl;
}
