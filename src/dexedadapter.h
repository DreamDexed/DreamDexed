//
// dexedadapter.h
//
// MiniDexed - Dexed FM synthesizer for bare metal Raspberry Pi
// Copyright (C) 2022  The MiniDexed Team
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#pragma once

#include <cstdint>

#include <circle/spinlock.h>
#include <compressor.h>
#include <dexed.h>
#include <synth_dexed.h>

#include "effect_3bandeqmono.h"

#define DEXED_OP_ENABLE (DEXED_OP_OSC_DETUNE + 1)

// Some Dexed methods require to be guarded from being interrupted
// by other Dexed calls. This is done herein.

class CDexedAdapter : public Dexed
{
public:
	CDexedAdapter(int maxnotes, unsigned samplerate) :
	Dexed{static_cast<uint8_t>(maxnotes), samplerate},
	EQ{static_cast<float>(samplerate)},
	Compr{static_cast<float>(samplerate)},
	m_bCompressorEnable{}
	{
	}

	void loadVoiceParameters(uint8_t *data)
	{
		m_SpinLock.Acquire();
		Dexed::loadVoiceParameters(data);
		m_SpinLock.Release();
	}

	void keyup(uint8_t pitch)
	{
		m_SpinLock.Acquire();
		Dexed::keyup(pitch);
		m_SpinLock.Release();
	}

	void keydown(uint8_t pitch, uint8_t velo)
	{
		m_SpinLock.Acquire();
		Dexed::keydown(pitch, velo);
		m_SpinLock.Release();
	}

	void getSamples(float *buffer, int n_samples)
	{
		m_SpinLock.Acquire();
		Dexed::getSamples(buffer, static_cast<uint16_t>(n_samples));
		EQ.process(buffer, n_samples);
		if (m_bCompressorEnable)
		{
			Compr.doCompression(buffer, static_cast<uint16_t>(n_samples));
		}
		m_SpinLock.Release();
	}

	void ControllersRefresh()
	{
		m_SpinLock.Acquire();
		Dexed::ControllersRefresh();
		m_SpinLock.Release();
	}

	void setSustain(bool sustain)
	{
		m_SpinLock.Acquire();
		Dexed::setSustain(sustain);
		m_SpinLock.Release();
	}

	void setCompressorEnable(bool enable)
	{
		m_SpinLock.Acquire();
		m_bCompressorEnable = enable;
		m_SpinLock.Release();
	}

	void resetState()
	{
		m_SpinLock.Acquire();
		deactivate();
		resetFxState();
		EQ.resetState();
		Compr.resetStates();
		m_SpinLock.Release();
	}

	AudioEffect3BandEQMono EQ;
	Compressor Compr;

private:
	CSpinLock m_SpinLock;
	bool m_bCompressorEnable;
};
