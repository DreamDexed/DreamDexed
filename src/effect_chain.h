#pragma once

#include "effect_dreamdelay.h"
#include "effect_platervbstereo.h"
#include "effect_ykchorus.h"

class AudioFXChain
{
public:
        AudioFXChain(float samplerate):
        yk_chorus{samplerate},
        dream_delay{samplerate},
        plate_reverb{samplerate},
        level{}
        {
        }

        float get_level() { return level; }
        void set_level(float value) { level = constrain(value, 0.0f, 1.0f); }

        void process (float *inputL, float *inputR, uint16_t len)
        {
                yk_chorus.process(inputL, inputR, len);
                dream_delay.process(inputL, inputR, len);
                plate_reverb.process(inputL, inputR, inputL, inputR, len);
                arm_scale_f32(inputL, level, inputL, len);
                arm_scale_f32(inputR, level, inputR, len);
        }

        void resetState()
        {
                dream_delay.resetState();
                plate_reverb.reset();
        }

        AudioEffectYKChorus yk_chorus;
        AudioEffectDreamDelay dream_delay;
        AudioEffectPlateReverb plate_reverb;

private:
        float level;
};
