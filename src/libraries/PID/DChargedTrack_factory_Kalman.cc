// $Id$
//
//    File: DChargedTrack_factory_Kalman.cc
// Created: Mon Dec  7 14:29:12 EST 2009
// Creator: staylor (on Linux ifarml6 2.6.18-128.el5 x86_64)
//
// This is a copy of the DChargedTrack factory hard-wired to use the time-based
// tracks from the Kalman Filter.

#include <iostream>
#include <iomanip>
using namespace std;

#include "DChargedTrack_factory_Kalman.h"
using namespace jana;

bool DChargedTrack_track_cmp(const DTrackTimeBased *a,const DTrackTimeBased *b){
  return (a->FOM>b->FOM);
}


//------------------
// init
//------------------
jerror_t DChargedTrack_factory_Kalman::init(void)
{
	return NOERROR;
}

//------------------
// brun
//------------------
jerror_t DChargedTrack_factory_Kalman::brun(jana::JEventLoop *eventLoop, int runnumber)
{
	return NOERROR;
}

//------------------
// evnt
//------------------
jerror_t DChargedTrack_factory_Kalman::evnt(JEventLoop *loop, int eventnumber)
{
  // Get tracks 
  vector<const DTrackTimeBased*> tracks;
  loop->Get(tracks,"Kalman");
  if (tracks.size()==0) return RESOURCE_UNAVAILABLE;

  // loop over the tracks, sorting them according to figure-of-merit and 
  // grouping them by trackid
  JObject::oid_t old_id=tracks[0]->candidateid;
  vector<const DTrackTimeBased *>hypotheses;
  for (unsigned int i=0;i<tracks.size();i++){
    const DTrackTimeBased *track=tracks[i];

    if (old_id != track->candidateid){
      // Create a new DChargedTrack object
      DChargedTrack *charged_track = new DChargedTrack;

      // Sort the hypothesis list according to figure-of-merit
      sort(hypotheses.begin(),hypotheses.end(),DChargedTrack_track_cmp);
      charged_track->hypotheses=hypotheses;

      // Add to the data vector
      _data.push_back(charged_track);

      // Clear the hypothesis list for the next track
      hypotheses.clear();
    }
    hypotheses.push_back(track);
    old_id=track->candidateid;
  }
  // Final set
  DChargedTrack *charged_track = new DChargedTrack;
  
  // Sort the hypothesis list according to figure-of-merit
  sort(hypotheses.begin(),hypotheses.end(),DChargedTrack_track_cmp);
  charged_track->hypotheses=hypotheses;
  
  // Add to the data vector
  _data.push_back(charged_track);

  return NOERROR;
}

//------------------
// erun
//------------------
jerror_t DChargedTrack_factory_Kalman::erun(void)
{
	return NOERROR;
}

//------------------
// fini
//------------------
jerror_t DChargedTrack_factory_Kalman::fini(void)
{
	return NOERROR;
}

