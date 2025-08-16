/* 
 * Stereo Delay
 * Features:
 * - Tone control using Low Pass Filter
 * - Ping Pong mode
 * - Tempo Sync
 * ported from https://github.com/jnonis/MiniDexed
 */

#pragma once

#include <cstddef>

#include "effect_lpf.h"

class AudioEffectDelay 
{
public:
    static constexpr float32_t MAX_DELAY_TIME = 1.0f;

    AudioEffectDelay(float32_t samplerate);
    ~AudioEffectDelay();

    void setTimeL(float32_t time) { timeL = constrain(time, 0.0f, MAX_DELAY_TIME); }
    void setTimeR(float32_t time) { timeR = constrain(time, 0.0f, MAX_DELAY_TIME); }
    void setFeedback(float32_t time) { feedback = constrain(time, 0.0f, MAX_DELAY_TIME); }
    void setTone(float32_t tone) { lpf.setCutoff(tone); }
    void setPingPongMode(bool mode) { pingPongMode = mode; }
    void setMix(float32_t mix);

    float32_t getTimeL() { return timeL; }
    float32_t getTimeR() { return timeR; }
    float32_t getFeedback() { return feedback; }
    float32_t getTone() { return lpf.getCutoff(); }
    bool getPingPongMode() { return pingPongMode; }
    float32_t getMix() { return mix; }

    void process(float32_t* blockL, float32_t* blockR, uint16_t len);

private:
    float32_t samplerate;

    size_t bufferSize;
    float32_t* bufferL;
    float32_t* bufferR;
    unsigned index;

    AudioEffectLPF lpf;

    float32_t timeL; // Left delay time in seconds
    float32_t timeR; // Right delay time in seconds
    float32_t feedback; // Feedback (0.0 - 1.0)
    bool pingPongMode;
    float32_t mix;
    float32_t dryMix;
    float32_t wetMix;
};
