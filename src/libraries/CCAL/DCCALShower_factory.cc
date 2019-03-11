/*
 *  File: DCCALHit_factory.cc
 *
 * Created on 11/25/18 by A.S.
 * use structure similar to FCAL
 */

#include <iostream>
#include <fstream>
#include <math.h>
#include <DVector3.h>
#include <mutex>
using namespace std;

#include <JANA/JEvent.h>
#include <JANA/JCalibration.h>
#include <JANA/JResourceManager.h>
using namespace jana;

#include "DIRC/DDIRCLutReader.h"
#include "CCAL/DCCALShower.h"
#include "CCAL/DCCALShower_factory.h"
#include "CCAL/DCCALHit.h"
#include "CCAL/DCCALGeometry.h"
#include "HDGEOMETRY/DGeometry.h"

#include "hycal.h"

static mutex CCAL_MUTEX;

//------------------
// LoadCCALProfileData
//------------------
bool DCCALShower_factory::LoadCCALProfileData(JApplication *japp, int32_t runnumber)
{
	static bool first = true;
	
	// we are just setting some global stuff inside island.F so it's OK
	if(!first) {
		return true;
	} else {
		first = false;
	}

	string ccal_profile_file;
	gPARMS->SetDefaultParameter("CCAL_PROFILE_FILE", ccal_profile_file, "CCAL profile data file name");
		
	// make sure we have a pointer to the JApplication
	if(japp == nullptr)
		return false;
	
	// follow similar procedure as other resources (DMagneticFieldMapFineMesh)
	map<string,string> profile_file_name;
	JCalibration *jcalib = japp->GetJCalibration(runnumber);
	if(jcalib->GetCalib("/CCAL/profile_data/profile_data_map", profile_file_name)) 
		jout << "Can't find requested /CCAL/profile_data/profile_data_map in CCDB for this run!" << endl;
	else if(profile_file_name.find("map_name") != profile_file_name.end() 
				&& profile_file_name["map_name"] != "None") {
		JResourceManager *jresman = japp->GetJResourceManager(runnumber);
		ccal_profile_file = jresman->GetResource(profile_file_name["map_name"]);
	}
	
	jout<<"Reading CCAL profile data from "<<ccal_profile_file<<" ..."<<endl;
	
	// check to see if we actually have a file
	if(ccal_profile_file.empty()) {
		jerr << "Empty file..." << endl;
		return false;
	}
	
	
	char fname_buf[1000];
	sprintf(fname_buf,"%s",ccal_profile_file.c_str());
	int ccal_profile_file_size = (int)ccal_profile_file.size();
	init_island_(fname_buf, &ccal_profile_file_size);

/*
	ifstream ccal_profile(ccal_profile_file.c_str());
	for(int i=0; i<=500; i++) {
		for(int j=0; j<i; j++) {
			int id1, id2;
			float fcell_hyc, fd2c;
			
			ccal_profile >> id1 >> id2 >> fcell_hyc >> fd2c;
		
			//cout << id1 << " " << id2 << " " << fcell_hyc << " " << fd2c << endl;
		
			acell[0][i][j] = fcell_hyc;
			acell[0][j][i] = fcell_hyc;
			ad2c[0][i][j] = fd2c;
			ad2c[0][j][i] = fd2c;
		}
	}
	
	// copy the results to the FORTRAN routines
	init_island_(acell, ad2c);
	
	// cleanup
	ccal_profile.close();	
*/
	
	return true;
}


//----------------
// Constructor
//----------------
DCCALShower_factory::DCCALShower_factory()
{
	// Set defaults
    	MIN_CLUSTER_BLOCK_COUNT   =  2;
    	MIN_CLUSTER_SEED_ENERGY   =  0.035;  // GeV
	MIN_CLUSTER_ENERGY        =  0.05;   // GeV
	MAX_CLUSTER_ENERGY        =  15.9;   // GeV
	TIME_CUT                  =  15.0 ;  // ns
	MAX_HITS_FOR_CLUSTERING   =  80;
	SHOWER_DEBUG              =  0;
	ALLOW_SINGLE_HIT_CLUSTERS =  0;

	gPARMS->SetDefaultParameter("CCAL:SHOWER_DEBUG",   SHOWER_DEBUG);
	gPARMS->SetDefaultParameter("CCAL:MIN_CLUSTER_BLOCK_COUNT", MIN_CLUSTER_BLOCK_COUNT);
	gPARMS->SetDefaultParameter("CCAL:MIN_CLUSTER_SEED_ENERGY", MIN_CLUSTER_SEED_ENERGY);
	gPARMS->SetDefaultParameter("CCAL:MIN_CLUSTER_ENERGY", MIN_CLUSTER_ENERGY);
	gPARMS->SetDefaultParameter("CCAL:MAX_CLUSTER_ENERGY", MAX_CLUSTER_ENERGY);
	gPARMS->SetDefaultParameter("CCAL:MAX_HITS_FOR_CLUSTERING", MAX_HITS_FOR_CLUSTERING);
	gPARMS->SetDefaultParameter("CCAL:TIME_CUT",TIME_CUT,"time cut for associating CCAL hits together into a cluster");
	gPARMS->SetDefaultParameter("CCAL:ALLOW_SINGLE_HIT_CLUSTERS", ALLOW_SINGLE_HIT_CLUSTERS);

	pthread_mutex_init(&mutex, NULL);

}

//------------------
// brun
//------------------
jerror_t DCCALShower_factory::brun(JEventLoop *eventLoop, int32_t runnumber)
{

	DApplication *dapp = dynamic_cast<DApplication*>(eventLoop->GetJApplication());
    	const DGeometry *geom = dapp->GetDGeometry(runnumber);
	
	vector<double> CCALpos;
	geom->Get("//composition[@name='ComptonEMcal']/posXYZ[@volume='comptonEMcal']/@X_Y_Z",CCALpos);
    
    	if (geom) {
      	  geom->GetTargetZ(m_zTarget);
      	  geom->GetCCALZ(m_CCALfront);
    	}
    	else{
      	  cerr << "No geometry accessbile." << endl;
      	  return RESOURCE_UNAVAILABLE;
    	}
	
	// accessing global variables again!
	//pthread_mutex_lock(&mutex);
	std::lock_guard<std::mutex> lck(CCAL_MUTEX);

	LoadCCALProfileData(eventLoop->GetJApplication(), runnumber);

	/*
	for(int icol = 1; icol <= MCOL; ++icol) {
	  for(int irow = 1; irow <= MROW; ++irow) {
	
		int id = (12-icol) + (12-irow)*12;
		DVector2 pos = ccalGeom.positionOnFace(12-irow,12-icol);
	
		blockINFO[id].id     = id;
		blockINFO[id].x      = pos.X();
		blockINFO[id].y      = pos.Y();
		blockINFO[id].sector = 0;
		blockINFO[id].row    = 12-irow;
		blockINFO[id].col    = 12-icol;
	  }
	}
	*/

	// Release the lock	
	pthread_mutex_unlock(&mutex);

	return NOERROR;
}


//------------------
// evnt
//------------------
jerror_t DCCALShower_factory::evnt(JEventLoop *eventLoop, uint64_t eventnumber)
{

	// extract the CCAL Geometry
	vector<const DCCALGeometry*> ccalGeomVect;
    	eventLoop->Get( ccalGeomVect );
    	if (ccalGeomVect.size() < 1)
      	  return OBJECT_NOT_AVAILABLE;
    	const DCCALGeometry& ccalGeom = *(ccalGeomVect[0]);


	vector< ccalcluster_t > ccalcluster;
	vector< cluster_t > cluster_storage;
	vector< const DCCALHit* > hit_storage;
	
	int n_h_clusters = 0;
	
	// for now, use center of target as vertex
	DVector3 vertex(0.0, 0.0, m_zTarget);
	
    	vector<const DCCALHit*> ccalhits;
	eventLoop->Get(ccalhits);
	int n_h_hits = (int)ccalhits.size();
	
	if(ccalhits.size() > MAX_HITS_FOR_CLUSTERING) return NOERROR;
	
	// if a single module has multiple hits in same event, one will overwrite the 
	// other in the cluster pattern. Maybe timing cut can be applied to remove 
	// out of time hits, but for now just use hit with larger energy.
	
	for(int ihit = 0; ihit < n_h_hits; ihit++) {
	  const DCCALHit *ccalhit = ccalhits[ihit];
	  //if(ccalhit->t > 20.) continue;
	  int id12 = (ccalhit->row)*12 + ccalhit->column;
	  int findVal = IsIDinVec( hit_storage, id12 );
	  if( findVal >= 0 ) {
	    if( ccalhit->E > hit_storage[findVal]->E ) {
	      hit_storage.erase( hit_storage.begin()+findVal );
	      hit_storage.push_back( ccalhit );
	    }
	  } else {
	    hit_storage.push_back( ccalhit );
	  }
	}
	n_h_hits = (int)hit_storage.size();
	
	
	// The Fortran code below uses common blocks, so we need to set a lock
	// so that different threads are not running on top of each other
	//japp->WriteLock("CCALShower_factory");
	//pthread_mutex_lock(&mutex);
	std::unique_lock<std::mutex> lck(CCAL_MUTEX);
	
	SET_EMIN   =  0.05;    // banks->CONFIG->config->CLUSTER_ENERGY_MIN;
	SET_EMAX   =  15.9;    // banks->CONFIG->config->CLUSTER_ENERGY_MAX;
	SET_HMIN   =  2;       // banks->CONFIG->config->CLUSTER_MIN_HITS_NUMBER;
	SET_MINM   =  0.08;    // banks->CONFIG->config->CLUSTER_MAX_CELL_MIN_ENERGY;
	ZHYCAL     =  1.e9;    // ALignment[1].al.z;
	ISECT      =  0;       //  PWO

	static int ich[MCOL][MROW]; 
	
	for(int icol = 1; icol <= MCOL; ++icol) {
	  for(int irow = 1; irow <= MROW; ++irow) {
	    ECH(icol,irow)     = 0;
	    STAT_CH(icol,irow) = 0;
	    if(icol>=6 && icol<=7 && irow>=6 && irow<=7) { STAT_CH(icol,irow) = -1; }	
	    
	    ich[icol-1][irow-1] = (12-icol)+(12-irow)*12;
	        
	  }
	}
	
	NCOL = 12; NROW = 12;
	SET_XSIZE = CRYS_SIZE_X; SET_YSIZE = CRYS_SIZE_Y;
	
	for (int i = 0; i < n_h_hits; i++) {
	  const DCCALHit *ccalhit = hit_storage[i];  
	  int row   = 12-ccalhit->row;
	  int col   = 12-ccalhit->column;
	  ECH(col,row) = int(ccalhit->E*10.+0.5);
	}
	
	main_island_();
	
	int init_clusters = adcgam_cbk_.nadcgam;
	for(int k = 0; k < init_clusters; ++k)  {
	
	  cluster_t clust_storage;
	  ccalcluster_t clust;

	  // results of main_island_():
	  float e     = adcgam_cbk_.u.fadcgam[k][0];
	  float x     = adcgam_cbk_.u.fadcgam[k][1];
	  float y     = adcgam_cbk_.u.fadcgam[k][2];
	  float xc    = adcgam_cbk_.u.fadcgam[k][4];
	  float yc    = adcgam_cbk_.u.fadcgam[k][5];
	  float chi2  = adcgam_cbk_.u.fadcgam[k][6];
	  int type    = adcgam_cbk_.u.iadcgam[k][7];
	  int dime    = adcgam_cbk_.u.iadcgam[k][8];
	  int status  = adcgam_cbk_.u.iadcgam[k][10];
	  
	  if( dime < MIN_CLUSTER_BLOCK_COUNT && !(ALLOW_SINGLE_HIT_CLUSTERS) ) { continue; } 
	  if( e < MIN_CLUSTER_ENERGY || e > MAX_CLUSTER_ENERGY ) { continue; }
	  n_h_clusters += 1;
	  
	  //------------------------------------------------------------
	  //  Do energy and position corrections:
	  
	  float ecellmax = -1; int idmax = -1;
	  float sW   = 0.0;
	  float xpos = 0.0;
	  float ypos = 0.0;
	  float e1   = 0.0;
	  float e2   = 0.0;
	  float W;
	  
	  // get id of cell with max energy:
	  for(int j = 0; j < (dime>MAX_CC ? MAX_CC : dime); ++j) {
	    float ecell = 1.e-4*(float)ICL_IENER(k,j);
	    int id = ICL_INDEX(k,j);
	    int kx = (id/100), ky = id%100;
	    id = ich[kx-1][ky-1];
	    e1 += ecell;
	    if(ecell>ecellmax) {
	      ecellmax = ecell;
	      idmax = id;
	    }
	  }
	  
	  // x and y pos of max cell
	  float xmax    = ccalGeom.positionOnFace(idmax).X();
	  float ymax    = ccalGeom.positionOnFace(idmax).Y();
	  
	  int ccal_id;
	  for(int j = 0; j < (dime>MAX_CC ? MAX_CC : dime); ++j) {
	    int id = ICL_INDEX(k,j);
	    int kx = (id/100), ky = id%100;
	    ccal_id = ich[kx-1][ky-1];
	    
	    float ecell  = 1.e-4*(float)ICL_IENER(k,j);
	    float xcell  = ccalGeom.positionOnFace(ccal_id).X();
	    float ycell  = ccalGeom.positionOnFace(ccal_id).Y();
	    
	    if(type%10 == 1 || type%10 == 2) {
	      xcell += xc;
	      ycell += yc;
	    }
	    
	    int trialid;
	    float hittime = 0.0;
	    for(int ihit = 0; ihit < n_h_hits; ihit++) {
	      trialid = 12*(hit_storage[ihit]->row) + hit_storage[ihit]->column;
	      if(trialid == ccal_id) {
	        hittime = hit_storage[ihit]->t;
		break;
	      }
	    }
	    
	    clust_storage.id[j] = ccal_id;
	    clust_storage.E[j]  = ecell;
	    clust_storage.x[j]  = xcell;
	    clust_storage.y[j]  = ycell;
	    clust_storage.t[j]  = hittime;
	    
	    if(ecell>0.009 && fabs(xcell-xmax)<6. && fabs(ycell-ymax)<6.) {
	      W = 4.2 + log(ecell/e); 
	      if(W > 0) { // i.e. if cell has > 1.5% of cluster energy
		sW   += W;
		xpos += xcell*W;
		ypos += ycell*W;
		e2   += ecell;
	      }
	    }
	  }
	  
	  for(int j = dime; j < MAX_CC; ++j)  // zero the rest
	    clust_storage.id[j] = -1;
	  
	  float weightedtime = 0;
	  float totEn = 0;
	  for(int j = 0; j < (dime>MAX_CC ? MAX_CC : dime); ++j) {
	    weightedtime += clust_storage.t[j]*clust_storage.E[j];
	    totEn += clust_storage.E[j];
	  }
	  weightedtime /= totEn;
	  
	  
	  //------------------------------------------------------------
	  //  Apply correction for finite depth of hit:
	  
	  float x1, y1;
	  float zV = vertex.Z();
	  float z0 = m_CCALfront - zV;
	  if(sW) {
	    float dz;
	    dz = shower_depth(e);
	    float zk = 1. / (1. + dz/z0);
	    x1 = zk*xpos/sW;
	    y1 = zk*ypos/sW;
	  } else {
	    printf("WRN bad cluster log. coord, center id = %i %f\n", idmax, e);
	    x1 = 0.0;
	    y1 = 0.0;
	  }
	  
	  // fill cluster bank for further processing:	
	  clust.type   = type;
	  clust.nhits  = dime;
	  clust.id     = idmax;
	  clust.E      = e;
	  clust.time   = weightedtime;
	  clust.x      = x;
	  clust.y      = y;
	  clust.chi2   = chi2;
	  clust.x1     = x1;
	  clust.y1     = y1;
	  clust.emax   = ecellmax;
	  clust.status = status;
	  
	  cluster_storage.push_back(clust_storage);
	  ccalcluster.push_back(clust);
	
	}

	// Release the lock
	// japp->Unlock("CCALShower_factory");
	//pthread_mutex_unlock(&mutex);
	lck.unlock();

	if(n_h_clusters == 0) return NOERROR;
	final_cluster_processing(ccalcluster, cluster_storage, n_h_clusters); 
	
	for(int k = 0; k < n_h_clusters; ++k) {
	
	  // Build hit object
	  DCCALShower *shower = new DCCALShower;
	  
	  shower->E        =   ccalcluster[k].E;
	  shower->x        =   ccalcluster[k].x;
	  shower->y        =   ccalcluster[k].y;
	  shower->z        =   m_CCALfront;
	  shower->x1       =   ccalcluster[k].x1;
	  shower->y1       =   ccalcluster[k].y1;
	  shower->time     =   ccalcluster[k].time;
	  shower->chi2     =   ccalcluster[k].chi2;
	  shower->type     =   ccalcluster[k].type;
	  shower->dime     =   ccalcluster[k].nhits;
	  shower->status   =   ccalcluster[k].status;
	  shower->sigma_E  =   ccalcluster[k].sigma_E;
	  shower->Emax     =   ccalcluster[k].emax;
	  shower->idmax    =   ccalcluster[k].id;
	  
	  for(int icell=0; icell<ccalcluster[k].nhits; icell++) {
	    shower->id_storage[icell] = cluster_storage[k].id[icell];
	    shower->en_storage[icell] = cluster_storage[k].E[icell];
	    shower->t_storage[icell] = cluster_storage[k].t[icell];
	    
	   // if(cluster_storage[k].E[icell] > 0.)  {   // maybe redundant if-statement???
  	   // 	shower->AddAssociatedObject(ccalhits[ cluster_storage[k].id[icell] ]);
	   //	}
	  }

	  _data.push_back( shower );
	}


	return NOERROR;
	
}


//----------------------------
//   final_cluster_processing
//----------------------------
void DCCALShower_factory::final_cluster_processing( vector< ccalcluster_t > ccalcluster, vector< cluster_t > cluster_storage, int n_h_clusters ) {


	//--------------------------
	// discard bad clusters:
	/*
	for(int i = 0; i < n_h_clusters; ++i) {
	  if(ccalcluster[i].status == -1 || \
		ccalcluster[i].E       < MIN_CLUSTER_ENERGY || \
          	ccalcluster[i].E       > MAX_CLUSTER_ENERGY || \
          	ccalcluster[i].nhits   < MIN_CLUSTER_BLOCK_COUNT || \
          	ccalcluster[i].emax    < MIN_CLUSTER_SEED_ENERGY) {
	
	    ccalcluster.erase(ccalcluster.begin()+i);
	    cluster_storage.erase(cluster_storage.begin()+i);
	    n_h_clusters -= 1;
	  }
	}
	*/

	//--------------------------
	// final cluster processing (add status and energy resolution sigma_E):

    	for(int i = 0; i < n_h_clusters; ++i)  {
      	  float e   = ccalcluster[i].E;
      	  //int idmax = ccalcluster[i].id;
      	  //float x   = ccalcluster[i].x1;
      	  //float y   = ccalcluster[i].y1;

          // coord_align(i, e, idmax);
      	  int status = ccalcluster[i].status;
      	  int type   = ccalcluster[i].type;
      	  int dime   = ccalcluster[i].nhits;

      	  if(status < 0) {
            printf("error in island : cluster with negative status");
            exit(1);
          }
          int itp, ist;
          itp  = 0;
      	  if(status==1) itp += 1;

      	  for(int k = 0; k < (dime>MAX_CC ? MAX_CC : dime); ++k) {
            if(itp>=10) {itp = 12; break;}
	  }
          ccalcluster[i].type = itp;

      	  ist = type;
      	  if(status>2) ist += 20;
      	  ccalcluster[i].status = ist;

      	  double se = sqrt(0.9*0.9*e*e+2.5*2.5*e+1.0); // from HYCAL reconstruction, may need to tune
      	  se /= 100.;
      	  if(itp%10==1)
            se *= 1.5;
      	  else if(itp%10==2) {
            if(itp>10)
              se *= 0.8;
            else
              se *=1.25;
          }
      	  ccalcluster[i].sigma_E = se;
	}

	return;

}


//------------------------
// IsIDinVec
//------------------------
int IsIDinVec( vector< const DCCALHit* > hitarray, int id12 ) {

	int findVal = -1;
	for(int ii = 0; ii < (int)hitarray.size(); ii++) {
	  int id = 12*(hitarray[ii]->row) + hitarray[ii]->column;
	  if(id == id12) { findVal = ii; break; }
	}

	return findVal;
}



//------------------------
// shower_depth
//------------------------
float shower_depth(float energy) {

	float z0 = 0.86, e0 = 1.1e-3;
	float res = (energy > 0.) ? z0*log(1.+energy/e0) : 0.;
	return res;

}
