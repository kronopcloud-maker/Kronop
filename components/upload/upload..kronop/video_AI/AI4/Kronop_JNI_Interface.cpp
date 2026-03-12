#include <jni.h>
#include <string>
#include <android/log.h>
#include <memory>

// Include Kronop Master Bridge
#include "Kronop_Master_Bridge.hpp"

// JNI Logging
#define LOG_TAG "Kronop_JNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Global bridge instance
static std::unique_ptr<KronopMasterBridge> g_bridge;

/**
 * Kronop JNI Interface
 * JNI Bridge between Android Java and Kronop C++ AI System
 */

// Convert Java string to C++ string
std::string jstringToString(JNIEnv* env, jstring jstr) {
    if (!jstr) return "";
    const char* chars = env->GetStringUTFChars(jstr, nullptr);
    std::string result(chars);
    env->ReleaseStringUTFChars(jstr, chars);
    return result;
}

/**
 * Initialize Kronop AI System
 * Called when System.loadLibrary("kronop_titan") is executed
 */
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGI("Kronop JNI Interface loading...");

    // Initialize global bridge instance
    g_bridge = std::make_unique<KronopMasterBridge>();

    if (!g_bridge->initialize()) {
        LOGE("Failed to initialize Kronop Master Bridge");
        return JNI_ERR;
    }

    LOGI("Kronop AI System initialized successfully");
    return JNI_VERSION_1_6;
}

/**
 * Cleanup when library is unloaded
 */
JNIEXPORT void JNICALL JNI_OnUnload(JavaVM* vm, void* reserved) {
    LOGI("Kronop JNI Interface unloading...");

    // Cleanup bridge
    g_bridge.reset();

    LOGI("Kronop AI System cleanup completed");
}

/**
 * Process Media with Kronop AI
 * Java_com_kronop_ai_NativeEngine_processMedia
 *
 * @param inputPath Input media file path
 * @param outputPath Output media file path
 * @param enhancementLevel Enhancement level (0.0-1.0)
 * @return 0 on success, -1 on error
 */
JNIEXPORT jint JNICALL Java_com_kronop_ai_NativeEngine_processMedia(
    JNIEnv* env, jobject thiz,
    jstring inputPath, jstring outputPath, jfloat enhancementLevel) {

    LOGI("Processing media via JNI...");

    // Check if bridge is initialized
    if (!g_bridge || !g_bridge->isInitialized()) {
        LOGE("Kronop Master Bridge not initialized");
        return -1;
    }

    // Convert Java strings to C++ strings
    std::string cppInputPath = jstringToString(env, inputPath);
    std::string cppOutputPath = jstringToString(env, outputPath);

    LOGI("Input: %s", cppInputPath.c_str());
    LOGI("Output: %s", cppOutputPath.c_str());
    LOGI("Enhancement Level: %.2f", enhancementLevel);

    try {
        // Call Kronop Master Bridge process_full_media
        bool success = g_bridge->process_full_media(cppInputPath, cppOutputPath);

        if (success) {
            LOGI("Media processing completed successfully");
            return 0; // Success
        } else {
            LOGE("Media processing failed");
            return -1; // Error
        }

    } catch (const std::exception& e) {
        LOGE("Exception during processing: %s", e.what());
        return -2; // Exception
    } catch (...) {
        LOGE("Unknown exception during processing");
        return -3; // Unknown error
    }
}

/**
 * Get Kronop AI System Status
 * Java_com_kronop_ai_NativeEngine_getSystemStatus
 *
 * @return Status string describing system health
 */
JNIEXPORT jstring JNICALL Java_com_kronop_ai_NativeEngine_getSystemStatus(
    JNIEnv* env, jobject thiz) {

    std::string status = "Kronop AI System Status:\n";

    if (g_bridge && g_bridge->isInitialized()) {
        status += "✅ Master Bridge: Initialized\n";

        // Check individual components
        if (g_bridge->getVideoProcessor()) {
            status += "✅ Video Processor: Available\n";
        } else {
            status += "❌ Video Processor: Not Available\n";
        }

        if (g_bridge->getImageProcessor()) {
            status += "✅ Image Processor: Available\n";
        } else {
            status += "❌ Image Processor: Not Available\n";
        }

        if (g_bridge->getMusicProcessor()) {
            status += "✅ Music Processor: Available\n";
        } else {
            status += "❌ Music Processor: Not Available\n";
        }

        if (g_bridge->getAudioProcessor()) {
            status += "✅ Audio Titan: Available\n";
        } else {
            status += "❌ Audio Titan: Not Available\n";
        }

        status += "🎯 System Ready for Processing\n";

    } else {
        status += "❌ Master Bridge: Not Initialized\n";
        status += "🔄 Call System.loadLibrary(\"kronop_titan\") first\n";
    }

    return env->NewStringUTF(status.c_str());
}

/**
 * Set Enhancement Parameters
 * Java_com_kronop_ai_NativeEngine_setEnhancementParams
 *
 * @param videoQuality Video quality multiplier (0.1-2.0)
 * @param audioBoost Audio boost level (-20 to +20 dB)
 * @param thermalLimit Thermal safety limit (°C)
 * @return 0 on success
 */
JNIEXPORT jint JNICALL Java_com_kronop_ai_NativeEngine_setEnhancementParams(
    JNIEnv* env, jobject thiz,
    jfloat videoQuality, jfloat audioBoost, jfloat thermalLimit) {

    LOGI("Setting enhancement parameters: Video=%.2f, Audio=%.1f dB, Thermal=%.1f°C",
         videoQuality, audioBoost, thermalLimit);

    // Parameters would be applied to bridge if needed
    // For now, just log and return success

    return 0;
}

/**
 * Emergency Shutdown
 * Java_com_kronop_ai_NativeEngine_emergencyShutdown
 *
 * @return 0 on success
 */
JNIEXPORT jint JNICALL Java_com_kronop_ai_NativeEngine_emergencyShutdown(
    JNIEnv* env, jobject thiz) {

    LOGI("Emergency shutdown requested");

    // Cleanup bridge
    g_bridge.reset();

    LOGI("Emergency shutdown completed");
    return 0;
}
