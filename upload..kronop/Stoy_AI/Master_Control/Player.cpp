#include "Player.hpp"
#include <iostream>

Player::Player() {
    // Constructor
}

Player::~Player() {
    // Destructor
}

void Player::receiveVideoFrames(const std::vector<VideoFrame>& frames) {
    std::cout << "Player received " << frames.size() << " processed video frames" << std::endl;
    
    for (size_t i = 0; i < frames.size(); ++i) {
        const auto& frame = frames[i];
        std::cout << "Playing frame " << i << " - Timestamp: " << frame.timestamp 
                  << ", Size: " << frame.width << "x" << frame.height 
                  << ", Channels: " << frame.channels 
                  << ", Data size: " << frame.data.size() << " bytes" << std::endl;
    }
    
    std::cout << "Video playback completed successfully!" << std::endl;
}
