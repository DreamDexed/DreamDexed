//
// uimenu.h
//
// MiniDexed - Dexed FM synthesizer for bare metal Raspberry Pi
// Copyright (C) 2022  The MiniDexed Team
//
// Original author of this class:
//	R. Stange <rsta2@o2online.de>
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
#include <string>

#include <circle/timer.h>

#include "config.h"

class CMiniDexed;
class CUserInterface;

class CUIMenu
{
private:
	static constexpr int MaxMenuDepth = 6;

public:
	enum TMenuEvent
	{
		MenuEventUpdate,
		MenuEventUpdateParameter,
		MenuEventSelect,
		MenuEventBack,
		MenuEventHome,
		MenuEventStepDown,
		MenuEventStepUp,
		MenuEventPressAndStepDown,
		MenuEventPressAndStepUp,
		MenuEventPgmUp,
		MenuEventPgmDown,
		MenuEventBankUp,
		MenuEventBankDown,
		MenuEventTGUp,
		MenuEventTGDown,
		MenuEventUnknown
	};

public:
	CUIMenu(CUserInterface *pUI, CMiniDexed *pMiniDexed, CConfig *pConfig);

	void EventHandler(TMenuEvent Event);

private:
	typedef void TMenuHandler(CUIMenu *pUIMenu, TMenuEvent Event);

	struct TMenuItem
	{
		const char *Name;
		TMenuHandler *Handler;
		const TMenuItem *MenuItem;
		int Parameter;
		TMenuHandler *OnSelect;
		TMenuHandler *StepDown;
		TMenuHandler *StepUp;
		int8_t nBus;
		int8_t idFX;
		bool ShowDirect;
	};

	typedef std::string TToString(int nValue, int nWidth);

	struct TParameter
	{
		int Minimum;
		int Maximum;
		int Increment;
		TToString *ToString;
	};

private:
	static void MenuHandler(CUIMenu *pUIMenu, TMenuEvent Event);
	static void EditGlobalParameter(CUIMenu *pUIMenu, TMenuEvent Event);
	static void EditVoiceBankNumber(CUIMenu *pUIMenu, TMenuEvent Event);
	static void EditProgramNumber(CUIMenu *pUIMenu, TMenuEvent Event);
	static void EditTGParameter(CUIMenu *pUIMenu, TMenuEvent Event);
	static void EditFXParameter2(CUIMenu *pUIMenu, TMenuEvent Event);
	static void EditFXParameterG(CUIMenu *pUIMenu, TMenuEvent Event);
	static void EditBusParameter(CUIMenu *pUIMenu, TMenuEvent Event);
	static void EditBusParameterG(CUIMenu *pUIMenu, TMenuEvent Event);
	static void EditVoiceParameter(CUIMenu *pUIMenu, TMenuEvent Event);
	static void EditOPParameter(CUIMenu *pUIMenu, TMenuEvent Event);
	static void SavePerformance(CUIMenu *pUIMenu, TMenuEvent Event);
	static void EditTGParameter2(CUIMenu *pUIMenu, TMenuEvent Event);
	static void EditTGParameterModulation(CUIMenu *pUIMenu, TMenuEvent Event);
	static void PerformanceMenu(CUIMenu *pUIMenu, TMenuEvent Event);
	static void SavePerformanceNewFile(CUIMenu *pUIMenu, TMenuEvent Event);
	static void EditPerformanceBankNumber(CUIMenu *pUIMenu, TMenuEvent Event);
	static void ShowCPUTemp(CUIMenu *pUIMenu, TMenuEvent Event);
	static void ShowCPUSpeed(CUIMenu *pUIMenu, TMenuEvent Event);
	static void ShowIPAddr(CUIMenu *pUIMenu, TMenuEvent Event);
	static void ShowVersion(CUIMenu *pUIMenu, TMenuEvent Event);

	static std::string GetGlobalValueString(int nParameter, int nValue, int nWidth);
	static std::string GetTGValueString(int nTGParameter, int nValue, int nWidth);
	static std::string GetFXValueString(int nFXParameter, int nValue, int nWidth);
	static std::string GetBusValueString(int nFXParameter, int nValue, int nWidth);
	static std::string GetVoiceValueString(int nVoiceParameter, int nValue, int nWidth);
	static std::string GetOPValueString(int nOPParameter, int nValue, int nWidth);

	static std::string ToSDFilter(int nValue, int nWidth);

	void GlobalShortcutHandler(TMenuEvent Event);
	void TGShortcutHandler(TMenuEvent Event);
	void OPShortcutHandler(TMenuEvent Event);

	void PgmUpDownHandler(TMenuEvent Event);
	void BankUpDownHandler(TMenuEvent Event);
	void TGUpDownHandler(TMenuEvent Event);

	static void TimerHandler(TKernelTimerHandle hTimer, void *pParam, void *pContext);
	static void TimerHandlerUpdate(TKernelTimerHandle hTimer, void *pParam, void *pContext);

	static void InputTxt(CUIMenu *pUIMenu, TMenuEvent Event);
	static void TimerHandlerNoBack(TKernelTimerHandle hTimer, void *pParam, void *pContext);

	static void InputKeyDown(CUIMenu *pUIMenu, TMenuEvent Event);
	static void InputShiftKeyDown(CUIMenu *pUIMenu, TMenuEvent Event);

	static void SelectCurrentEffect(CUIMenu *pUIMenu, TMenuEvent Event);
	static void StepDownEffect(CUIMenu *pUIMenu, TMenuEvent Event);
	static void StepUpEffect(CUIMenu *pUIMenu, TMenuEvent Event);
	static bool FXSlotFilter(CUIMenu *pUIMenu, TMenuEvent Event, int nValue);

private:
	CUserInterface *m_pUI;
	CMiniDexed *m_pMiniDexed;
	CConfig *m_pConfig;

	int m_nToneGenerators;

	const TMenuItem *m_pParentMenu;
	const TMenuItem *m_pCurrentMenu;
	int m_nCurrentMenuItem;
	int m_nCurrentSelection;
	int m_nCurrentParameter;

	const TMenuItem *m_MenuStackParent[MaxMenuDepth];
	const TMenuItem *m_MenuStackMenu[MaxMenuDepth];
	int m_nMenuStackItem[MaxMenuDepth];
	int m_nMenuStackSelection[MaxMenuDepth];
	int m_nMenuStackParameter[MaxMenuDepth];
	int m_nCurrentMenuDepth;

	static const TMenuItem s_MenuRoot[];
	static const TMenuItem s_MainMenu[];
	static const TMenuItem s_TGMenu[];
	static const TMenuItem s_EffectsMenu[];
	static const TMenuItem s_BusMenu[];
	static const TMenuItem s_OutputMenu[];
	static const TMenuItem s_SendFXMenu[];
	static const TMenuItem s_MasterFXMenu[];
	static const TMenuItem s_FXListMenu[];
	static const TMenuItem s_MixerMenu[];
	static const TMenuItem s_EQMenu[];
	static const TMenuItem s_ZynDistortionMenu[];
	static const TMenuItem s_YKChorusMenu[];
	static const TMenuItem s_ZynChorusMenu[];
	static const TMenuItem s_ZynSympatheticMenu[];
	static const TMenuItem s_ZynAPhaserMenu[];
	static const TMenuItem s_ZynPhaserMenu[];
	static const TMenuItem s_DreamDelayMenu[];
	static const TMenuItem s_PlateReverbMenu[];
	static const TMenuItem s_CloudSeed2Menu[];
	static const TMenuItem s_CloudSeed2InputMenu[];
	static const TMenuItem s_CloudSeed2MultitapMenu[];
	static const TMenuItem s_CloudSeed2EarlyDiffusionMenu[];
	static const TMenuItem s_CloudSeed2LateDiffusionMenu[];
	static const TMenuItem s_CloudSeed2LateLineMenu[];
	static const TMenuItem s_CloudSeed2LowShelfMenu[];
	static const TMenuItem s_CloudSeed2HighShelfMenu[];
	static const TMenuItem s_CloudSeed2LowPassMenu[];
	static const TMenuItem s_CompressorMenu[];
	static const TMenuItem s_FXEQMenu[];
	static const TMenuItem s_EditCompressorMenu[];
	static const TMenuItem s_EditVoiceMenu[];
	static const TMenuItem s_OperatorMenu[];
	static const TMenuItem s_SaveMenu[];
	static const TMenuItem s_EditPitchBendMenu[];
	static const TMenuItem s_EditPortamentoMenu[];
	static const TMenuItem s_EditNoteLimitMenu[];
	static const TMenuItem s_PerformanceMenu[];
	static const TMenuItem s_StatusMenu[];

	static const TMenuItem s_ModulationMenu[];
	static const TMenuItem s_ModulationMenuParameters[];

	static const TMenuItem s_MIDIMenu[];

	static TParameter s_GlobalParameter[];
	static TParameter s_TGParameter[];
	static const TParameter s_VoiceParameter[];
	static const TParameter s_OPParameter[];

	static const char s_NoteName[100][5];
	static const char s_MIDINoteName[128][9];
	static const char s_MIDINoteShift[49][8];

	std::string m_InputText = "1234567890ABCD";
	unsigned m_InputTextPosition = 0;
	int m_InputTextChar = 32;
	bool m_bPerformanceDeleteMode = false;
	bool m_bConfirmDeletePerformance = false;
	int m_nSelectedPerformanceID = 0;
	int m_nSelectedPerformanceBankID = 0;
	bool m_bSplashShow = false;
};
