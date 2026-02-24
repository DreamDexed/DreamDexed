#include "bus.h"
#include "uitostring.h"

Bus::ParameterType Bus::s_Parameter[Bus::Unknown] =
{
	{0, 99, 99, 1, "MixerDryLevel"},
	{0, 99, 0, 1, "ReturnLevel"},
	{0, 1, 0, 1, "FXBypass", ToOnOff},
};
