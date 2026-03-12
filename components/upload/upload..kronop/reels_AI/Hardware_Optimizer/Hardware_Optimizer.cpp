#include "Hardware_Optimizer.hpp"

#ifdef _ANDROID_
#include <sys/sysinfo.h>
#include <fstream>
#include <string>
#endif

#ifdef TARGET_OS_IPHONE
#import <Foundation/Foundation.h>
#include <mach/host_info.h>
#include <mach/mach_host.h>
#endif

#include <iostream>
#include <thread>

ThermalState HardwareOptimizer::getThermalState() {
#ifdef _ANDROID_
    std::ifstream file("/sys/class/thermal/thermal_zone0/temp");
    if (file.is_open()) {
        int temp;
        file >> temp;
        file.close();
        if (temp > 80000) return CRITICAL;
        else if (temp > 60000) return SERIOUS;
        else if (temp > 30000) return FAIR;
        else return NORMAL;
    }
    return NORMAL;
#endif
#ifdef TARGET_OS_IPHONE
    NSProcessInfo *processInfo = [NSProcessInfo processInfo];
    NSProcessInfoThermalState thermalState = processInfo.thermalState;
    switch (thermalState) {
        case NSProcessInfoThermalStateNominal: return NORMAL;
        case NSProcessInfoThermalStateFair: return FAIR;
        case NSProcessInfoThermalStateSerious: return SERIOUS;
        case NSProcessInfoThermalStateCritical: return CRITICAL;
        default: return NORMAL;
    }
#endif
}

long long HardwareOptimizer::getAvailableMemory() {
#ifdef _ANDROID_
    struct sysinfo si;
    sysinfo(&si);
    return (long long)si.freeram * si.mem_unit;
#endif
#ifdef TARGET_OS_IPHONE
    vm_statistics_data_t vmStats;
    mach_msg_type_number_t infoCount = HOST_VM_INFO_COUNT;
    kern_return_t kernReturn = host_statistics(mach_host_self(), HOST_VM_INFO, (host_info_t)&vmStats, &infoCount);
    if (kernReturn != KERN_SUCCESS) return 0;
    return (long long)(vmStats.free_count + vmStats.inactive_count) * vm_page_size;
#endif
}

int HardwareOptimizer::getCPUCoreCount() {
#ifdef _ANDROID_
    return sysconf(_SC_NPROCESSORS_ONLN);
#endif
#ifdef TARGET_OS_IPHONE
    host_basic_info_data_t hostInfo;
    mach_msg_type_number_t infoCount = HOST_BASIC_INFO_COUNT;
    kern_return_t kernReturn = host_info(mach_host_self(), HOST_BASIC_INFO, (host_info_t)&hostInfo, &infoCount);
    if (kernReturn == KERN_SUCCESS) {
        return hostInfo.max_cpus;
    }
    return 1; // fallback
#endif
    return std::thread::hardware_concurrency();
}

bool HardwareOptimizer::shouldReduceTileSize() {
    ThermalState thermal = getThermalState();
    return thermal == FAIR || thermal == SERIOUS || thermal == CRITICAL;
}

int HardwareOptimizer::getRecommendedTileSize() {
    if (shouldReduceTileSize()) {
        return 256; // Reduce to 256x256 when temperature is high
    }
    return 512; // Default 512x512
}

void HardwareOptimizer::createAIThreads() {
    if (!shouldUseMultiThreading()) {
        std::cout << "Multi-threading not needed: CPU cores <= 8" << std::endl;
        return;
    }

    int coreCount = getCPUCoreCount();
    int threadCount = std::min(coreCount / 2, 5); // Max 5 threads for 5 AIs

    std::cout << "Creating " << threadCount << " AI threads for " << coreCount << " CPU cores" << std::endl;

    for (int i = 0; i < threadCount; ++i) {
        aiThreads_.emplace_back(&HardwareOptimizer::aiThreadWorker, this, i + 1);
    }
}

void HardwareOptimizer::joinAIThreads() {
    for (auto& thread : aiThreads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    aiThreads_.clear();
}

void HardwareOptimizer::aiThreadWorker(int aiId) {
    std::cout << "AI Thread " << aiId << " started on core " << std::this_thread::get_id() << std::endl;
    
    // Simulate AI processing work
    while (true) {
        // Thread-specific processing logic here
        // For now, just sleep to simulate work
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

AIMode HardwareOptimizer::selectAI() {
    long long mem = getAvailableMemory();
    ThermalState thermal = getThermalState();
    bool lowPower = false;
#ifdef TARGET_OS_IPHONE
    NSProcessInfo *processInfo = [NSProcessInfo processInfo];
    lowPower = processInfo.lowPowerModeEnabled;
#endif
    if (lowPower || thermal == CRITICAL || mem < 512LL * 1024 * 1024) { // less than 512MB
        return ONE_AI;
    } else if (thermal == SERIOUS || mem < 1024LL * 1024 * 1024) { // less than 1GB
        return TWO_AI;
    } else if (thermal == FAIR || mem < 2048LL * 1024 * 1024) { // less than 2GB
        return THREE_AI;
    } else if (thermal == NORMAL && mem < 4096LL * 1024 * 1024) { // less than 4GB
        return FOUR_AI;
    } else {
        return FIVE_AI; // Full capacity for 5 AIs
    }
}
