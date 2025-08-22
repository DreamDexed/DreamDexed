/* 
 * DISTHRO 3 Band EQ
 * Ported from https://github.com/DISTRHO/Mini-Series/blob/master/plugins/3BandEQ
 * Ported from https://github.com/jnonis/MiniDexed
 */

#pragma once

#include <cmath>

#include "midi.h"

class AudioEffect3BandEQ
{
public:
    AudioEffect3BandEQ(unsigned samplerate):
    samplerate{samplerate},
    fLow{}, fMid{}, fHigh{}, fGain{}, fLowMidFreq{}, fMidHighFreq{},
    nLowMidFreq{24}, /* 315Hz */
    nMidHighFreq{44}, /* 3.2kHz */
    tmpLP{}, tmpHP{}
    {
        setLow_dB(fLow);
        setMid_dB(fMid);
        setHigh_dB(fHigh);
        setGain_dB(fGain);
        setLowMidFreq_n(nLowMidFreq);
        setMidHighFreq_n(nMidHighFreq);
    }

    void setLow_dB(float value)
    {
        fLow = value;
        lowVol = std::pow(10.0f, fLow / 20.0f);
    }

    void setMid_dB(float value)
    {
        fMid = value;
        midVol = std::pow(10.0f, fMid / 20.0f);
    }

    void setHigh_dB(float value)
    {
        fHigh = value;
        highVol = std::pow(10.0f, fHigh / 20.0f);
    }

    void setGain_dB(float value)
    {
        fGain = value;
        outVol = std::pow(10.0f, fGain / 20.0f);
    }

    float setLowMidFreq(float value)
    {
        fLowMidFreq = std::min(value, fMidHighFreq);
        xLP  = std::exp(-2.0f * M_PI * fLowMidFreq / samplerate);
        a0LP = 1.0f - xLP;
        b1LP = -xLP;
        return fLowMidFreq;
    }

    float setMidHighFreq(float value)
    {
        fMidHighFreq = std::max(value, fLowMidFreq);
        xHP  = std::exp(-2.0f * M_PI * fMidHighFreq / samplerate);
        a0HP = 1.0f - xHP;
        b1HP = -xHP;
        return fMidHighFreq;
    }

    unsigned setLowMidFreq_n(unsigned value)
    {
        nLowMidFreq = std::min(value, nMidHighFreq);
        setLowMidFreq(MIDI_EQ_HZ[nLowMidFreq]);
        return nLowMidFreq;
    }

    unsigned setMidHighFreq_n(unsigned value)
    {
        nMidHighFreq = std::max(value, nLowMidFreq);
        setMidHighFreq(MIDI_EQ_HZ[nMidHighFreq]);
        return nMidHighFreq;
    }

    float getLow_dB() const { return fLow; }
    float getMid_dB() const { return fMid; }
    float getHigh_dB() const { return fHigh; }
    float getGain_dB() const { return fGain; }
    float getLowMidFreq() const { return fLowMidFreq; }
    float getMidHighFreq() const { return fMidHighFreq; }
    unsigned getLowMidFreq_n() const { return nLowMidFreq; }
    unsigned getMidHighFreq_n() const { return nMidHighFreq; }

    void process(float32_t* block, uint16_t len)
    {
        float outLP, outHP;

        if (!fLow && !fMid && !fHigh && !fGain) return;

        for (uint16_t i=0; i < len; ++i)
        {
            float inValue = std::isnan(block[i]) ? 0.0f : block[i];

            tmpLP = a0LP * inValue - b1LP * tmpLP;
            outLP = tmpLP;

            tmpHP = a0HP * inValue - b1HP * tmpHP;
            outHP = inValue - tmpHP;

            block[i] = (outLP*lowVol + (inValue - outLP - outHP)*midVol + outHP*highVol) * outVol;
        }
    }

private:
    unsigned samplerate;

    float fLow, fMid, fHigh, fGain, fLowMidFreq, fMidHighFreq;
    unsigned nLowMidFreq, nMidHighFreq;

    float lowVol, midVol, highVol, outVol;

    float xLP, a0LP, b1LP;
    float xHP, a0HP, b1HP;

    float tmpLP, tmpHP;
};
