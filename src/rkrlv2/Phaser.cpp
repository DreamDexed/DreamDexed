/*
  ZynAddSubFX - a software synthesizer

  Phaser.C - Phaser effect
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  Modified for rakarrack by Josep Andreu

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License (version 2) for more details.

  You should have received a copy of the GNU General Public License (version 2)
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

*/

#include <math.h>
#include "Phaser.h"
#define PHASER_LFO_SHAPE 2

Phaser::Phaser(float samplerate):
    bypass{},
    lfo{samplerate},
    Ppreset{},
    dry{1.0f},
    wet{}
{
    loadpreset(Ppreset);
    cleanup();
};

std::string Phaser::ToPresetName(int nValue, int nWidth)
{
    return nValue == 0 ? "INIT" : std::to_string(nValue);
}

void Phaser::process(float *smpsl, float *smpsr, uint16_t period)
{
    if (bypass) return;

    if (wet == 0.0f) return;

    if (lfo.nPeriod != period) lfo.updateparams(period);

    float lgain, rgain;
    lfo.effectlfoout(&lgain, &rgain);

    lgain =
        (expf(lgain * PHASER_LFO_SHAPE) - 1.0f) / (expf(PHASER_LFO_SHAPE) - 1.0f);
    rgain =
        (expf(rgain * PHASER_LFO_SHAPE) - 1.0f) / (expf(PHASER_LFO_SHAPE) - 1.0f);


    lgain = 1.0f - phase * (1.0f - depth) - (1.0f - phase) * lgain * depth;
    rgain = 1.0f - phase * (1.0f - depth) - (1.0f - phase) * rgain * depth;

    if (lgain > 1.0)
        lgain = 1.0f;
    else if (lgain < 0.0)
        lgain = 0.0f;
    if (rgain > 1.0)
        rgain = 1.0f;
    else if (rgain < 0.0)
        rgain = 0.0f;

    for (unsigned int i = 0; i < period; i++) {
        float x = (float) i / ((float)period);
        float x1 = 1.0f - x;
        float gl = lgain * x + oldlgain * x1;
        float gr = rgain * x + oldrgain * x1;
        float inl = smpsl[i] * panning + fbl;
        float inr = smpsr[i] * (1.0f - panning) + fbr;

        //Left channel
        for (int j = 0; j < Pstages * 2; j++) {
            //Phasing routine
            float tmp = oldl[j] + DENORMAL_GUARD;
            oldl[j] = gl * tmp + inl;
            inl = tmp - gl * oldl[j];
        };
        //Right channel
        for (int j = 0; j < Pstages * 2; j++) {
            //Phasing routine
            float tmp = oldr[j] + DENORMAL_GUARD;
            oldr[j] = (gr * tmp) + inr;
            inr = tmp - (gr * oldr[j]);
        };
        //Left/Right crossing
        float l = inl;
        float r = inr;
        inl = l * (1.0f - lrcross) + r * lrcross;
        inr = r * (1.0f - lrcross) + l * lrcross;

        fbl = inl * fb;
        fbr = inr * fb;

        if (Psubtractive != 0) {
            inl *= -1.0f;
            inr *= -1.0f;
        }

        smpsl[i] = smpsl[i] * dry + inl * wet;
        smpsr[i] = smpsr[i] * dry + inr * wet;
    };

    oldlgain = lgain;
    oldrgain = rgain;
};

void Phaser::cleanup()
{
    fbl = fbr = 0.0f;
    oldlgain = oldrgain = 0.0f;
    memset(oldl, 0, sizeof oldl);
    memset(oldr, 0, sizeof oldr);
};

void Phaser::setdepth(int Pdepth)
{
    this->Pdepth = Pdepth;
    depth = ((float)Pdepth / 127.0f);
};

void Phaser::setfb(int Pfb)
{
    this->Pfb = Pfb;
    fb = ((float)Pfb - 64.0f) / 64.1f;
};

void Phaser::setmix(int Pmix)
{
    this->Pmix = Pmix;

    float mix = (float)Pmix / 100.0f;
    if (mix < 0.5f)
    {
        dry = 1.0f;
        wet = mix * 2.0f;
    }
    else
    {
        dry = (1.0f - mix) * 2.0f;
        wet = 1.0f;
    }
};

void Phaser::setpanning(int Ppanning)
{
    this->Ppanning = Ppanning;
    panning = ((float)Ppanning - .5f)/ 127.0f;
};

void Phaser::setlrcross(int Plrcross)
{
    this->Plrcross = Plrcross;
    lrcross = (float)Plrcross / 127.0f;
};

void Phaser::setstages(int Pstages)
{
    if (Pstages > MAX_PHASER_STAGES)
        Pstages = MAX_PHASER_STAGES;
    this->Pstages = Pstages;
    cleanup ();
};

void Phaser::setphase(int Pphase)
{
    this->Pphase = Pphase;
    phase = ((float)Pphase / 127.0f);
};

void Phaser::loadpreset(unsigned npreset)
{
    const int PRESET_SIZE = 12;
    const int presets[presets_num][PRESET_SIZE] = {
        {0, 64, 11, 0, 0, 64, 110, 64, 1, 0, 0, 20},
        {50, 64, 11, 0, 0, 64, 110, 64, 1, 0, 0, 20},
        {50, 64, 10, 0, 0, 88, 40, 64, 3, 0, 0, 20},
        {50, 64, 8, 0, 0, 66, 68, 107, 2, 0, 0, 20},
        {31, 64, 1, 0, 0, 66, 67, 10, 5, 0, 1, 20},
        {50, 64, 1, 0, 1, 110, 67, 78, 10, 0, 0, 20},
        {50, 64, 31, 100, 0, 58, 37, 78, 3, 0, 0, 20}
    };

    if (npreset >= presets_num)
        npreset = presets_num;

    for (int n = 0; n < PRESET_SIZE; n++)
        changepar(n, presets[npreset][n]);

    Ppreset = npreset;
};

void Phaser::changepar(unsigned npar, int value)
{
    switch (npar) {
    case ParameterMix:
        setmix(value);
        break;
    case ParameterPanning:
        setpanning(value);
        break;
    case ParameterLFOFreq:
        lfo.Pfreq = value;
        lfo.updateparams(lfo.nPeriod);
        break;
    case ParameterLFORandomness:
        lfo.Prandomness = value;
        lfo.updateparams(lfo.nPeriod);
        break;
    case ParameterLFOType:
        lfo.PLFOtype = value;
        lfo.updateparams(lfo.nPeriod);
        break;
    case ParameterLFOLRDelay:
        lfo.Pstereo = value;
        lfo.updateparams(lfo.nPeriod);
        break;
    case ParameterDepth:
        setdepth(value);
        break;
    case ParameterFeedback:
        setfb(value);
        break;
    case ParameterStages:
        setstages(value);
        break;
    case ParameterLRCross:
        setlrcross(value);
        break;
    case ParameterSubtractive:
        if (value > 1)
            value = 1;
        Psubtractive = value;
        break;
    case ParameterPhase:
        setphase(value);
        break;
    }
};

int Phaser::getpar(unsigned npar)
{
    switch (npar) {
    case ParameterMix: return Pmix;
    case ParameterPanning: return Ppanning;
    case ParameterLFOFreq: return lfo.Pfreq;
    case ParameterLFORandomness: return lfo.Prandomness;
    case ParameterLFOType: return lfo.PLFOtype;
    case ParameterLFOLRDelay: return lfo.Pstereo;
    case ParameterDepth: return Pdepth;
    case ParameterFeedback: return Pfb;
    case ParameterStages: return Pstages;
    case ParameterLRCross: return Plrcross;
    case ParameterSubtractive: return Psubtractive;
    case ParameterPhase: return Pphase;
    default: return 0;
    }
};
