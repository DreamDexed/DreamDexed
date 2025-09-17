/* 
 * YK Chorus Port
 * Ported from https://github.com/SpotlightKid/ykchorus
 * Ported from https://github.com/jnonis/MiniDexed
 */

#pragma once

#include "ykchorus/ChorusEngine.h"

class AudioEffectChorus
{
public:
    AudioEffectChorus(float samplerate) : engine {samplerate}
    {
        setChorus1(true);
        setChorus2(true);
        setChorus1LFORate(0.5f);
        setChorus2LFORate(0.83f);
        setMix(0.0f);
    }

    bool getChorus1() { return engine.isChorus1Enabled; }
    bool getChorus2() { return engine.isChorus2Enabled; }

    void setChorus1(bool enable) { engine.setEnablesChorus(enable, engine.isChorus2Enabled); }
    void setChorus2(bool enable) { engine.setEnablesChorus(engine.isChorus1Enabled, enable); }

    float getChorus1Rate() { return engine.chorus1L->rate; }
    float getChorus2Rate() { return engine.chorus2L->rate; }

    void setChorus1LFORate(float32_t rate) { engine.setChorus1LfoRate(rate); }
    void setChorus2LFORate(float32_t rate) { engine.setChorus2LfoRate(rate); }

    float32_t getMix() { return mix; }

    void setMix(float32_t value)
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

    void process(float32_t* inblockL, float32_t* inblockR, uint16_t len)
    {
        if (wet == 0.0f) return;

        if (!engine.isChorus1Enabled && !engine.isChorus2Enabled) return;

        for (uint16_t i = 0; i < len; i++) 
        {
                engine.process(dry, wet, inblockL++, inblockR++);
        }
    }

private:
    ChorusEngine engine;

    float32_t mix, dry, wet;
};
