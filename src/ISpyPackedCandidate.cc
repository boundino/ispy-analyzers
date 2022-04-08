#include "ISpy/Analyzers/interface/ISpyPackedCandidate.h"
#include "ISpy/Analyzers/interface/ISpyService.h"
#include "ISpy/Analyzers/interface/ISpyVector.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "FWCore/Utilities/interface/Exception.h"

#include "ISpy/Services/interface/IgCollection.h"

#include "MagneticField/Engine/interface/MagneticField.h"

#include "TrackingTools/GeomPropagators/interface/Propagator.h"
#include "TrackingTools/TrajectoryParametrization/interface/GlobalTrajectoryParameters.h"
#include "TrackingTools/TrajectoryState/interface/FreeTrajectoryState.h"
#include "TrackingTools/TrajectoryState/interface/TrajectoryStateOnSurface.h"

#include "TrackingTools/TrackAssociator/interface/DetIdAssociator.h"
#include "TrackingTools/Records/interface/DetIdAssociatorRecord.h"

#include "TrackPropagation/SteppingHelixPropagator/interface/SteppingHelixPropagator.h"

using namespace edm::service;
using namespace edm;

#include <iostream>

ISpyPackedCandidate::ISpyPackedCandidate(const ParameterSet& iConfig)
: inputTag_(iConfig.getParameter<InputTag>("iSpyPackedCandidateTag"))
{
  candidateToken_ = consumes<pat::PackedCandidateCollection>(inputTag_);
  magneticFieldToken_ = esConsumes<MagneticField, IdealMagneticFieldRecord>();
}

void ISpyPackedCandidate::analyze(const Event& event, const EventSetup& eventSetup)
{
  Service<ISpyService> config;

  if ( ! config.isAvailable() )
  {
    throw cms::Exception ("Configuration")
      << "ISpyPackedCandidate requires the ISpyService\n"
     "which is not present in the configuration file.\n"
     "You must add the service in the configuration file\n"
     "or remove the module that requires it";
  }

  IgDataStorage *storage = config->storage();

  Handle<pat::PackedCandidateCollection> collection;
  event.getByToken(candidateToken_, collection);

  magneticField_ = &eventSetup.getData(magneticFieldToken_);

  if ( ! magneticField_ )
  {
    std::string error = 
      "### Error: ISpyMuon::analyze: Invalid Magnetic field ";
    
    config->error (error);
    return;
  }
  
  SteppingHelixPropagator propagator(magneticField_, alongMomentum);
 
  if ( collection.isValid() )
  {
    std::string product = "PackedCandidates "
                          + TypeID (typeid (pat::PackedCandidateCollection)).friendlyClassName() + ":"
                          + inputTag_.label() + ":"
                          + inputTag_.instance() + ":"
                          + inputTag_.process();

    IgCollection& products = storage->getCollection("Products_V1");
    IgProperty PROD = products.addProperty("Product", std::string ());
    IgCollectionItem item = products.create();
    item[PROD] = product;

    IgCollection &tracks = storage->getCollection("Tracks_V4");
    IgProperty VTX = tracks.addProperty("pos", IgV3d());
    IgProperty P   = tracks.addProperty("dir", IgV3d());
    IgProperty PT  = tracks.addProperty("pt", 0.0); 
    IgProperty PHI = tracks.addProperty("phi", 0.0);
    IgProperty ETA = tracks.addProperty("eta", 0.0);
    IgProperty CHARGE = tracks.addProperty("charge", int(0));
    IgProperty CHI2 = tracks.addProperty("chi2", 0.0);
    IgProperty NDOF = tracks.addProperty("ndof", 0.0);

    IgCollection &extras = storage->getCollection("Extras_V1");
    IgProperty IPOS = extras.addProperty("pos_1", IgV3d());
    IgProperty IP   = extras.addProperty("dir_1", IgV3d());
    IgProperty OPOS = extras.addProperty("pos_2", IgV3d());
    IgProperty OP   = extras.addProperty("dir_2", IgV3d());
    IgAssociations &trackExtras = storage->getAssociations("TrackExtras_V1");

    for ( pat::PackedCandidateCollection::const_iterator c = collection->begin(); 
          c != collection->end(); ++c )
    {

      if ( ! (*c).hasTrackDetails() )
        continue;

      IgCollectionItem track = tracks.create();

      track[VTX] = IgV3d((*c).vx()/100.,
                         (*c).vy()/100.,
                         (*c).vz()/100.);

      IgV3d dir = IgV3d((*c).px(),
                        (*c).py(),
                        (*c).pz());

      ISpyVector::normalize(dir);
      track[P] = dir;

      track[PT] = (*c).pt();
      track[ETA] = (*c).eta();
      track[PHI] = (*c).phi();
      track[CHARGE] = (*c).charge();
      track[CHI2] = (*c).vertexChi2();
      track[NDOF] = (*c).vertexNdof();

      IgCollectionItem eitem = extras.create();

      GlobalPoint trackP((*c).vx(), (*c).vy(), (*c).vz());
      GlobalVector trackM((*c).px(), (*c).py(), (*c).pz());  

      GlobalTrajectoryParameters trackParams(trackP, trackM, (*c).charge(), magneticField_);
      FreeTrajectoryState trackState(trackParams);

      // NOTE: Ideally would get this from FiducicalVolume
      // but required record isn't available for some reason.
      // Investigate.
      double minR = 1.24*100.; 
      double minZ = 3.0*100.; 

      TrajectoryStateOnSurface tsos = propagator.propagate(
        trackState, *Cylinder::build(minR, Surface::PositionType(0,0,0), Surface::RotationType())
        );
      
      // If out the endcaps then repropagate a la TrackExtrapolator
      if ( tsos.isValid() && tsos.globalPosition().z() > minZ ) 
      {
        tsos = propagator.propagate(trackState, *Plane::build(Surface::PositionType(0, 0, minZ), Surface::RotationType()));
      }   
      else if ( tsos.isValid() && tsos.globalPosition().z() < -minZ ) 
      {
        tsos = propagator.propagate(trackState, *Plane::build(Surface::PositionType(0, 0, -minZ), Surface::RotationType()));
      }
      
      if ( tsos.isValid() ) 
      {         
        eitem[IPOS] = IgV3d((*c).vx()/100.,
                            (*c).vy()/100.,
                            (*c).vz()/100.);
        eitem[IP] = dir;
      

        eitem[OPOS] = IgV3d(tsos.globalPosition().x()/100., 
                            tsos.globalPosition().y()/100., 
                            tsos.globalPosition().z()/100.);

        IgV3d odir = IgV3d(tsos.globalMomentum().x(),
                           tsos.globalMomentum().y(),
                           tsos.globalMomentum().z());

        ISpyVector::normalize(odir);
        eitem[OP] = odir;      

      }          

      trackExtras.associate(track, eitem);

    }
  }

  else
  {
    std::string error = "### Error: PackedCandidates "
                        + TypeID (typeid (pat::PackedCandidateCollection)).friendlyClassName() + ":"
                        + inputTag_.label() + ":"
                        + inputTag_.instance() + " are not found";
    config->error(error);
  }
}

DEFINE_FWK_MODULE(ISpyPackedCandidate);
