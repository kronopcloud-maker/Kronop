#ifndef INPUT_MANAGER_HPP
#define INPUT_MANAGER_HPP

#include <string>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include "AI1_Receiver.hpp"
#include "AI2_Receiver.hpp"
#include "AI3_Receiver.hpp"
#include "AI4_Receiver.hpp"
#include "AI5_Receiver.hpp"

enum AIUnit { AI_NONE, AI_1, AI_2, AI_3, AI_4, AI_5 };

struct VideoAssignment {
    std::string videoId;
    AIUnit assignedAI;
    std::string processedData;
    int videoIndex; // Unique index for ordering
};

class InputManager {
public:
    InputManager(AI1_Receiver& ai1, AI2_Receiver& ai2, AI3_Receiver& ai3, AI4_Receiver& ai4, AI5_Receiver& ai5);
    ~InputManager();

    void startProcessing();
    void stopProcessing();
    void enqueueVideo(const std::string& videoPath);
    void setEnabledAIs(bool enableAI1, bool enableAI2, bool enableAI3, bool enableAI4, bool enableAI5);
    std::atomic<bool>* getPauseSignal() { return &pauseSignal_; }

private:
    void processingThread();
    void preFetchVideos();
    AIUnit getNextAI();
    void assignVideoToAI(int index, const std::string& videoPath, AIUnit ai);
    int countVideosInFolder(const std::string& folder);

    AI1_Receiver& ai1Receiver_;
    AI2_Receiver& ai2Receiver_;
    AI3_Receiver& ai3Receiver_;
    AI4_Receiver& ai4Receiver_;
    AI5_Receiver& ai5Receiver_;

    std::queue<std::pair<int, std::string>> videoQueue_; // index, path
    std::mutex queueMutex_;
    std::condition_variable queueCV_;
    std::thread processingThread_;
    std::atomic<bool> running_;

    // Round robin counter
    int currentAICounter_;
    bool ai1Enabled_;
    bool ai2Enabled_;
    bool ai3Enabled_;
    bool ai4Enabled_;
    bool ai5Enabled_;

    // Video indexing
    std::atomic<int> nextVideoIndex_; // Unique index for each video

    // Pause signal for cache management
    std::atomic<bool> pauseSignal_;
    std::condition_variable pauseCV_;
    std::mutex pauseMutex_;

    // Temp folder for hard limit
    std::string tempFolder_;
};

#endif // INPUT_MANAGER_HPP
