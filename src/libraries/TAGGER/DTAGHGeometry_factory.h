//
// File: DTAGHGeometry_factory.h
// Created: Sat Jul 5 10:09:27 EST 2014
// Creator: jonesrt on gluey.phys.uconn.edu
//

#ifndef _DTAGHGeometry_factory_
#define _DTAGHGeometry_factory_

#include <string>

#include <JANA/JFactory.h>
#include <TAGGER/DTAGHGeometry.h>

class DTAGHGeometry_factory : public JFactory<DTAGHGeometry> {
	public:
		DTAGHGeometry_factory(){}
		~DTAGHGeometry_factory(){}
	
		DTAGHGeometry *taghgeometry;

		//------------------
		// brun
		//------------------
		jerror_t brun(JEventLoop *loop, int32_t runnumber)
		{
			// The NOT_OBJECT_OWNER flag tells JANA not to automatically
			// delete the objects in _data. This allows the same geometry
			// object to persist through multiple events.
			//
			// n.b. There is another, similar factory flag "PERSISTANT"
			// that will also prevent the object from being deleted. That,
			// however, will also prevent the brun and evnt methods from
			// being called. Here, we want the brun method to be called since
			// the geometry actually may change with run number. (This
			// happened for Lubomir's J/Psi analysis where the tiny number
			// of events spread over many runs were concatenated in a single
			// job.)
			SetFactoryFlag(NOT_OBJECT_OWNER);
			ClearFactoryFlag(WRITE_TO_OUTPUT);
			
			if( taghgeometry ) delete taghgeometry;

			taghgeometry = new DTAGHGeometry(loop);

			return NOERROR;
		}

		//------------------
		// evnt
		//------------------
		 jerror_t evnt(JEventLoop *loop, uint64_t eventnumber)
		 {
			// Reuse existing DBCALGeometry object.
			if( taghgeometry ) _data.push_back( taghgeometry );
			 
			 return NOERROR;
		 }

		//------------------
		// erun
		//------------------
		jerror_t erun(void)
		{
			if( taghgeometry ) delete taghgeometry;
			taghgeometry = NULL;
			
			return NOERROR;
		}
};

#endif // _DTAGHGeometry_factory_
