// $Id$
//
//    File: DChargedTrack_factory_Kalman.h
// Created: Mon Dec  7 14:29:12 EST 2009
// Creator: staylor (on Linux ifarml6 2.6.18-128.el5 x86_64)
//

#ifndef _DChargedTrack_factory_Kalman_
#define _DChargedTrack_factory_Kalman_

#include <JANA/JFactory.h>
#include "DChargedTrack.h"

class DChargedTrack_factory_Kalman:public jana::JFactory<DChargedTrack>{
	public:
		DChargedTrack_factory_Kalman(){};
		~DChargedTrack_factory_Kalman(){};
		const char* Tag(void){return "Kalman";}

	private:
		jerror_t init(void);						///< Called once at program start.
		jerror_t brun(jana::JEventLoop *eventLoop, int runnumber);	///< Called everytime a new run number is detected.
		jerror_t evnt(jana::JEventLoop *eventLoop, int eventnumber);	///< Called every event.
		jerror_t erun(void);						///< Called everytime run number changes, provided brun has been called.
		jerror_t fini(void);						///< Called after last event of last event source has been processed.
};

#endif // _DChargedTrack_factory_Kalman_

