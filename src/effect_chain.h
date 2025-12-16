#pragma once

#include <functional>

#include "zyn/APhaser.h"
#include "zyn/Phaser.h"

#include "effect_3bandeq.h"
#include "effect_compressor.h"
#include "effect_cloudseed2.h"
#include "effect_dreamdelay.h"
#include "effect_platervbstereo.h"
#include "effect_ykchorus.h"

class AudioFXChain
{
public:
        typedef std::function<void(float *inputL, float *inputR, uint16_t len)> process_t;

        AudioFXChain(float samplerate):
        yk_chorus{samplerate},
        zyn_aphaser{samplerate},
        zyn_phaser{samplerate},
        dream_delay{samplerate},
        plate_reverb{samplerate},
        cloudseed2{samplerate},
        compressor{samplerate},
        eq{samplerate},
        bypass{},
        funcs{
                [this](float *inputL, float *inputR, uint16_t len) {},
                [this](float *inputL, float *inputR, uint16_t len) { yk_chorus.process(inputL, inputR, len); },
                [this](float *inputL, float *inputR, uint16_t len) { zyn_aphaser.process(inputL, inputR, len); },
                [this](float *inputL, float *inputR, uint16_t len) { zyn_phaser.process(inputL, inputR, len); },
                [this](float *inputL, float *inputR, uint16_t len) { dream_delay.process(inputL, inputR, len); },
                [this](float *inputL, float *inputR, uint16_t len) { plate_reverb.process(inputL, inputR, inputL, inputR, len); },
                [this](float *inputL, float *inputR, uint16_t len) { cloudseed2.process(inputL, inputR, len); },
                [this](float *inputL, float *inputR, uint16_t len) { compressor.process(inputL, inputR, len); },
                [this](float *inputL, float *inputR, uint16_t len) { eq.process(inputL, inputR, len); },
        },
        level{}
        {
        }

        float get_level() { return level; }
        void set_level(float value) { level = constrain(value, 0.0f, 1.0f); }

        void process (float *inputL, float *inputR, uint16_t len)
        {
                if (bypass) return;

                for (int i = 0; i < FX::slots_num; ++i)
                        if (uint8_t id = slots[i])
                                funcs[id](inputL, inputR, len);

                if (level != 1.0f)
                {
                        arm_scale_f32(inputL, level, inputL, len);
                        arm_scale_f32(inputR, level, inputR, len);
                }
        }

        void resetState()
        {
                zyn_aphaser.cleanup();
                zyn_phaser.cleanup();
                dream_delay.resetState();
                plate_reverb.reset();
                compressor.resetState();
                eq.resetState();

                cloudseed2.setRampedDown();
                cloudseed2.setNeedBufferClear();
        }

        void setSlot(uint8_t slot, uint8_t effect_id)
        {
                assert(slot < FX::slots_num);
                assert(effect_id < FX::effects_num);

                slots[slot] = effect_id;
        }

        AudioEffectYKChorus yk_chorus;
        zyn::APhaser zyn_aphaser;
        zyn::Phaser zyn_phaser;
        AudioEffectDreamDelay dream_delay;
        AudioEffectPlateReverb plate_reverb;
        AudioEffectCloudSeed2 cloudseed2;
        AudioEffectCompressor compressor;
        AudioEffect3BandEQ eq;

        std::atomic<bool> bypass;

private:
        std::atomic<uint8_t> slots[FX::slots_num];
        const process_t funcs[FX::effects_num];

        float level;
};
