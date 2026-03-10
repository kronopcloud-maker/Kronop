#ifndef HARDWARE_OPTIMIZER_HPP
#define HARDWARE_OPTIMIZER_HPP

#include <vector>
#include <thread>

enum ThermalState { NORMAL, FAIR, SERIOUS, CRITICAL };
enum AIMode { ONE_AI, TWO_AI, THREE_AI, FOUR_AI, FIVE_AI };

class HardwareOptimizer {
public:
    ThermalState getThermalState();
    long long getAvailableMemory();
    AIMode selectAI();
    int getCPUCoreCount();
    bool shouldUseMultiThreading();
    void createAIThreads();
    void joinAIThreads();
    int getRecommendedTileSize();
    bool shouldReduceTileSize();

private:
    std::vector<std::thread> aiThreads_;
    void aiThreadWorker(int aiId);
};

#endif // HARDWARE_OPTIMIZER_HPP
