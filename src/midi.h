//
// midi.h
//
// MiniDexed - Dexed FM synthesizer for bare metal Raspberry Pi
// Copyright (C) 2025  The MiniDexed Team
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

#define MIDI_NOTE_OFF		0b1000
#define MIDI_NOTE_ON		0b1001
#define MIDI_AFTERTOUCH		0b1010			// TODO
#define MIDI_CHANNEL_AFTERTOUCH 0b1101   // right now Synth_Dexed just manage Channel Aftertouch not Polyphonic AT -> 0b1010
#define MIDI_CONTROL_CHANGE	0b1011

#define MIDI_CC_BANK_SELECT_MSB		0
#define MIDI_CC_MODULATION		1
#define MIDI_CC_BREATH_CONTROLLER	2 
#define MIDI_CC_FOOT_PEDAL 		4
#define MIDI_CC_PORTAMENTO_TIME		5
#define MIDI_CC_VOLUME			7
#define MIDI_CC_PAN_POSITION		10
#define MIDI_CC_EXPRESSION		11
#define MIDI_CC_BANK_SELECT_LSB		32
#define MIDI_CC_SUSTAIN	                64
#define MIDI_CC_PORTAMENTO		65
#define MIDI_CC_SOSTENUTO		66
#define MIDI_CC_HOLD2		        69
#define MIDI_CC_RESONANCE		71
#define MIDI_CC_FREQUENCY_CUTOFF	74
#define MIDI_CC_EFFECT1_SEND		91
#define MIDI_CC_EFFECT2_SEND		92
#define MIDI_CC_DETUNE_LEVEL		94
#define MIDI_CC_ALL_SOUND_OFF		120
#define MIDI_CC_ALL_NOTES_OFF		123
#define MIDI_CC_OMNI_MODE_OFF		124
#define MIDI_CC_OMNI_MODE_ON		125
#define MIDI_CC_MONO_MODE_ON		126
#define MIDI_CC_POLY_MODE_ON		127

#define MIDI_PROGRAM_CHANGE	0b1100
#define MIDI_PITCH_BEND		0b1110

static const uint16_t MIDI_EQ_HZ[] = {
        20, 22, 25, 28, 32, 36, 40, 45, 50, 56,
        63, 70, 80, 90, 100, 110, 125, 140, 160, 180,
        200, 225, 250, 280, 315, 355, 400, 450, 500, 560,
        630, 700, 800, 900, 1000, 1100, 1200, 1400, 1600, 1800,
        2000, 2200, 2500, 2800, 3200, 3600, 4000, 4500, 5000, 5600,
        6300, 7000, 8000, 9000, 10000, 11000, 12000, 14000, 16000, 18000,
        20000
};
