// $Id$
//
//    File: JEventProcessor_ST_Eff.cc
//

#include "JEventProcessor_ST_Eff.h"

// Routine used to create our JEventProcessor
extern "C"
{
	void InitPlugin(JApplication *locApplication)
	{
		InitJANAPlugin(locApplication);
		locApplication->AddProcessor(new JEventProcessor_ST_Eff()); //register this plugin
	}
} // "C"

//define static local variable //declared in header file
thread_local DTreeFillData JEventProcessor_ST_Eff::dTreeFillData;

//------------------
// init
//------------------
jerror_t JEventProcessor_ST_Eff::init(void)
{
	//TRACK REQUIREMENTS
	dMinPIDFOM = 5.73303E-7; //+/- 5 sigma
	dMinTrackingFOM = 5.73303E-7; // +/- 5 sigma
	dMinNumTrackHits = 14; //e.g. 6 in CDC, 8 in 
	dMinHitRingsPerCDCSuperlayer = 3;
	dMinHitPlanesPerFDCPackage = 4;
	dCutAction_TrackHitPattern = new DCutAction_TrackHitPattern(NULL, dMinHitRingsPerCDCSuperlayer, dMinHitPlanesPerFDCPackage);
	//action initialize not necessary: is empty

	locHistMaxDeltaPhi = 12.0; //width of a paddle

	TDirectory* locOriginalDir = gDirectory;
	gDirectory->mkdir("ST_Eff")->cd();

	//Histograms
	dHist_HitFound = new TH2I("HitFound", "Hit Found;Projected Hit-Z (cm);Sector", 140, 30.0, 100.0, 30, 0.5, 30.5);
	dHist_HitTotal = new TH2I("HitTotal", "Hit Total;Projected Hit-Z (cm);Sector", 140, 30.0, 100.0, 30, 0.5, 30.5);
	
	// back to original dir
	locOriginalDir->cd();

	//TTREE INTERFACE
	//MUST DELETE WHEN FINISHED: OR ELSE DATA WON'T BE SAVED!!!
	dTreeInterface = DTreeInterface::Create_DTreeInterface("ST_Eff", "tree_ST_Eff.root");

	//TTREE BRANCHES
	DTreeBranchRegister locTreeBranchRegister;

	//TRACK
	locTreeBranchRegister.Register_Single<Int_t>("PID_PDG"); //gives charge, mass, beta
	locTreeBranchRegister.Register_Single<Float_t>("TimingBeta");
	locTreeBranchRegister.Register_Single<Float_t>("TrackVertexZ");
	locTreeBranchRegister.Register_Single<TVector3>("TrackP3");
	locTreeBranchRegister.Register_Single<UInt_t>("TrackCDCRings"); //rings correspond to bits (1 -> 28)
	locTreeBranchRegister.Register_Single<UInt_t>("TrackFDCPlanes"); //planes correspond to bits (1 -> 24)

	//SC
	locTreeBranchRegister.Register_Single<UChar_t>("NumSCHits"); //may want to ignore event if too many (especially for tracks in nose)
	locTreeBranchRegister.Register_Single<Float_t>("ProjectedSCHitPhi"); //degrees
	locTreeBranchRegister.Register_Single<Float_t>("ProjectedSCHitZ");

	//SEARCH
	locTreeBranchRegister.Register_Single<UChar_t>("ProjectedSCHitSector");
	locTreeBranchRegister.Register_Single<UChar_t>("NearestSCHitSector"); //0 if none
	locTreeBranchRegister.Register_Single<Float_t>("TrackHitDeltaPhi"); //is signed: SC - Track

	//REGISTER BRANCHES
	dTreeInterface->Create_Branches(locTreeBranchRegister);

	return NOERROR;
}


//------------------
// brun
//------------------
jerror_t JEventProcessor_ST_Eff::brun(jana::JEventLoop* locEventLoop, int locRunNumber)
{
	// This is called whenever the run number changes

	return NOERROR;
}

//------------------
// evnt
//------------------

jerror_t JEventProcessor_ST_Eff::evnt(jana::JEventLoop* locEventLoop, uint64_t locEventNumber)
{
	// This is called for every event. Use of common resources like writing
	// to a file or filling a histogram should be mutex protected. Using
	// locEventLoop->Get(...) to get reconstructed objects (and thereby activating the
	// reconstruction algorithm) should be done outside of any mutex lock
	// since multiple threads may call this method at the same time.

	// This plugin is used to determine the reconstruction efficiency of hits in the BCAL
		// Note, this is hit-level, not shower-level.  Hits: By sector/layer/module/end

	//CUT ON TRIGGER TYPE
	const DTrigger* locTrigger = NULL;
	locEventLoop->GetSingle(locTrigger);
	if(locTrigger->Get_L1FrontPanelTriggerBits() != 0)
		return NOERROR;

	//SEE IF SC REQUIRED TO TRIGGER
	uint32_t locTriggerBits = locTrigger->Get_L1TriggerBits();
	if(locTriggerBits >= 32)
		return NOERROR;

	vector<const DChargedTrack*> locChargedTracks;
	locEventLoop->Get(locChargedTracks);

	vector<const DSCHit*> locSCHits;
	locEventLoop->Get(locSCHits);

	const DParticleID* locParticleID = NULL;
	locEventLoop->GetSingle(locParticleID);

	const DDetectorMatches* locDetectorMatches = NULL;
	locEventLoop->GetSingle(locDetectorMatches);

	//Try to select the most-pure sample of tracks possible
	//select the best DTrackTimeBased for each track: of tracks with good hit pattern, use best PID confidence level (need accurate beta)
		//also, must have min tracking FOM, min PID confidence level, min #-hits on track, and matching hit in SC
	set<const DChargedTrackHypothesis*> locBestTracks; //lowest tracking FOM for each candidate id
	for(size_t loc_i = 0; loc_i < locChargedTracks.size(); ++loc_i)
	{
		const DChargedTrackHypothesis* locChargedTrackHypothesis = locChargedTracks[loc_i]->Get_BestFOM();
		if(locChargedTrackHypothesis->dFOM < dMinPIDFOM)
			continue; //don't trust PID

		const DTrackTimeBased* locTrackTimeBased = NULL;
		locChargedTrackHypothesis->GetSingle(locTrackTimeBased);
		if(locTrackTimeBased->FOM < dMinTrackingFOM)
			continue;

		if(!locDetectorMatches->Get_IsMatchedToDetector(locTrackTimeBased, SYS_TOF) && !locDetectorMatches->Get_IsMatchedToDetector(locTrackTimeBased, SYS_BCAL))
			continue; //not matched to either TOF or BCAL

		if(!dCutAction_TrackHitPattern->Cut_TrackHitPattern(locParticleID, locTrackTimeBased))
			continue;

		unsigned int locNumTrackHits = locTrackTimeBased->Ndof + 5;
		if(locNumTrackHits < dMinNumTrackHits)
			continue;

		locBestTracks.insert(locChargedTrackHypothesis);
	}

	//for histograms, keep running total of what to fill
	//will fill at end: only one lock
	vector<pair<int, double> > locHitMap_HitFound, locHitMap_HitTotal; //int: sector, double: projected-z

	// Loop over the good tracks, using the best DTrackTimeBased object for each
	for(auto& locChargedTrackHypothesis : locBestTracks)
	{
		const DTrackTimeBased* locTrackTimeBased = NULL;
		locChargedTrackHypothesis->GetSingle(locTrackTimeBased);

		//Predict ST Surface Hit Location
		DVector3 locPredictedSurfacePosition;
		bool locProjBarrelRegion = false;
		unsigned int locPredictedSCSector = locParticleID->PredictSCSector(locTrackTimeBased->rt, 999.0, &locPredictedSurfacePosition, &locProjBarrelRegion);
		if(locPredictedSCSector == 0)
			continue; //don't expect it to hit at all 

		pair<int, double> locHitPair(locPredictedSCSector, locPredictedSurfacePosition.Z());
		locHitMap_HitTotal.push_back(locHitPair);

		//Find closest SC hit
		const DSCHit* locBestSCHit = NULL;
		double locBestDeltaPhi = 7.0;
		for(auto& locSCHit : locSCHits)
		{
			double locDeltaPhi = 0.0;
			if(!locParticleID->Distance_ToTrack(locSCHit, locTrackTimeBased->rt, locTrackTimeBased->t0(), locDeltaPhi))
				continue;
			if(fabs(locDeltaPhi) >= fabs(locBestDeltaPhi))
				continue;
			locBestDeltaPhi = locDeltaPhi;
			locBestSCHit = locSCHit;
		}
		int locBestSCHitSector = (locBestSCHit != NULL) ? locBestSCHit->sector : 0;

		//Fill hit hist
		if(fabs(locBestDeltaPhi) <= locHistMaxDeltaPhi)
			locHitMap_HitFound.push_back(locHitPair);

		//TRACK
		dTreeFillData.Fill_Single<Int_t>("PID_PDG", PDGtype(locChargedTrackHypothesis->PID()));
		dTreeFillData.Fill_Single<Float_t>("TimingBeta", locChargedTrackHypothesis->measuredBeta());
		dTreeFillData.Fill_Single<Float_t>("TrackVertexZ", locChargedTrackHypothesis->position().Z());
		dTreeFillData.Fill_Single<UInt_t>("TrackCDCRings", locTrackTimeBased->dCDCRings);
		dTreeFillData.Fill_Single<UInt_t>("TrackFDCPlanes", locTrackTimeBased->dFDCPlanes);
		DVector3 locDP3 = locChargedTrackHypothesis->momentum();
		TVector3 locP3(locDP3.X(), locDP3.Y(), locDP3.Z());
		dTreeFillData.Fill_Single<TVector3>("TrackP3", locP3);

		//SC
		dTreeFillData.Fill_Single<UChar_t>("NumSCHits", locSCHits.size());
		dTreeFillData.Fill_Single<Float_t>("ProjectedSCHitPhi", locPredictedSurfacePosition.Phi()*180.0/TMath::Pi());
		dTreeFillData.Fill_Single<Float_t>("ProjectedSCHitZ", locPredictedSurfacePosition.Z());

		//SEARCH
		dTreeFillData.Fill_Single<UChar_t>("ProjectedSCHitSector", locPredictedSCSector);
		dTreeFillData.Fill_Single<UChar_t>("NearestSCHitSector", locBestSCHitSector);
		dTreeFillData.Fill_Single<Float_t>("TrackHitDeltaPhi", locBestDeltaPhi); //is signed: SC - Track

		//FILL TTREE
		dTreeInterface->Fill(dTreeFillData);
	}

	// FILL HISTOGRAMS
	// Since we are filling histograms local to this plugin, it will not interfere with other ROOT operations: can use plugin-wide ROOT fill lock
	japp->RootFillLock(this); //ACQUIRE ROOT FILL LOCK
	{
		//Fill Found
		for(auto& locHitPair : locHitMap_HitFound)
			dHist_HitFound->Fill(locHitPair.second, locHitPair.first);

		//Fill Total
		for(auto& locHitPair : locHitMap_HitTotal)
			dHist_HitTotal->Fill(locHitPair.second, locHitPair.first);
	}
	japp->RootFillUnLock(this); //RELEASE ROOT FILL LOCK

	return NOERROR;
}

//------------------
// erun
//------------------
jerror_t JEventProcessor_ST_Eff::erun(void)
{
	// This is called whenever the run number changes, before it is
	// changed to give you a chance to clean up before processing
	// events from the next run number.

	return NOERROR;
}

//------------------
// fini
//------------------
jerror_t JEventProcessor_ST_Eff::fini(void)
{
	// Called before program exit after event processing is finished.  

	delete dCutAction_TrackHitPattern;
	delete dTreeInterface; //saves trees to file, closes file

	return NOERROR;
}

