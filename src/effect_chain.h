#pragma once

#include "effect_chorus.h"
#include "effect_delay.h"
#include "effect_platervbstereo.h"

class AudioFXChain
{
public:
        AudioFXChain(float samplerate):
        chorus{samplerate},
        delay{samplerate},
        reverb{samplerate},
        level{}
        {
        }

        float get_level() { return level; }
        void set_level(float value) { level = constrain(value, 0.0f, 1.0f); }

        void process (float *inputL, float *inputR, uint16_t len)
        {
                chorus.process(inputL, inputR, len);
                delay.process(inputL, inputR, len);
                reverb.doReverb(inputL, inputR, inputL, inputR, len);
                arm_scale_f32(inputL, level, inputL, len);
                arm_scale_f32(inputR, level, inputR, len);
        }

        void resetState()
        {
                delay.resetState();
                reverb.reset();
        }

        AudioEffectChorus chorus;
        AudioEffectDelay delay;
        AudioEffectPlateReverb reverb;

private:
        float level;
};
