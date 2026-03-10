#include "Kronop_Cache_Monitor.hpp"
#include <filesystem>
#include <iostream>
#include <chrono>

namespace fs = std::filesystem;

Kronop_Cache_Monitor::Kronop_Cache_Monitor(const std::string& tempFolder, std::atomic<bool>* pauseSignal)
    : tempFolder_(tempFolder), running_(false), pauseSignal_(pauseSignal) {
    // Create folder if not exists
    try {
        fs::create_directories(tempFolder_);
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error creating temp folder: " << e.what() << std::endl;
    }
}

Kronop_Cache_Monitor::~Kronop_Cache_Monitor() {
    stopMonitoring();
}

void Kronop_Cache_Monitor::startMonitoring() {
    if (running_) return;
    running_ = true;
    monitorThread_ = std::thread(&Kronop_Cache_Monitor::monitorLoop, this);
}

void Kronop_Cache_Monitor::stopMonitoring() {
    if (!running_) return;
    running_ = false;
    if (monitorThread_.joinable()) {
        monitorThread_.join();
    }
}

void Kronop_Cache_Monitor::monitorLoop() {
    while (running_) {
        int count = countVideosInFolder();
        if (count >= 5) {
            *pauseSignal_ = true;
            std::cout << "PAUSE_SIGNAL sent: Video count " << count << " >= 5, pausing AIs" << std::endl;
        } else if (count <= 4) {
            *pauseSignal_ = false;
            std::cout << "RESUME_SIGNAL sent: Video count " << count << " <= 4, resuming AIs" << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

int Kronop_Cache_Monitor::countVideosInFolder() {
    int count = 0;
    try {
        for (const auto& entry : fs::directory_iterator(tempFolder_)) {
            if (entry.is_regular_file() && entry.path().extension() == ".mp4") {
                count++;
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error scanning folder: " << e.what() << std::endl;
    }
    return count;
}
