#pragma once
#include <vector>
#include <string>
#include <memory>

class AudioWeightsManager {
private:
    std::vector<float> bassEnhancementWeights;    // 512 weights for low freq boost
    std::vector<float> vocalClarityWeights;       // 256 weights for mid freq enhancement
    std::vector<float> dynamicRangeWeights;       // 128 weights for compression/limiting

public:
    AudioWeightsManager() {
        // Pre-trained weights for 10-15% enhancement
        bassEnhancementWeights.resize(512, 1.1f);  // 10% boost default
        vocalClarityWeights.resize(256, 1.08f);    // 8% clarity boost
        dynamicRangeWeights.resize(128, 0.95f);    // 5% compression
    }

    const std::vector<float>& getBassWeights() const { return bassEnhancementWeights; }
    const std::vector<float>& getVocalWeights() const { return vocalClarityWeights; }
    const std::vector<float>& getDynamicWeights() const { return dynamicRangeWeights; }
};
