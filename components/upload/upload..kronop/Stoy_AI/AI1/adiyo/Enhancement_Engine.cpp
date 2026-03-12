#include <vector>
#include <complex>
#include <cmath>
#include <arm_neon.h>
#include <android/log.h>

#define LOG_TAG "Enhancement_Engine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

using namespace std;

// Lightweight FFT using Cooley-Tukey algorithm for time-to-frequency conversion
void fft(vector<complex<float>>& data, bool inverse = false) {
    int n = data.size();
    // Bit reversal permutation
    for (int i = 1, j = 0; i < n; ++i) {
        int bit = n >> 1;
        for (; j >= bit; bit >>= 1) j -= bit;
        j += bit;
        if (i < j) swap(data[i], data[j]);
    }
    // Cooley-Tukey FFT
    for (int len = 2; len <= n; len <<= 1) {
        float ang = 2 * M_PI / len * (inverse ? -1 : 1);
        complex<float> wlen(cos(ang), sin(ang));
        for (int i = 0; i < n; i += len) {
            complex<float> w(1, 0);
            for (int j = 0; j < len / 2; ++j) {
                complex<float> u = data[i + j];
                complex<float> v = data[i + j + len / 2] * w;
                data[i + j] = u + v;
                data[i + j + len / 2] = u - v;
                w *= wlen;
            }
        }
    }
    if (inverse) {
        for (auto& c : data) c /= n;
    }
}

// Apply 15% quality boost: 12-15% boost for low freq (20-250Hz), sharpen mids (300-4000Hz)
void applyQualityBoost(vector<complex<float>>& freqData, int sampleRate) {
    int n = freqData.size();
    float lowBoost = 1.135f;  // Average 13.5% boost
    int lowStart = (int)(20.0f * n / sampleRate);
    int lowEnd = (int)(250.0f * n / sampleRate);
    for (int i = lowStart; i <= lowEnd && i < n / 2; ++i) {
        freqData[i] *= lowBoost;
        freqData[n - i] *= lowBoost;
    }
    float midBoost = 1.1f;  // 10% sharpen for mids
    int midStart = (int)(300.0f * n / sampleRate);
    int midEnd = (int)(4000.0f * n / sampleRate);
    for (int i = midStart; i <= midEnd && i < n / 2; ++i) {
        freqData[i] *= midBoost;
        freqData[n - i] *= midBoost;
    }
}

// Soft Limiter to prevent clipping using NEON SIMD
void applySoftLimiter(vector<float>& audioData, float threshold = 0.9f, float ratio = 4.0f) {
    int n = audioData.size();
    float32x4_t thresh_vec = vdupq_n_f32(threshold);
    float32x4_t one_vec = vdupq_n_f32(1.0f);
    float32x4_t ratio_inv = vdupq_n_f32(1.0f / ratio);
    for (int i = 0; i < n; i += 4) {
        float32x4_t data = vld1q_f32(&audioData[i]);
        float32x4_t abs_data = vabsq_f32(data);
        uint32x4_t over = vcgtq_f32(abs_data, thresh_vec);
        float32x4_t excess = vsubq_f32(abs_data, thresh_vec);
        excess = vmulq_f32(excess, vsubq_f32(one_vec, ratio_inv));
        float32x4_t compressed = vaddq_f32(thresh_vec, excess);
        uint32x4_t neg = vcltq_f32(data, vdupq_n_f32(0.0f));
        compressed = vbslq_f32(neg, vnegq_f32(compressed), compressed);
        data = vbslq_f32(over, compressed, data);
        vst1q_f32(&audioData[i], data);
    }
}

// Main enhancement function
void enhanceAudio(vector<float>& audioChunk, int sampleRate) {
    int n = audioChunk.size();
    // Assume n is power of 2 for FFT
    vector<complex<float>> freqData(n);
    for (int i = 0; i < n; ++i) {
        freqData[i] = complex<float>(audioChunk[i], 0.0f);
    }
    fft(freqData, false);
    applyQualityBoost(freqData, sampleRate);
    fft(freqData, true);
    for (int i = 0; i < n; ++i) {
        audioChunk[i] = freqData[i].real();
    }
    applySoftLimiter(audioChunk);
    LOGI("Audio chunk enhanced: %d samples at %d Hz", n, sampleRate);
}
