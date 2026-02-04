/*
 * CloudSeed
 * Ported from https://github.com/GhostNoteAudio/CloudSeedCore
 */

#pragma once

#define BUFFER_SIZE 128
#define SLOW_CLEAR_SIZE 192000 // may need to be adjusted for other Pis

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstring>
#include <string>

#include "../CloudSeedCore/DSP/ReverbController.h"
#include "../CloudSeedCore/Parameters.h"

class AudioEffectCloudSeed2
{
public:
	static constexpr const char *PresetNames[] = {
		"Init",
		"FXDivineInspiration",
		"FXLawsOfPhysics",
		"FXSlowBraaam",
		"FXTheUpsideDown",
		"LBigSoundStage",
		"LDiffusionCyclone",
		"LScreamIntoTheVoid",
		"M90sDigitalReverb",
		"MAiryAmbience",
		"MDarkPlate",
		"MGhostly",
		"MTappedLines",
		"SFastAttack",
		"SSmallPlate",
		"SSnappyAttack",
	};

	static constexpr int presets_num = sizeof PresetNames / sizeof *PresetNames;
	static const float *Presets[];

	static std::string GetLateMode(int nValue, int nWidth)
	{
		return nValue ? "Post" : "Pre";
	}

	static std::string getPresetName(int nValue, int nWidth)
	{
		assert(nValue >= 0 && nValue < presets_num);
		return PresetNames[nValue];
	}

	static const char *getPresetNameChar(int nValue)
	{
		assert(nValue >= 0 && nValue < presets_num);
		return PresetNames[nValue];
	}

	static int getIDFromPresetName(const char *presetName)
	{
		for (int i = 0; i < presets_num; ++i)
			if (strcmp(PresetNames[i], presetName) == 0)
				return i;

		return 0;
	}

	AudioEffectCloudSeed2(float samplerate) :
	bypass{},
	ramp_dt{10.0f / samplerate},
	engine{static_cast<int>(samplerate)},
	targetVol{},
	needBufferClear{},
	waitBufferClear{},
	needParameterLoad{},
	preset{},
	vol{}
	{
	}

	void setParameter(int paramID, float param)
	{
		engine.SetParameter(paramID, param);
	}

	float getParameter(int paramID)
	{
		return engine.GetAllParameters()[paramID];
	}

	void process(float *inblockL, float *inblockR, int len)
	{
		if (targetVol == 0.0f && vol > 0.0f)
		{
			engine.Process(inblockL, inblockR, inblockL, inblockR, len);
			for (int i = 0; i < len; ++i)
			{
				vol = std::max(0.0f, vol - ramp_dt);
				inblockL[i] *= vol;
				inblockR[i] *= vol;
			}

			return;
		}

		if (needBufferClear)
		{
			engine.StartSlowClear();
			needBufferClear = false;
			waitBufferClear = true;
		}

		if (waitBufferClear)
		{
			if (engine.SlowClearDone(SLOW_CLEAR_SIZE))
				waitBufferClear = false;

			std::fill_n(inblockL, len, 0);
			std::fill_n(inblockR, len, 0);

			return;
		}

		if (needParameterLoad)
		{
			int needParam = needParameterLoad;
			int paramID = Cloudseed::Parameter::COUNT - needParam;
			engine.SetParameter(paramID, Presets[preset][paramID]);
			needParameterLoad = needParam - 1;

			std::fill_n(inblockL, len, 0);
			std::fill_n(inblockR, len, 0);

			if (!needParameterLoad)
				targetVol = 1.0f;

			return;
		}

		if (targetVol == 1.0f && vol < 1.0f)
		{
			engine.Process(inblockL, inblockR, inblockL, inblockR, len);
			for (int i = 0; i < len; ++i)
			{
				vol = std::min(vol + ramp_dt, 1.0f);
				inblockL[i] *= vol;
				inblockR[i] *= vol;
			}

			return;
		}

		if (bypass) return;

		if (isDisabled()) return;

		engine.Process(inblockL, inblockR, inblockL, inblockR, len);
	}

	void loadPreset(int p)
	{
		assert(p >= 0 && p < presets_num);
		preset = p;

		targetVol = 0.0f;
		needBufferClear = true;
		needParameterLoad = Cloudseed::Parameter::COUNT;
	}

	void setNeedBufferClear()
	{
		needBufferClear = true;
	}

	void setRampedDown()
	{
		vol = 0.0f;
	}

	bool isDisabled()
	{
		double *params = engine.GetAllParameters();
		return params[Cloudseed::Parameter::DryOut] == 1.0f &&
		       params[Cloudseed::Parameter::EarlyOut] == 0.0f &&
		       params[Cloudseed::Parameter::LateOut] == 0.0f;
	}

	std::atomic<bool> bypass;

private:
	float ramp_dt;
	Cloudseed::ReverbController engine;
	std::atomic<float> targetVol;
	std::atomic<bool> needBufferClear;
	bool waitBufferClear;
	std::atomic<int> needParameterLoad;
	std::atomic<int> preset;

	std::atomic<float> vol;
};
