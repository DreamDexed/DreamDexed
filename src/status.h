#pragma once

#include <atomic>
#include <circle/timer.h>
#include <circle/cputhrottle.h>

class CStatus
{
public:
	CStatus (unsigned nUpdateSecs = 3):
	nCPUMaxTemp{CCPUThrottle::Get ()->GetMaxTemperature ()},
	nCPUMaxClockRate{CCPUThrottle::Get ()->GetMaxClockRate ()},
	m_nUpdateTicks{nUpdateSecs*CLOCKHZ},
	m_nLastTicks{}
	{
		assert (s_pThis == 0);
		s_pThis = this;
		assert (s_pThis != 0);
	}

	~CStatus ()
	{
		s_pThis = 0;
	}

	void Update ()
	{
		unsigned nTicks = CTimer::GetClockTicks ();

		if (nTicks - m_nLastTicks >= m_nUpdateTicks)
		{
			m_nLastTicks = nTicks;

			CCPUThrottle *pCPUT = CCPUThrottle::Get ();

			nCPUTemp = pCPUT->GetTemperature ();
			nCPUClockRate = pCPUT->GetClockRate ();
		}
	}

	static CStatus *Get ()
	{
		assert (s_pThis != 0);
		return s_pThis;
	}

	std::atomic<unsigned> nCPUTemp;
	const unsigned nCPUMaxTemp;
	std::atomic<unsigned> nCPUClockRate;
	const unsigned nCPUMaxClockRate;

private:
	unsigned m_nUpdateTicks;
	unsigned m_nLastTicks;

	static CStatus *s_pThis;
};
