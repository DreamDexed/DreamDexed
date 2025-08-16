#include "effect_dreamdelay.h"

#include <cassert>

constexpr float32_t MAX_DELAY_TIME = 2.0f;

AudioEffectDreamDelay::AudioEffectDreamDelay(float32_t samplerate):
samplerate{samplerate},
mode{DUAL},
bufferSize{(size_t)(samplerate * MAX_DELAY_TIME)},
bufferL{new float32_t[bufferSize]{}},
bufferR{new float32_t[bufferSize]{}},
index{},
timeLSync{},
timeRSync{},
feedback{0.6f},
lpf{samplerate, 6300.0f, 0.0f},
tempo{120}
{   
    assert(bufferL);
    assert(bufferR);

    setTimeL(0.36f);
    setTimeR(0.36f);
    setMix(0.0f);
}

AudioEffectDreamDelay::~AudioEffectDreamDelay()
{
    delete bufferL;
    delete bufferR;
}

static float32_t calculateTime(AudioEffectDreamDelay::Sync sync, unsigned tempo)
{
    constexpr float32_t triplet = 2.0 / 3.0;
    static constexpr float32_t denominators[] = {1, 1, 1, 2, 2, 4, 4, 8, 8, 16, 16, 32, 32, 1};
    return 240.0 / tempo / denominators[(int)sync] * ((int)sync % 2 ? 1 : triplet);
}

void AudioEffectDreamDelay::setTimeL(float32_t time)
{
    timeL = constrain(time, 0.0f, MAX_DELAY_TIME);
    int offset = index - timeL * samplerate - (timeL ? 0 : 1);
    if (offset < 0) offset += bufferSize;
    indexDL = offset;
}

void AudioEffectDreamDelay::setTimeR(float32_t time)
{
    timeR = constrain(time, 0.0f, MAX_DELAY_TIME);
    int offset = index - timeR * samplerate - (timeR ? 0 : 1);
    if (offset < 0) offset += bufferSize;
    indexDR = offset;
}

void AudioEffectDreamDelay::setTimeLSync(Sync sync)
{
    timeLSync = sync;
    if (sync != SYNC_NONE)
        setTimeL(calculateTime(sync, tempo));
}

void AudioEffectDreamDelay::setTimeRSync(Sync sync)
{
    timeRSync = sync;
    if (sync != SYNC_NONE)
        setTimeR(calculateTime(sync, tempo));
}

void AudioEffectDreamDelay::setTempo(unsigned t)
{
    tempo = constrain(t, 30u, 240u);
    setTimeLSync(timeLSync);
    setTimeRSync(timeRSync);
}

void AudioEffectDreamDelay::setMix(float32_t value)
{
    mix = constrain(value, 0.0f, 1.0f);

    if (mix <= 0.5f)
    {
        dry = 1.0f;
        wet = mix * 2.0f;
    }
    else
    {
        dry = 1.0f - ((mix - 0.5f) * 2.0f);
        wet = 1.0f;
    }
}

void AudioEffectDreamDelay::process(float32_t* blockL, float32_t* blockR, uint16_t len)
{
    if (wet == 0.0f) return;

    switch(mode)
    {
    case DUAL:
        for (uint16_t i = 0; i < len; i++) 
        {
            float32_t inL = blockL[i];
            float32_t inR = blockR[i];

            float32_t delayL = bufferL[indexDL];
            float32_t delayR = bufferR[indexDR];

            bufferL[index] = lpf.processSampleL(inL) + delayL * feedback;
            bufferR[index] = lpf.processSampleR(inR) + delayR * feedback;

            blockL[i] = inL * dry + delayL * wet;
            blockR[i] = inR * dry + delayR * wet;

            indexDL = (indexDL + 1) % bufferSize;
            indexDR = (indexDR + 1) % bufferSize;
            index = (index + 1) % bufferSize;
        }
        break;

    case CROSSOVER:
        for (uint16_t i = 0; i < len; i++) 
        {
            float32_t inL = blockL[i];
            float32_t inR = blockR[i];

            float32_t delayL = bufferL[indexDL];
            float32_t delayR = bufferR[indexDR];

            bufferL[index] = lpf.processSampleL(inL) + delayR * feedback;
            bufferR[index] = lpf.processSampleR(inR) + delayL * feedback;

            blockL[i] = inL * dry + delayL * wet;
            blockR[i] = inR * dry + delayR * wet;

            indexDL = (indexDL + 1) % bufferSize;
            indexDR = (indexDR + 1) % bufferSize;
            index = (index + 1) % bufferSize;
        }
        break;

    case PINGPONG:
        for (uint16_t i = 0; i < len; i++) 
        {
            float32_t in = (blockL[i] + blockR[i]) / 2;

            float32_t delayL = bufferL[indexDL];
            float32_t delayR = bufferR[indexDR];

            bufferL[index] = lpf.processSampleL(in) + delayR * feedback;
            bufferR[index] = delayL;
            
            blockL[i] = in * dry + delayL * wet;
            blockR[i] = in * dry + delayR * wet;

            indexDL = (indexDL + 1) % bufferSize;
            indexDR = (indexDR + 1) % bufferSize;
            index = (index + 1) % bufferSize;
        }
        break;

    default:
        assert(0);
        break;
    }
}

void AudioEffectDreamDelay::resetState()
{
    memset(bufferL, 0, bufferSize * sizeof *bufferL);
    memset(bufferR, 0, bufferSize * sizeof *bufferR);

    lpf.resetState();
}
