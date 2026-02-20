//
// udpmididevice.cpp
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

#include "udpmididevice.h"

#include <cassert>
#include <cstdint>

#include <circle/logger.h>
#include <circle/net/in.h>
#include <circle/net/ipaddress.h>
#include <circle/net/netsubsystem.h>
#include <circle/net/socket.h>
#include <circle/string.h>

#include "config.h"
#include "mididevice.h"
#include "net/applemidi.h"
#include "net/udpmidi.h"
#include "userinterface.h"

#define VIRTUALCABLE 0

LOGMODULE("udpmididevice");

CUDPMIDIDevice::CUDPMIDIDevice(CMiniDexed *pSynthesizer, CConfig *pConfig, CUserInterface *pUI) :
CMIDIDevice{pSynthesizer, pConfig, pUI},
m_pSynthesizer{pSynthesizer},
m_pConfig{pConfig}
{
	AddDevice("udp");
}

CUDPMIDIDevice::~CUDPMIDIDevice()
{
	// m_pSynthesizer = 0;
}

bool CUDPMIDIDevice::Initialize()
{
	m_pAppleMIDIParticipant = new CAppleMIDIParticipant(&m_Random, this, m_pConfig->GetNetworkHostname());
	if (!m_pAppleMIDIParticipant->Initialize())
	{
		LOGERR("Failed to init RTP listener");
		delete m_pAppleMIDIParticipant;
		m_pAppleMIDIParticipant = nullptr;
	}
	else
		LOGNOTE("RTP Listener initialized");

	if (m_pConfig->GetUDPMIDIEnabled())
	{
		m_pUDPMIDIReceiver = new CUDPMIDIReceiver(this);
		if (!m_pUDPMIDIReceiver->Initialize())
		{
			LOGERR("Failed to init UDP MIDI receiver");
			delete m_pUDPMIDIReceiver;
			m_pUDPMIDIReceiver = nullptr;
		}
		else
			LOGNOTE("UDP MIDI receiver initialized");

		// UDP MIDI send socket setup (default: broadcast 255.255.255.255:1999)
		m_UDPDestAddress.Set(0xFFFFFFFF); // Broadcast by default
		m_UDPDestPort = 1999;
		if (m_pConfig->GetUDPMIDIIPAddress().IsSet())
		{
			m_UDPDestAddress.Set(m_pConfig->GetUDPMIDIIPAddress());
		}
		CString IPAddressString;
		m_UDPDestAddress.Format(&IPAddressString);

		// address 0.0.0.0 disables transmit
		if (!m_UDPDestAddress.IsNull())
		{
			CNetSubSystem *pNet = CNetSubSystem::Get();
			m_pUDPSendSocket = new CSocket(pNet, IPPROTO_UDP);
			m_pUDPSendSocket->Connect(m_UDPDestAddress, m_UDPDestPort);
			m_pUDPSendSocket->SetOptionBroadcast(true);

			LOGNOTE("UDP MIDI sender initialized. target is %s",
				IPAddressString.c_str());
		}
		else
			LOGNOTE("UDP MIDI sender disabled. target was %s",
				IPAddressString.c_str());
	}
	else
		LOGNOTE("UDP MIDI is disabled in configuration");

	return true;
}

// Methods to handle MIDI events

void CUDPMIDIDevice::OnAppleMIDIDataReceived(const uint8_t *pData, size_t nSize)
{
	MIDIMessageHandler(pData, nSize, VIRTUALCABLE);
}

void CUDPMIDIDevice::OnAppleMIDIConnect(const CIPAddress *pIPAddress, const char *pName)
{
	m_bIsAppleMIDIConnected = true;
	LOGNOTE("RTP Device connected");
}

void CUDPMIDIDevice::OnAppleMIDIDisconnect(const CIPAddress *pIPAddress, const char *pName)
{
	m_bIsAppleMIDIConnected = false;
	LOGNOTE("RTP Device disconnected");
}

void CUDPMIDIDevice::OnUDPMIDIDataReceived(const uint8_t *pData, size_t nSize)
{
	MIDIMessageHandler(pData, nSize, VIRTUALCABLE);
}

void CUDPMIDIDevice::Send(const uint8_t *pMessage, int nLength, int nCable)
{
	if (m_pAppleMIDIParticipant && m_bIsAppleMIDIConnected)
	{
		bool res = m_pAppleMIDIParticipant->SendMIDIToHost(pMessage, nLength);
		if (!res)
		{
			LOGERR("Failed to send %d bytes to RTP-MIDI host", nLength);
		}
		else
		{
			// LOGDBG("Sent %d bytes to RTP-MIDI host", nLength);
		}
	}

	if (m_pUDPSendSocket)
	{
		int res = m_pUDPSendSocket->SendTo(pMessage, static_cast<unsigned>(nLength), 0, m_UDPDestAddress, m_UDPDestPort);
		if (res < 0)
		{
			LOGERR("Failed to send %d bytes to UDP MIDI host", nLength);
		}
		else
		{
			// LOGDBG("Sent %d bytes to UDP MIDI host",nLength);
		}
	}
}
