#include <sched.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <android/log.h>
#include <NeuralNetworks.h>

#define LOG_TAG "Hardware_Initialize"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// Detect mobile's Performance Cores (Big Cores) and bind audio thread to them
bool detectAndBindBigCoresForAudio() {
    int totalCores = sysconf(_SC_NPROCESSORS_ONLN);
    int bigCoreStart = totalCores / 2;  // First half little, second half big

    cpu_set_t cpuSet;
    CPU_ZERO(&cpuSet);

    for (int i = bigCoreStart; i < totalCores; ++i) {
        CPU_SET(i, &cpuSet);
    }

    if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuSet) == 0) {
        LOGI("Audio thread bound to big cores");
        return true;
    }
    return false;
}

// Access mobile's NPU for AI calculations
bool accessNPUForAI() {
    ANeuralNetworksModel* model;
    ANeuralNetworksCompilation* compilation;

    ANeuralNetworksModel_create(&model);

    uint32_t dims[] = {1, 1024};
    ANeuralNetworksOperandType type = {
        .type = ANEURALNETWORKS_TENSOR_FLOAT32,
        .dimensionCount = 2,
        .dimensions = dims
    };

    ANeuralNetworksModel_addOperand(model, &type);
    ANeuralNetworksModel_addOperand(model, &type);

    uint32_t inputs[] = {0};
    uint32_t outputs[] = {1};
    ANeuralNetworksModel_addOperation(model, ANEURALNETWORKS_ADD, 1, inputs, 1, outputs);
    ANeuralNetworksModel_identifyInputsAndOutputs(model, 1, inputs, 1, outputs);
    ANeuralNetworksModel_finish(model);

    ANeuralNetworksCompilation_create(model, &compilation);
    ANeuralNetworksCompilation_setPreference(compilation, ANEURALNETWORKS_PREFER_LOW_POWER);
    ANeuralNetworksCompilation_finish(compilation);

    LOGI("NPU accessed for AI calculations");

    ANeuralNetworksCompilation_free(compilation);
    ANeuralNetworksModel_free(model);

    return true;
}
