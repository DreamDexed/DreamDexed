//
// mdnspublisher.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2024  R. Stange <rsta2@o2online.de>
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

#include <circle/net/ipaddress.h>
#include <circle/net/netsubsystem.h>
#include <circle/net/socket.h>
#include <circle/ptrlist.h>
#include <circle/sched/mutex.h>
#include <circle/sched/synchronizationevent.h>
#include <circle/sched/task.h>
#include <circle/string.h>

class CmDNSPublisher : public CTask /// mDNS / Bonjour client task
{
public:
	static constexpr const char *ServiceTypeAppleMIDI = "_apple-midi._udp";
	static constexpr const char *ServiceTypeFTP = "_ftp._tcp";

public:
	/// \param pNet Pointer to the network subsystem object
	CmDNSPublisher(CNetSubSystem *pNet);
	~CmDNSPublisher();
	/// \brief Start publishing a service
	/// \param pServiceName Name of the service to be published
	/// \param pServiceType Type of the service to be published (e.g. ServiceTypeAppleMIDI)
	/// \param usServicePort Port number of the service to be published (in host byte order)
	/// \param ppText Descriptions of the service (terminated with a nullptr, or nullptr itself)
	/// \return Operation successful?
	bool PublishService(const char *pServiceName,
			    const char *pServiceType,
			    uint16_t usServicePort,
			    const char *ppText[] = nullptr);
	/// \brief Stop publishing a service
	/// \param pServiceName Name of the service to be unpublished (same as when published)
	/// \return Operation successful?
	bool UnpublishService(const char *pServiceName);
	/// \brief Stop publishing a service
	/// \param pServiceName Name of the service to be unpublished
	/// \param pServiceType Type of the service to be unpublished
	/// \param usServicePort Port number of the service to be unpublished
	/// \return Operation successful?
	bool UnpublishService(const char *pServiceName, const char *pServiceType, uint16_t usServicePort);
	void Run() override;

private:
	static constexpr int MaxTextRecords = 10;
	static constexpr int MaxMessageSize = 1400; // safe UDP payload in an Ethernet frame
	static constexpr int TTLShort = 15; // seconds
	static constexpr int TTLLong = 4500;
	static constexpr int TTLDelete = 0;
	struct TService
	{
		CString ServiceName;
		CString ServiceType;
		uint16_t usServicePort;
		int nTextRecords;
		CString *ppText[MaxTextRecords];
	};
	bool SendResponse(TService *pService, bool bDelete);
	// Helpers for writing to buffer
	void PutByte(uint8_t uchValue);
	void PutWord(uint16_t usValue);
	void PutDWord(uint32_t nValue);
	void PutString(const char *pValue);
	void PutCompressedString(const uint8_t *pWritePtr);
	void PutDNSName(const char *pValue);
	void PutIPAddress(const CIPAddress &rValue);
	void ReserveDataLength();
	void SetDataLength();

private:
	CNetSubSystem *m_pNet;
	CPtrList m_ServiceList;
	CMutex m_Mutex;
	CSocket *m_pSocket;
	bool m_bRunning;
	CSynchronizationEvent m_Event;
	uint8_t m_Buffer[MaxMessageSize];
	uint8_t *m_pWritePtr;
	uint8_t *m_pDataLen;
};
