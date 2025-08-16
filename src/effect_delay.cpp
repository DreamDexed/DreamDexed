/* 
 * Stereo Delay
 * Features:
 * - Tone control using Low Pass Filter
 * - Ping Pong mode
 * ported from https://github.com/jnonis/MiniDexed
 */

#include <cassert>
#include <arm_math.h>

#include "common.h"
#include "effect_delay.h"

AudioEffectDelay::AudioEffectDelay(float32_t samplerate):
samplerate{samplerate},
bufferSize{(size_t)(samplerate * MAX_DELAY_TIME)},
bufferL{new float32_t[bufferSize]{}},
bufferR{new float32_t[bufferSize]{}},
index{},
lpf{samplerate, 0.8f, 0.0f},
timeL{0.36f},
timeR{0.36f},
feedback{0.6f},
pingPongMode{}
{   
    assert(bufferL);
    assert(bufferR);

    setMix(0.0f);
}

AudioEffectDelay::~AudioEffectDelay()
{
    delete bufferL;
    delete bufferR;
}

void AudioEffectDelay::setMix(float32_t value)
{
    mix = value;
    if (mix <= 0.5f)
    {
        dryMix = 1.0f;
        wetMix = mix * 2;
    }
    else
    {
        dryMix = 1.0f - ((mix - 0.5f) * 2);
        wetMix = 1.0f;
    }
}

void AudioEffectDelay::process(float32_t* blockL, float32_t* blockR, uint16_t len)
{
    if (wetMix == 0.0) return;

    if (pingPongMode)
    {
        for (uint16_t i = 0; i < len; i++) 
        {
            float32_t inL = blockL[i];
            float32_t inR = blockR[i];

            // Ping Pong mode only uses timeL
            // Calculate offsets
            int offsetL = index - (timeL * samplerate);
            if (offsetL < 0) offsetL = bufferSize + offsetL;
            
            // We need to feed the buffers each other
            bufferL[index] = lpf.processSampleR(bufferR[offsetL]) * feedback;
            bufferR[index] = lpf.processSampleL(bufferL[offsetL]) * feedback;

            blockL[i] = (inL * dryMix) + (bufferL[index] * wetMix);
            blockR[i] = (inR * dryMix) + (bufferR[index] * wetMix);

            // Update buffers
            bufferL[index] += (inL + inR) * 0.5f;

            if (++index >= bufferSize) index = 0;
        }
    }
    else
    {
        for (uint16_t i = 0; i < len; i++) 
        {
            float32_t inL = blockL[i];
            float32_t inR = blockR[i];

            // Calculate offsets
            int offsetL = index - (timeL * samplerate);
            if (offsetL < 0) offsetL = bufferSize + offsetL;

            int offsetR = index - (timeR * samplerate);
            if (offsetR < 0) offsetR = bufferSize + offsetR;

            bufferL[index] = lpf.processSampleL(bufferL[offsetL]) * feedback;
            bufferR[index] = lpf.processSampleR(bufferR[offsetR]) * feedback;

            blockL[i] = (inL * dryMix) + (bufferL[index] * wetMix);
            blockR[i] = (inR * dryMix) + (bufferR[index] * wetMix);

            // Update buffers
            bufferL[index] += inL;
            bufferR[index] += inR;

            if (++index >= bufferSize) index = 0;
        }
    }
}
