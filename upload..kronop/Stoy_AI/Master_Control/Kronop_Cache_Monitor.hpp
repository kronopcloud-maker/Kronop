#ifndef KRONOP_CACHE_MONITOR_HPP
#define KRONOP_CACHE_MONITOR_HPP

#include <string>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>

class Kronop_Cache_Monitor {
public:
    Kronop_Cache_Monitor(const std::string& tempFolder, std::atomic<bool>* pauseSignal);
    ~Kronop_Cache_Monitor();

    void startMonitoring();
    void stopMonitoring();

private:
    void monitorLoop();
    int countVideosInFolder();

    std::string tempFolder_;
    std::thread monitorThread_;
    std::atomic<bool> running_;
    std::atomic<bool>* pauseSignal_;
};

#endif // KRONOP_CACHE_MONITOR_HPP
