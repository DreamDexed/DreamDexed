#pragma once

#include <cstddef>
#include <arm_math.h>
#include "common.h"
#include "effect_lpf.h"

class AudioEffectDreamDelay 
{
public:
    enum Mode { DUAL, CROSSOVER, PINGPONG, MODE_UNKNOWN };
    enum Sync { SYNC_NONE, T1_1, T1_1T, T1_2, T1_2T, T1_4, T1_4T, T1_8, T1_8T, T1_16, T1_16T, T1_32, T1_32T, SYNC_UNKNOWN };

    AudioEffectDreamDelay(float32_t samplerate);
    ~AudioEffectDreamDelay();

    void setMode(Mode m) { mode = m; }
    void setTimeL(float32_t time);
    void setTimeLSync(Sync sync);
    void setTimeR(float32_t time);
    void setTimeRSync(Sync sync);
    void setFeedback(float32_t fb) { feedback = constrain(fb, 0.0f, 1.0f); }
    void setHighCut(float32_t highcut) { lpf.setCutoff_Hz(highcut); }
    void setTempo(unsigned t);
    void setMix(float32_t mix);

    Mode getMode() { return mode; }
    float32_t getTimeL() { return timeL; }
    float32_t getTimeR() { return timeR; }
    float32_t getFeedback() { return feedback; }
    float32_t getHighCut() { return lpf.getCutoff_Hz(); }
    unsigned getTempo() { return tempo; }
    float32_t getMix() { return mix; }

    void process(float32_t* blockL, float32_t* blockR, uint16_t len);

    void resetState();

private:
    float32_t samplerate;

    Mode mode;

    size_t bufferSize;
    float32_t* bufferL;
    float32_t* bufferR;
    unsigned index;
    unsigned indexDL;
    unsigned indexDR;

    float32_t timeL; // Left delay time in seconds
    float32_t timeR; // Right delay time in seconds
    Sync timeLSync;
    Sync timeRSync;

    float32_t feedback; // 0.0 - 1.0

    AudioEffectLPF lpf;

    unsigned tempo;

    float32_t mix;
    float32_t dry;
    float32_t wet;
};
