#include "bus.h"
#include "mididevice.h"
#include "performanceconfig.h"
#include "uitostring.h"

Bus::ParameterType Bus::s_Parameter[Bus::Unknown] =
{
	{0, NUM_PERFORMANCE_BANKS - 1, 0, 1, "Bank", .Flags = Flag::UIOnly},
	{0, NUM_PERFORMANCES - 1, 0, 1, "Performance", .Flags = Flag::UIOnly},
	{0, Bus::LoadType::LoadTypeUnknown - 1, 0, 1, "LoadType", ToLoadType, .Flags = Flag::UIOnly},
	{0, CMIDIDevice::Disabled, CMIDIDevice::Disabled, 1, "MIDIChannel", ToMIDIChannel},
	{0, 99, 99, 1, "MixerDryLevel"},
	{0, 99, 0, 1, "ReturnLevel"},
	{0, 1, 0, 1, "FXBypass", ToOnOff},
};
