/* 
 * DISTHRO 3 Band EQ
 * Ported from https://github.com/DISTRHO/Mini-Series/blob/master/plugins/3BandEQ
 * Ported from https://github.com/jnonis/MiniDexed
 */

#pragma once

#include <math.h>

class AudioEffect3BandEQ
{
public:
    static constexpr float kDC_ADD = 1e-30f; // Why do we need to add a small DC?

    AudioEffect3BandEQ(unsigned samplerate):
    samplerate{samplerate},
    fLow{}, fMid{}, fHigh{}, fGain{}, fLowMidFreq{320}, fMidHighFreq{3100},
    tmp1LP{}, tmp2LP{}, tmp1HP{}, tmp2HP{}
    {
        setLow_dB(fLow);
        setMid_dB(fMid);
        setHigh_dB(fHigh);
        setGain_dB(fGain);
        setLowMidFreq(fLowMidFreq);
        setMidHighFreq(fMidHighFreq);
    }

    void setLow_dB(float value)
    {
        fLow = value;
        lowVol = pow(10.0f, fLow / 20.0f);
    }

    void setMid_dB(float value)
    {
        fMid = value;
        midVol = pow(10.0f, fMid / 20.0f);
    }

    void setHigh_dB(float value)
    {
        fHigh = value;
        highVol = pow(10.0f, fHigh / 20.0f);
    }

    void setGain_dB(float value)
    {
        fGain = value;
        outVol = pow(10.0f, fGain / 20.0f);
    }

    void setLowMidFreq(float value)
    {
        fLowMidFreq = std::min(value, fMidHighFreq);
        xLP  = exp(-2.0f * M_PI * fLowMidFreq / samplerate);
        a0LP = 1.0f - xLP;
        b1LP = -xLP;
    }

    void setMidHighFreq(float value)
    {
        fMidHighFreq = std::max(value, fLowMidFreq);
        xHP  = exp(-2.0f * M_PI * fMidHighFreq / samplerate);
        a0HP = 1.0f - xHP;
        b1HP = -xHP;
    }

    float getLow_dB() const { return fLow; }
    float getMid_dB() const { return fMid; }
    float getHigh_dB() const { return fHigh; }
    float getGain_dB() const { return fGain; }
    float getLowMidFreq() const { return fLowMidFreq; }
    float getMidHighFreq() const { return fMidHighFreq; }

    void process(float32_t* blockL, float32_t* blockR, uint16_t len)
    {
        float out1LP, out2LP, out1HP, out2HP;

        if (!fLow && !fMid && !fHigh && !fGain) return;

        for (uint32_t i=0; i < len; ++i)
        {
            float inValue1 = isnan(blockL[i]) ? 0.0f : blockL[i];
            float inValue2 = isnan(blockR[i]) ? 0.0f : blockR[i];

            tmp1LP = a0LP * inValue1 - b1LP * tmp1LP + kDC_ADD;
            tmp2LP = a0LP * inValue2 - b1LP * tmp2LP + kDC_ADD;
            out1LP = tmp1LP - kDC_ADD;
            out2LP = tmp2LP - kDC_ADD;

            tmp1HP = a0HP * inValue1 - b1HP * tmp1HP + kDC_ADD;
            tmp2HP = a0HP * inValue2 - b1HP * tmp2HP + kDC_ADD;
            out1HP = inValue1 - tmp1HP - kDC_ADD;
            out2HP = inValue2 - tmp2HP - kDC_ADD;

            blockL[i] = (out1LP*lowVol + (inValue1 - out1LP - out1HP)*midVol + out1HP*highVol) * outVol;
            blockR[i] = (out2LP*lowVol + (inValue2 - out2LP - out2HP)*midVol + out2HP*highVol) * outVol;
        }
    }

private:
    unsigned samplerate;

    float fLow, fMid, fHigh, fGain, fLowMidFreq, fMidHighFreq;

    float lowVol, midVol, highVol, outVol;

    float xLP, a0LP, b1LP;
    float xHP, a0HP, b1HP;

    float tmp1LP, tmp2LP, tmp1HP, tmp2HP;
};
