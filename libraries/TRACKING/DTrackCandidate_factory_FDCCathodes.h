// $Id$
//
//    File: DTrackCandidate_factory_FDCCathodes.h
// Created: Tue Nov  6 13:37:08 EST 2007
// Creator: staylor (on Linux ifarml1.jlab.org 2.4.21-47.0.1.ELsmp i686)
//

#ifndef _DTrackCandidate_factory_FDCCathodes_
#define _DTrackCandidate_factory_FDCCathodes_

#include <JANA/JFactory.h>
#include "DTrackCandidate.h"
#include <DMatrix.h>
#include "FDC/DFDCSegment_factory.h"
#include "HDGEOMETRY/DMagneticFieldMap.h"
#include <TH1F.h>

class DTrackCandidate_factory_FDCCathodes:public JFactory<DTrackCandidate>{
	public:
		DTrackCandidate_factory_FDCCathodes(){
		  DEBUG_HISTS = false;
		};
		~DTrackCandidate_factory_FDCCathodes(){};
		const string toString(void);
		const char* Tag(void){return "FDCCathodes";}
	
	private:  
		const DMagneticFieldMap *bfield;

		//jerror_t init(void);						///< Called once at program start.
		jerror_t brun(JEventLoop *eventLoop, int runnumber);	///< Called everytime a new run number is detected.
		jerror_t evnt(JEventLoop *eventLoop, int eventnumber);	///< Called every event.
		//jerror_t erun(void);						///< Called everytime run number changes, provided brun has been called.
		//jerror_t fini(void);						///< Called after last event of last event source has been processed.

		jerror_t GetPositionAndMomentum(DFDCSegment *segment,
						DVector3 &pos, DVector3 &mom);
		
                DFDCSegment *GetTrackMatch(double q,double z,
                                           DFDCSegment *segment,
                                           vector<DFDCSegment*>package,
                                           unsigned int &match_id);
	bool DEBUG_HISTS;
	TH1F *match_dist;

};

#endif // _DTrackCandidate_factory_FDCCathodes_

