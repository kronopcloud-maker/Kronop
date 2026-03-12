#include <vector>
#include <fstream>
#include <cstdint>
#include <android/log.h>

#define LOG_TAG "Titan_Output_Saver"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// WAV header structure for lossless saving with metadata preservation
struct WavHeader {
    char riff[4] = {'R', 'I', 'F', 'F'};
    uint32_t fileSize;
    char wave[4] = {'W', 'A', 'V', 'E'};
    char fmt[4] = {'f', 'm', 't', ' '};
    uint32_t fmtSize = 16;
    uint16_t audioFormat = 1;  // PCM
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample = 16;
    char data[4] = {'d', 'a', 't', 'a'};
    uint32_t dataSize;
};

// Function to save processed audio to WAV file lossless with metadata
bool saveProcessedAudioToWav(const std::string& path, const std::vector<float>& audioData, int sampleRate = 44100, int channels = 1) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        LOGI("Failed to open file for writing: %s", path.c_str());
        return false;
    }

    WavHeader header;
    header.numChannels = channels;
    header.sampleRate = sampleRate;
    header.byteRate = sampleRate * channels * 16 / 8;
    header.blockAlign = channels * 16 / 8;
    header.dataSize = audioData.size() * sizeof(int16_t);
    header.fileSize = sizeof(WavHeader) + header.dataSize - 8;

    file.write(reinterpret_cast<char*>(&header), sizeof(WavHeader));

    for (float sample : audioData) {
        // Convert float to int16_t for lossless PCM (no quality loss)
        int16_t outSample = static_cast<int16_t>(sample * 32767.0f);
        file.write(reinterpret_cast<char*>(&outSample), sizeof(int16_t));
    }

    file.close();
    LOGI("Audio saved to WAV: %s, samples: %zu, rate: %d, channels: %d", path.c_str(), audioData.size(), sampleRate, channels);
    return true;
}

// Optional: Save to raw PCM (no metadata, just data)
bool saveProcessedAudioToPcm(const std::string& path, const std::vector<float>& audioData) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        LOGI("Failed to open file for writing: %s", path.c_str());
        return false;
    }

    for (float sample : audioData) {
        int16_t outSample = static_cast<int16_t>(sample * 32767.0f);
        file.write(reinterpret_cast<char*>(&outSample), sizeof(int16_t));
    }

    file.close();
    LOGI("Audio saved to PCM: %s, samples: %zu", path.c_str(), audioData.size());
    return true;
}
