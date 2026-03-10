#ifndef MAIN_VIDEO_MAKER_HPP
#define MAIN_VIDEO_MAKER_HPP

#include "Input_Gateway/AI1_Receiver.hpp"
#include "Input_Gateway/AI2_Receiver.hpp"
#include "Input_Gateway/AI3_Receiver.hpp"
#include "Input_Gateway/AI4_Receiver.hpp"
#include "Input_Gateway/AI5_Receiver.hpp"
#include "Player.hpp"
#include "../Hardware_Optimizer/Hardware_Optimizer.hpp"
#include "Input_Manager.hpp"
#include <string>
#include <map>
#include <fstream>
#include <filesystem>
#include "Kronop_Cache_Monitor.hpp"

namespace fs = std::filesystem;

class Main_Video_Maker {
public:
    Main_Video_Maker();
    ~Main_Video_Maker();

    // Process videos and send in order to Player
    void processVideos();
    void addVideo(const std::string& videoPath);

private:
    AI1_Receiver ai1Receiver;
    AI2_Receiver ai2Receiver;
    AI3_Receiver ai3Receiver;
    AI4_Receiver ai4Receiver;
    AI5_Receiver ai5Receiver;
    Player player;
    HardwareOptimizer optimizer;
    InputManager inputManager;

    // Video ordering system
    int nextExpectedIndex_;
    std::ofstream logFile_;

    Kronop_Cache_Monitor* cacheMonitor_;
    std::string tempFolder_;

    void processHoldingArea();
    bool canSendVideo(int index);
    void sendToPlayer(int index);
};

#endif // MAIN_VIDEO_MAKER_HPP
