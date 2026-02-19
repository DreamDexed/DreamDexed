//
// applemidi.h
//
// mt32-pi - A baremetal MIDI synthesizer for Raspberry Pi
// Copyright (C) 2020-2023 Dale Whinham <daleyo@gmail.com>
//
// This file is part of mt32-pi.
//
// mt32-pi is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// mt32-pi is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// mt32-pi. If not, see <http://www.gnu.org/licenses/>.
//

#pragma once

#include <cstdint>

#include <circle/bcmrandom.h>
#include <circle/net/ipaddress.h>
#include <circle/net/socket.h>
#include <circle/netdevice.h>
#include <circle/sched/task.h>

class CAppleMIDIHandler
{
public:
	virtual void OnAppleMIDIDataReceived(const uint8_t *pData, int nSize) = 0;
	virtual void OnAppleMIDIConnect(const CIPAddress *pIPAddress, const char *pName) = 0;
	virtual void OnAppleMIDIDisconnect(const CIPAddress *pIPAddress, const char *pName) = 0;
};

class CAppleMIDIParticipant : protected CTask
{
public:
	CAppleMIDIParticipant(CBcmRandomNumberGenerator *pRandom, CAppleMIDIHandler *pHandler, const char *pSessionName);
	virtual ~CAppleMIDIParticipant() override;

	bool Initialize();

	virtual void Run() override;

public:
	bool SendMIDIToHost(const uint8_t *pData, int nSize);

private:
	void ControlInvitationState();
	void MIDIInvitationState();
	void ConnectedState();
	void Reset();

	bool SendPacket(CSocket *pSocket, CIPAddress *pIPAddress, uint16_t nPort, const void *pData, int nSize);
	bool SendAcceptInvitationPacket(CSocket *pSocket, CIPAddress *pIPAddress, uint16_t nPort);
	bool SendRejectInvitationPacket(CSocket *pSocket, CIPAddress *pIPAddress, uint16_t nPort, uint32_t nInitiatorToken);
	bool SendSyncPacket(uint64_t nTimestamp1, uint64_t nTimestamp2);
	bool SendFeedbackPacket();

	CBcmRandomNumberGenerator *m_pRandom;

	// UDP sockets
	CSocket *m_pControlSocket;
	CSocket *m_pMIDISocket;

	// Foreign peers
	CIPAddress m_ForeignControlIPAddress;
	CIPAddress m_ForeignMIDIIPAddress;
	uint16_t m_nForeignControlPort;
	uint16_t m_nForeignMIDIPort;

	// Connected peer
	CIPAddress m_InitiatorIPAddress;
	uint16_t m_nInitiatorControlPort;
	uint16_t m_nInitiatorMIDIPort;

	// Socket receive buffers
	uint8_t m_ControlBuffer[FRAME_BUFFER_SIZE];
	uint8_t m_MIDIBuffer[FRAME_BUFFER_SIZE];

	int m_nControlResult;
	int m_nMIDIResult;

	// Callback handler
	CAppleMIDIHandler *m_pHandler;

	// Participant state machine
	enum class TState
	{
		ControlInvitation,
		MIDIInvitation,
		Connected
	};

	TState m_State;

	uint32_t m_nInitiatorToken = 0;
	uint32_t m_nInitiatorSSRC = 0;
	uint32_t m_nSSRC = 0;
	uint32_t m_nLastMIDISequenceNumber = 0;

	uint64_t m_nOffsetEstimate = 0;
	uint64_t m_nLastSyncTime = 0;

	uint16_t m_nSequence = 0;
	uint16_t m_nLastFeedbackSequence = 0;
	uint64_t m_nLastFeedbackTime = 0;

	const char *m_pSessionName;
};
