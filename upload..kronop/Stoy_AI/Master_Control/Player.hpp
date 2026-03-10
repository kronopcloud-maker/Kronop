#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <string>
#include <vector>
#include <cstdint>

struct VideoFrame {
    int64_t timestamp;
    std::vector<uint8_t> data;
    int width;
    int height;
    int channels;
};

class Player {
public:
    Player();
    ~Player();

    // Receive final assembled video frames
    void receiveVideoFrames(const std::vector<VideoFrame>& frames);
};

#endif // PLAYER_HPP
