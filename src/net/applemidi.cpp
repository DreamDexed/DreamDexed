//
// applemidi.cpp
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

#include "applemidi.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include <circle/bcmrandom.h>
#include <circle/logger.h>
#include <circle/macros.h>
#include <circle/net/in.h>
#include <circle/net/ipaddress.h>
#include <circle/net/netsubsystem.h>
#include <circle/net/socket.h>
#include <circle/sched/scheduler.h>
#include <circle/sched/task.h>
#include <circle/string.h>
#include <circle/sysconfig.h>
#include <circle/timer.h>
#include <circle/util.h>

#include "byteorder.h"
#include "utility.h"

#define MAX_DX7_SYSEX_LENGTH 4104
#define MAX_MIDI_MESSAGE MAX_DX7_SYSEX_LENGTH

// #define APPLEMIDI_DEBUG

LOGMODULE("applemidi");

constexpr uint16_t ControlPort = 5004;
constexpr uint16_t MIDIPort = ControlPort + 1;

constexpr uint16_t AppleMIDISignature = 0xFFFF;
constexpr uint8_t AppleMIDIVersion = 2;

constexpr uint8_t RTPMIDIPayloadType = 0x61;
constexpr uint8_t RTPMIDIVersion = 2;

// Arbitrary value
constexpr size_t MaxNameLength = 256;

// Timeout period for invitation (5 seconds in 100 microsecond units)
constexpr unsigned int InvitationTimeout = 5 * 10000;

// Timeout period for sync packets (60 seconds in 100 microsecond units)
constexpr unsigned int SyncTimeout = 60 * 10000;

// Receiver feedback packet frequency (1 second in 100 microsecond units)
constexpr unsigned int ReceiverFeedbackPeriod = 1 * 10000;

constexpr uint16_t CommandWord(const char Command[2])
{
	return static_cast<uint16_t>(Command[0] << 8 | Command[1]);
}

enum TAppleMIDICommand : uint16_t
{
	Invitation = CommandWord("IN"),
	InvitationAccepted = CommandWord("OK"),
	InvitationRejected = CommandWord("NO"),
	Sync = CommandWord("CK"),
	ReceiverFeedback = CommandWord("RS"),
	EndSession = CommandWord("BY"),
};

struct TAppleMIDISession
{
	uint16_t nSignature;
	uint16_t nCommand;
	uint32_t nVersion;
	uint32_t nInitiatorToken;
	uint32_t nSSRC;
	char Name[MaxNameLength];
} PACKED;

// The Name field is optional
constexpr int NamelessSessionPacketSize = sizeof(TAppleMIDISession) - sizeof(TAppleMIDISession::Name);

struct TAppleMIDISync
{
	uint16_t nSignature;
	uint16_t nCommand;
	uint32_t nSSRC;
	uint8_t nCount;
	uint8_t Padding[3];
	uint64_t Timestamps[3];
} PACKED;

struct TAppleMIDIReceiverFeedback
{
	uint16_t nSignature;
	uint16_t nCommand;
	uint32_t nSSRC;
	uint32_t nSequence;
} PACKED;

struct TRTPMIDI
{
	uint16_t nFlags;
	uint16_t nSequence;
	uint32_t nTimestamp;
	uint32_t nSSRC;
} PACKED;

uint64_t GetSyncClock()
{
	static const uint64_t nStartTime = CTimer::GetClockTicks();
	const uint64_t nMicrosSinceEpoch = CTimer::GetClockTicks();

	// Units of 100 microseconds
	return (nMicrosSinceEpoch - nStartTime) / 100;
}

bool ParseInvitationPacket(const uint8_t *pBuffer, int nSize, TAppleMIDISession *pOutPacket)
{
	const TAppleMIDISession *const pInPacket = reinterpret_cast<const TAppleMIDISession *>(pBuffer);

	if (nSize < NamelessSessionPacketSize)
		return false;

	const uint16_t nSignature = ntohs(pInPacket->nSignature);
	if (nSignature != AppleMIDISignature)
		return false;

	const uint16_t nCommand = ntohs(pInPacket->nCommand);
	if (nCommand != Invitation)
		return false;

	const uint32_t nVersion = ntohl(pInPacket->nVersion);
	if (nVersion != AppleMIDIVersion)
		return false;

	pOutPacket->nSignature = nSignature;
	pOutPacket->nCommand = nCommand;
	pOutPacket->nVersion = nVersion;
	pOutPacket->nInitiatorToken = ntohl(pInPacket->nInitiatorToken);
	pOutPacket->nSSRC = ntohl(pInPacket->nSSRC);

	if (nSize > NamelessSessionPacketSize)
		strncpy(pOutPacket->Name, pInPacket->Name, sizeof(pOutPacket->Name));
	else
		strncpy(pOutPacket->Name, "<unknown>", sizeof(pOutPacket->Name));

	return true;
}

bool ParseEndSessionPacket(const uint8_t *pBuffer, int nSize, TAppleMIDISession *pOutPacket)
{
	const TAppleMIDISession *const pInPacket = reinterpret_cast<const TAppleMIDISession *>(pBuffer);

	if (nSize < NamelessSessionPacketSize)
		return false;

	const uint16_t nSignature = ntohs(pInPacket->nSignature);
	if (nSignature != AppleMIDISignature)
		return false;

	const uint16_t nCommand = ntohs(pInPacket->nCommand);
	if (nCommand != EndSession)
		return false;

	const uint32_t nVersion = ntohl(pInPacket->nVersion);
	if (nVersion != AppleMIDIVersion)
		return false;

	pOutPacket->nSignature = nSignature;
	pOutPacket->nCommand = nCommand;
	pOutPacket->nVersion = nVersion;
	pOutPacket->nInitiatorToken = ntohl(pInPacket->nInitiatorToken);
	pOutPacket->nSSRC = ntohl(pInPacket->nSSRC);

	return true;
}

bool ParseSyncPacket(const uint8_t *pBuffer, int nSize, TAppleMIDISync *pOutPacket)
{
	const TAppleMIDISync *const pInPacket = reinterpret_cast<const TAppleMIDISync *>(pBuffer);

	if (nSize < ssizeof(TAppleMIDISync))
		return false;

	const uint16_t nSignature = ntohs(pInPacket->nSignature);
	if (nSignature != AppleMIDISignature)
		return false;

	const uint16_t nCommand = ntohs(pInPacket->nCommand);
	if (nCommand != Sync)
		return false;

	pOutPacket->nSignature = nSignature;
	pOutPacket->nCommand = nCommand;
	pOutPacket->nSSRC = ntohl(pInPacket->nSSRC);
	pOutPacket->nCount = pInPacket->nCount;
	pOutPacket->Timestamps[0] = ntohll(pInPacket->Timestamps[0]);
	pOutPacket->Timestamps[1] = ntohll(pInPacket->Timestamps[1]);
	pOutPacket->Timestamps[2] = ntohll(pInPacket->Timestamps[2]);

	return true;
}

uint8_t ParseMIDIDeltaTime(const uint8_t *pBuffer)
{
	uint8_t nLength = 0;

	while (nLength < 4)
	{
		// Upper bit not set; end of timestamp
		if ((pBuffer[nLength++] & (1 << 7)) == 0)
			break;
	}

	return nLength;
}

int ParseSysExCommand(const uint8_t *pBuffer, int nSize, CAppleMIDIHandler *pHandler)
{
	int nBytesParsed = 1;
	const uint8_t nHead = pBuffer[0];
	uint8_t nTail = 0;

	while (nBytesParsed < nSize && !(nTail == 0xF0 || nTail == 0xF7 || nTail == 0xF4))
		nTail = pBuffer[nBytesParsed++];

	int nReceiveLength = nBytesParsed;

	// First segmented SysEx packet
	if (nHead == 0xF0 && nTail == 0xF0)
	{
#ifdef APPLEMIDI_DEBUG
		LOGNOTE("Received segmented SysEx (first)");
#endif
		--nReceiveLength;
	}

	// Middle segmented SysEx packet
	else if (nHead == 0xF7 && nTail == 0xF0)
	{
#ifdef APPLEMIDI_DEBUG
		LOGNOTE("Received segmented SysEx (middle)");
#endif
		++pBuffer;
		nBytesParsed -= 2;
	}

	// Last segmented SysEx packet
	else if (nHead == 0xF7 && nTail == 0xF7)
	{
#ifdef APPLEMIDI_DEBUG
		LOGNOTE("Received segmented SysEx (last)");
#endif
		++pBuffer;
		--nReceiveLength;
	}

	// Cancelled segmented SysEx packet
	else if (nHead == 0xF7 && nTail == 0xF4)
	{
#ifdef APPLEMIDI_DEBUG
		LOGNOTE("Received cancelled SysEx");
#endif
		nReceiveLength = 1;
	}

#ifdef APPLEMIDI_DEBUG
	else
	{
		LOGNOTE("Received complete SysEx");
	}
#endif

	pHandler->OnAppleMIDIDataReceived(pBuffer, nReceiveLength);

	return nBytesParsed;
}

int ParseMIDICommand(const uint8_t *pBuffer, int nSize, uint8_t &nRunningStatus, CAppleMIDIHandler *pHandler)
{
	int nBytesParsed = 0;
	uint8_t nByte = pBuffer[0];

	// System Real-Time message - single byte, handle immediately
	// Can appear anywhere in the stream, even in between status/data bytes
	if (nByte >= 0xF8)
	{
		// Ignore undefined System Real-Time
		if (nByte != 0xF9 && nByte != 0xFD)
			pHandler->OnAppleMIDIDataReceived(&nByte, 1);

		return 1;
	}

	// Is it a status byte?
	if (nByte & 0x80)
	{
		// Update running status if non Real-Time System status
		if (nByte < 0xF0)
			nRunningStatus = nByte;
		else
			nRunningStatus = 0;

		++nBytesParsed;
	}
	else
	{
		// First byte not a status byte and no running status - invalid
		if (!nRunningStatus)
			return 0;

		// Use running status
		nByte = nRunningStatus;
	}

	// Channel messages
	if (nByte < 0xF0)
	{
		// How many data bytes?
		switch (nByte & 0xF0)
		{
		case 0x80: // Note off
		case 0x90: // Note on
		case 0xA0: // Polyphonic key pressure/aftertouch
		case 0xB0: // Control change
		case 0xE0: // Pitch bend
			nBytesParsed += 2;
			break;

		case 0xC0: // Program change
		case 0xD0: // Channel pressure/aftertouch
			nBytesParsed += 1;
			break;
		}

		// Handle command
		pHandler->OnAppleMIDIDataReceived(pBuffer, nBytesParsed);
		return nBytesParsed;
	}

	// System common commands
	switch (nByte)
	{
	case 0xF0: // Start of System Exclusive
	case 0xF7: // End of Exclusive
		return ParseSysExCommand(pBuffer, nSize, pHandler);

	case 0xF1: // MIDI Time Code Quarter Frame
	case 0xF3: // Song Select
		++nBytesParsed;
		break;

	case 0xF2: // Song Position Pointer
		nBytesParsed += 2;
		break;
	}

	pHandler->OnAppleMIDIDataReceived(pBuffer, nBytesParsed);
	return nBytesParsed;
}

bool ParseMIDICommandSection(const uint8_t *pBuffer, int nSize, CAppleMIDIHandler *pHandler)
{
	// Must have at least a header byte and a single status byte
	if (nSize < 2)
		return false;

	int nMIDICommandsProcessed = 0;
	int nBytesRemaining = nSize - 1;
	uint8_t nRunningStatus = 0;

	const uint8_t nMIDIHeader = pBuffer[0];
	const uint8_t *pMIDICommands = pBuffer + 1;

	// Lower 4 bits of the header is length
	int nMIDICommandLength = nMIDIHeader & 0x0F;

	// If B flag is set, length value is 12 bits
	if (nMIDIHeader & (1 << 7))
	{
		nMIDICommandLength <<= 8;
		nMIDICommandLength |= pMIDICommands[0];
		++pMIDICommands;
		--nBytesRemaining;
	}

	if (nMIDICommandLength > nBytesRemaining)
	{
		LOGERR("Invalid MIDI command length");
		return false;
	}

	// Begin decoding the command list
	while (nMIDICommandLength)
	{
		// If Z flag is set, first list entry is a delta time
		if (nMIDICommandsProcessed || nMIDIHeader & (1 << 5))
		{
			const uint8_t nBytesParsed = ParseMIDIDeltaTime(pMIDICommands);
			nMIDICommandLength -= nBytesParsed;
			pMIDICommands += nBytesParsed;
		}

		if (nMIDICommandLength)
		{
			const int nBytesParsed = ParseMIDICommand(pMIDICommands, nMIDICommandLength, nRunningStatus, pHandler);
			nMIDICommandLength -= nBytesParsed;
			pMIDICommands += nBytesParsed;
			++nMIDICommandsProcessed;
		}
	}

	return true;
}

bool ParseMIDIPacket(const uint8_t *pBuffer, int nSize, TRTPMIDI *pOutPacket, CAppleMIDIHandler *pHandler)
{
	assert(pHandler != nullptr);

	const TRTPMIDI *const pInPacket = reinterpret_cast<const TRTPMIDI *>(pBuffer);
	const uint16_t nRTPFlags = ntohs(pInPacket->nFlags);

	// Check size (RTP-MIDI header plus MIDI command section header)
	if (nSize < ssizeof(TRTPMIDI) + 1)
		return false;

	// Check version
	if (((nRTPFlags >> 14) & 0x03) != RTPMIDIVersion)
		return false;

	// Ensure no CSRC identifiers
	if (((nRTPFlags >> 8) & 0x0F) != 0)
		return false;

	// Check payload type
	if ((nRTPFlags & 0xFF) != RTPMIDIPayloadType)
		return false;

	pOutPacket->nFlags = nRTPFlags;
	pOutPacket->nSequence = ntohs(pInPacket->nSequence);
	pOutPacket->nTimestamp = ntohl(pInPacket->nTimestamp);
	pOutPacket->nSSRC = ntohl(pInPacket->nSSRC);

	// RTP-MIDI variable-length header
	const uint8_t *const pMIDICommandSection = pBuffer + sizeof(TRTPMIDI);
	int nRemaining = nSize - ssizeof(TRTPMIDI);
	return ParseMIDICommandSection(pMIDICommandSection, nRemaining, pHandler);
}

CAppleMIDIParticipant::CAppleMIDIParticipant(CBcmRandomNumberGenerator *pRandom, CAppleMIDIHandler *pHandler, const char *pSessionName) :
CTask{TASK_STACK_SIZE, true},

m_pRandom{pRandom},

m_pControlSocket{},
m_pMIDISocket{},

m_nForeignControlPort{},
m_nForeignMIDIPort{},
m_nInitiatorControlPort{},
m_nInitiatorMIDIPort{},
m_ControlBuffer{},
m_MIDIBuffer{},

m_nControlResult{},
m_nMIDIResult{},

m_pHandler{pHandler},

m_State{TState::ControlInvitation},

m_nInitiatorToken{},
m_nInitiatorSSRC{},
m_nSSRC{},
m_nLastMIDISequenceNumber{},

m_nOffsetEstimate{},
m_nLastSyncTime{},

m_nSequence{},
m_nLastFeedbackSequence{},
m_nLastFeedbackTime{},

m_pSessionName{pSessionName}
{
}

CAppleMIDIParticipant::~CAppleMIDIParticipant()
{
	if (m_pControlSocket)
		delete m_pControlSocket;

	if (m_pMIDISocket)
		delete m_pMIDISocket;
}

bool CAppleMIDIParticipant::Initialize()
{
	assert(m_pControlSocket == nullptr);
	assert(m_pMIDISocket == nullptr);

	CNetSubSystem *const pNet = CNetSubSystem::Get();

	if ((m_pControlSocket = new CSocket(pNet, IPPROTO_UDP)) == nullptr)
		return false;

	if ((m_pMIDISocket = new CSocket(pNet, IPPROTO_UDP)) == nullptr)
		return false;

	if (m_pControlSocket->Bind(ControlPort) != 0)
	{
		LOGERR("Couldn't bind to port %d", ControlPort);
		return false;
	}

	if (m_pMIDISocket->Bind(MIDIPort) != 0)
	{
		LOGERR("Couldn't bind to port %d", MIDIPort);
		return false;
	}

	// We started as a suspended task; run now that initialization is successful
	Start();

	return true;
}

void CAppleMIDIParticipant::Run()
{
	assert(m_pControlSocket != nullptr);
	assert(m_pMIDISocket != nullptr);

	CScheduler *const pScheduler = CScheduler::Get();

	while (true)
	{
		if ((m_nControlResult = m_pControlSocket->ReceiveFrom(m_ControlBuffer, sizeof(m_ControlBuffer), MSG_DONTWAIT, &m_ForeignControlIPAddress, &m_nForeignControlPort)) < 0)
			LOGERR("Control socket receive error: %d", m_nControlResult);

		if ((m_nMIDIResult = m_pMIDISocket->ReceiveFrom(m_MIDIBuffer, sizeof(m_MIDIBuffer), MSG_DONTWAIT, &m_ForeignMIDIIPAddress, &m_nForeignMIDIPort)) < 0)
			LOGERR("MIDI socket receive error: %d", m_nMIDIResult);

		switch (m_State)
		{
		case TState::ControlInvitation:
			ControlInvitationState();
			break;

		case TState::MIDIInvitation:
			MIDIInvitationState();
			break;

		case TState::Connected:
			ConnectedState();
			break;
		}

		// Allow other tasks to run
		pScheduler->Yield();
	}
}

void CAppleMIDIParticipant::ControlInvitationState()
{
	TAppleMIDISession SessionPacket;

	if (m_nControlResult == 0)
		return;

	if (!ParseInvitationPacket(m_ControlBuffer, m_nControlResult, &SessionPacket))
	{
		LOGERR("Unexpected packet");
		return;
	}

#ifdef APPLEMIDI_DEBUG
	LOGNOTE("<-- Control invitation");
#endif

	// Store initiator details
	m_InitiatorIPAddress.Set(m_ForeignControlIPAddress);
	m_nInitiatorControlPort = m_nForeignControlPort;
	m_nInitiatorToken = SessionPacket.nInitiatorToken;
	m_nInitiatorSSRC = SessionPacket.nSSRC;

	// Generate random SSRC and accept
	m_nSSRC = m_pRandom->GetNumber();
	if (!SendAcceptInvitationPacket(m_pControlSocket, &m_InitiatorIPAddress, m_nInitiatorControlPort))
	{
		LOGERR("Couldn't accept control invitation");
		return;
	}

	m_nLastSyncTime = GetSyncClock();
	m_State = TState::MIDIInvitation;
}

void CAppleMIDIParticipant::MIDIInvitationState()
{
	TAppleMIDISession SessionPacket;

	if (m_nControlResult > 0)
	{
		if (ParseInvitationPacket(m_ControlBuffer, m_nControlResult, &SessionPacket))
		{
			// Unexpected peer; reject invitation
			if (m_ForeignControlIPAddress != m_InitiatorIPAddress || m_nForeignControlPort != m_nInitiatorControlPort)
				SendRejectInvitationPacket(m_pControlSocket, &m_ForeignControlIPAddress, m_nForeignControlPort, SessionPacket.nInitiatorToken);
			else
				LOGERR("Unexpected packet");
		}
	}

	if (m_nMIDIResult > 0)
	{
		if (!ParseInvitationPacket(m_MIDIBuffer, m_nMIDIResult, &SessionPacket))
		{
			LOGERR("Unexpected packet");
			return;
		}

		// Unexpected peer; reject invitation
		if (m_ForeignMIDIIPAddress != m_InitiatorIPAddress)
		{
			SendRejectInvitationPacket(m_pMIDISocket, &m_ForeignMIDIIPAddress, m_nForeignMIDIPort, SessionPacket.nInitiatorToken);
			return;
		}

#ifdef APPLEMIDI_DEBUG
		LOGNOTE("<-- MIDI invitation");
#endif

		m_nInitiatorMIDIPort = m_nForeignMIDIPort;

		if (SendAcceptInvitationPacket(m_pMIDISocket, &m_InitiatorIPAddress, m_nInitiatorMIDIPort))
		{
			CString IPAddressString;
			m_InitiatorIPAddress.Format(&IPAddressString);
			LOGNOTE("Connection to %s (%s) established", SessionPacket.Name, static_cast<const char *>(IPAddressString));
			m_nLastSyncTime = GetSyncClock();
			m_State = TState::Connected;
			m_pHandler->OnAppleMIDIConnect(&m_InitiatorIPAddress, SessionPacket.Name);
		}
		else
		{
			LOGERR("Couldn't accept MIDI invitation");
			Reset();
		}
	}

	// Timeout
	else if ((GetSyncClock() - m_nLastSyncTime) > InvitationTimeout)
	{
		LOGERR("MIDI port invitation timed out");
		Reset();
	}
}

void CAppleMIDIParticipant::ConnectedState()
{
	TAppleMIDISession SessionPacket;
	TRTPMIDI MIDIPacket;
	TAppleMIDISync SyncPacket;

	if (m_nControlResult > 0)
	{
		if (ParseEndSessionPacket(m_ControlBuffer, m_nControlResult, &SessionPacket))
		{
#ifdef APPLEMIDI_DEBUG
			LOGNOTE("<-- End session");
#endif

			if (m_ForeignControlIPAddress == m_InitiatorIPAddress &&
			    m_nForeignControlPort == m_nInitiatorControlPort &&
			    SessionPacket.nSSRC == m_nInitiatorSSRC)
			{
				LOGNOTE("Initiator ended session");
				m_pHandler->OnAppleMIDIDisconnect(&m_InitiatorIPAddress, SessionPacket.Name);
				Reset();
				return;
			}
		}
		else if (ParseInvitationPacket(m_ControlBuffer, m_nControlResult, &SessionPacket))
		{
			// Unexpected peer; reject invitation
			if (m_ForeignControlIPAddress != m_InitiatorIPAddress || m_nForeignControlPort != m_nInitiatorControlPort)
				SendRejectInvitationPacket(m_pControlSocket, &m_ForeignControlIPAddress, m_nForeignControlPort, SessionPacket.nInitiatorToken);
			else
				LOGERR("Unexpected packet");
		}
	}

	if (m_nMIDIResult > 0)
	{
		if (m_ForeignMIDIIPAddress != m_InitiatorIPAddress || m_nForeignMIDIPort != m_nInitiatorMIDIPort)
			LOGERR("Unexpected packet");
		else if (ParseMIDIPacket(m_MIDIBuffer, m_nMIDIResult, &MIDIPacket, m_pHandler))
			m_nSequence = MIDIPacket.nSequence;
		else if (ParseSyncPacket(m_MIDIBuffer, m_nMIDIResult, &SyncPacket))
		{
#ifdef APPLEMIDI_DEBUG
			LOGNOTE("<-- Sync %d", SyncPacket.nCount);
#endif

			if (SyncPacket.nSSRC == m_nInitiatorSSRC && (SyncPacket.nCount == 0 || SyncPacket.nCount == 2))
			{
				if (SyncPacket.nCount == 0)
					SendSyncPacket(SyncPacket.Timestamps[0], GetSyncClock());
				else if (SyncPacket.nCount == 2)
				{
					m_nOffsetEstimate = ((SyncPacket.Timestamps[2] + SyncPacket.Timestamps[0]) / 2) - SyncPacket.Timestamps[1];
#ifdef APPLEMIDI_DEBUG
					LOGNOTE("Offset estimate: %llu", m_nOffsetEstimate);
#endif
				}

				m_nLastSyncTime = GetSyncClock();
			}
			else
			{
				LOGERR("Unexpected sync packet");
			}
		}
	}

	const uint64_t nTicks = GetSyncClock();

	if ((nTicks - m_nLastFeedbackTime) > ReceiverFeedbackPeriod)
	{
		if (m_nSequence != m_nLastFeedbackSequence)
		{
			SendFeedbackPacket();
			m_nLastFeedbackSequence = m_nSequence;
		}
		m_nLastFeedbackTime = nTicks;
	}

	if ((nTicks - m_nLastSyncTime) > SyncTimeout)
	{
		LOGERR("Initiator timed out");
		Reset();
	}
}

void CAppleMIDIParticipant::Reset()
{
	m_State = TState::ControlInvitation;

	m_nInitiatorToken = 0;
	m_nInitiatorSSRC = 0;
	m_nSSRC = 0;
	m_nLastMIDISequenceNumber = 0;

	m_nOffsetEstimate = 0;
	m_nLastSyncTime = 0;

	m_nSequence = 0;
	m_nLastFeedbackSequence = 0;
	m_nLastFeedbackTime = 0;
}

bool CAppleMIDIParticipant::SendPacket(CSocket *pSocket, CIPAddress *pIPAddress, uint16_t nPort, const void *pData, int nSize)
{
	const int nResult = pSocket->SendTo(pData, static_cast<unsigned>(nSize), MSG_DONTWAIT, *pIPAddress, nPort);

	if (nResult < 0)
	{
		LOGERR("Send failure, error code: %d", nResult);
		return false;
	}

	if (nResult != nSize)
	{
		LOGERR("Send failure, only %d/%d bytes sent", nResult, nSize);
		return false;
	}

#ifdef APPLEMIDI_DEBUG
	LOGNOTE("Sent %d bytes to port %d", nResult, nPort);
#endif

	return true;
}

bool CAppleMIDIParticipant::SendAcceptInvitationPacket(CSocket *pSocket, CIPAddress *pIPAddress, uint16_t nPort)
{
	TAppleMIDISession AcceptPacket =
	{
		htons(AppleMIDISignature),
		htons(InvitationAccepted),
		htonl(AppleMIDIVersion),
		htonl(m_nInitiatorToken),
		htonl(m_nSSRC),
		{'\0'},
	};

	// Use hostname as the session name
	if (m_pSessionName && m_pSessionName[0])
		strncpy(AcceptPacket.Name, m_pSessionName, sizeof(AcceptPacket.Name));
	else
		strncpy(AcceptPacket.Name, "MiniDexed", sizeof(AcceptPacket.Name));

	AcceptPacket.Name[sizeof(AcceptPacket.Name) - 1] = 0;

#ifdef APPLEMIDI_DEBUG
	LOGNOTE("--> Accept invitation");
#endif

	const int nSendSize = NamelessSessionPacketSize + sstrlen(AcceptPacket.Name) + 1;
	return SendPacket(pSocket, pIPAddress, nPort, &AcceptPacket, nSendSize);
}

bool CAppleMIDIParticipant::SendRejectInvitationPacket(CSocket *pSocket, CIPAddress *pIPAddress, uint16_t nPort, uint32_t nInitiatorToken)
{
	TAppleMIDISession RejectPacket =
	{
		htons(AppleMIDISignature),
		htons(InvitationRejected),
		htonl(AppleMIDIVersion),
		htonl(nInitiatorToken),
		htonl(m_nSSRC),
		{'\0'},
	};

#ifdef APPLEMIDI_DEBUG
	LOGNOTE("--> Reject invitation");
#endif

	// Send without name
	return SendPacket(pSocket, pIPAddress, nPort, &RejectPacket, NamelessSessionPacketSize);
}

bool CAppleMIDIParticipant::SendSyncPacket(uint64_t nTimestamp1, uint64_t nTimestamp2)
{
	const TAppleMIDISync SyncPacket =
	{
		htons(AppleMIDISignature),
		htons(Sync),
		htonl(m_nSSRC),
		1,
		{0},
		{
			htonll(nTimestamp1),
			htonll(nTimestamp2),
			0,
		},
	};

#ifdef APPLEMIDI_DEBUG
	LOGNOTE("--> Sync 1");
#endif

	return SendPacket(m_pMIDISocket, &m_InitiatorIPAddress, m_nInitiatorMIDIPort, &SyncPacket, sizeof(SyncPacket));
}

bool CAppleMIDIParticipant::SendFeedbackPacket()
{
	const TAppleMIDIReceiverFeedback FeedbackPacket =
	{
		htons(AppleMIDISignature),
		htons(ReceiverFeedback),
		htonl(m_nSSRC),
		htonl(static_cast<uint32_t>(m_nSequence) << 16),
	};

#ifdef APPLEMIDI_DEBUG
	LOGNOTE("--> Feedback");
#endif

	return SendPacket(m_pControlSocket, &m_InitiatorIPAddress, m_nInitiatorControlPort, &FeedbackPacket, sizeof(FeedbackPacket));
}

bool CAppleMIDIParticipant::SendMIDIToHost(const uint8_t *pData, int nSize)
{
	if (m_State != TState::Connected)
		return false;

	// Build RTP-MIDI packet
	TRTPMIDI packet;
	packet.nFlags = htons((RTPMIDIVersion << 14) | RTPMIDIPayloadType);
	packet.nSequence = htons(++m_nSequence);
	packet.nTimestamp = htonl(0); // No timestamping for now
	packet.nSSRC = htonl(m_nSSRC);

	// RTP-MIDI command section: header + MIDI data
	// Header: 0x80 | length (if length < 0x0F)
	uint8_t midiHeader = 0x00;
	int midiLen = nSize;
	if (midiLen < 0x0F)
	{
		midiHeader = midiLen & 0x0F;
	}
	else
	{
		midiHeader = 0x80 | ((midiLen >> 8) & 0x0F);
	}

	uint8_t buffer[sizeof(TRTPMIDI) + 2 + MAX_MIDI_MESSAGE];
	int offset = 0;
	memcpy(buffer + offset, &packet, sizeof(TRTPMIDI));
	offset += sizeof(TRTPMIDI);
	buffer[offset++] = midiHeader;
	if (midiLen >= 0x0F)
	{
		buffer[offset++] = midiLen & 0xFF;
	}
	memcpy(buffer + offset, pData, static_cast<size_t>(midiLen));
	offset += midiLen;

	if (SendPacket(m_pMIDISocket, &m_InitiatorIPAddress, m_nInitiatorMIDIPort, buffer, offset) <= 0)
	{
		LOGNOTE("Failed to send MIDI data to host");
		return false;
	}

#ifdef APPLEMIDI_DEBUG
	LOGDBG("Successfully sent %u bytes of MIDI data", nSize);
#endif
	return true;
}
