#include "ISpy/Analyzers/interface/ISpySiStripCluster.h"
#include "ISpy/Analyzers/interface/ISpyService.h"
#include "ISpy/Services/interface/IgCollection.h"

//#include "DataFormats/Common/interface/DetSetVectorNew.h"
#include "DataFormats/SiStripCluster/interface/SiStripCluster.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "FWCore/Utilities/interface/Exception.h"

#include "Geometry/CommonDetUnit/interface/GeomDet.h"
#include "Geometry/CommonTopologies/interface/StripTopology.h"
#include "Geometry/TrackerGeometryBuilder/interface/StripGeomDetUnit.h"
#include "Geometry/TrackerGeometryBuilder/interface/TrackerGeometry.h"
#include "Geometry/Records/interface/TrackerDigiGeometryRecord.h"

using namespace edm::service;

ISpySiStripCluster::ISpySiStripCluster (const edm::ParameterSet& iConfig)
  : inputTag_ (iConfig.getParameter<edm::InputTag>("iSpySiStripClusterTag"))
{
  clusterToken_ = consumes<edm::DetSetVector<SiStripCluster> >(inputTag_);
  trackingGeometryToken_ = esConsumes<TrackingGeometry, TrackerDigiGeometryRecord>();
}

void 
ISpySiStripCluster::analyze (const edm::Event& event, const edm::EventSetup& eventSetup)
{
  edm::Service<ISpyService> config;

  if (! config.isAvailable ()) 
  {
    throw cms::Exception ("Configuration")
      << "ISpySiStripCluster requires the ISpyService\n"
      "which is not present in the configuration file.\n"
      "You must add the service in the configuration file\n"
      "or remove the module that requires it";
  }

  IgDataStorage *storage = config->storage();

  trackingGeometry_ = &eventSetup.getData(trackingGeometryToken_);
    
  if ( ! trackingGeometry_ )
  {
    std::string error = 
      "### Error: ISpySiStripCluster::analyze: Invalid TrackerDigiGeometryRecord ";
    config->error (error);
    return;
  }

  edm::Handle<edm::DetSetVector<SiStripCluster> > collection;
  event.getByToken(clusterToken_, collection);

  if (collection.isValid ())
  {	    
    std::string product = "SiStripClusters "
			  + edm::TypeID (typeid (edm::DetSetVector<SiStripCluster>)).friendlyClassName () + ":" 
			  + inputTag_.label() + ":"
			  + inputTag_.instance() + ":" 
			  + inputTag_.process();

    IgCollection& products = storage->getCollection("Products_V1");
    IgProperty PROD = products.addProperty("Product", std::string ());
    IgCollectionItem item = products.create();
    item[PROD] = product;

    IgCollection &clusters = storage->getCollection ("SiStripClusters_V1");
    IgProperty DET_ID   = clusters.addProperty ("detid", int (0)); 
    IgProperty POS 	    = clusters.addProperty ("pos", IgV3d());

    edm::DetSetVector<SiStripCluster>::const_iterator it = collection->begin ();
    edm::DetSetVector<SiStripCluster>::const_iterator end = collection->end ();

    for (; it != end; ++it)
    {
      //edm::DetSet<SiStripCluster> ds = *it;
      const uint32_t detID = it->detId ();
      DetId detid (detID);

      const StripGeomDetUnit* theDet = dynamic_cast<const StripGeomDetUnit *>(trackingGeometry_->idToDet (detid));
      const StripTopology* theTopol = dynamic_cast<const StripTopology *>( &(theDet->specificTopology ()));

      edm::DetSet<SiStripCluster>::const_iterator icluster = it->begin ();
      edm::DetSet<SiStripCluster>::const_iterator iclusterEnd = it->end ();

      for(; icluster != iclusterEnd; ++icluster)
      { 
	short firststrip = (*icluster).firstStrip ();
	GlobalPoint pos =  (trackingGeometry_->idToDet (detid))->surface().toGlobal (theTopol->localPosition (firststrip));
	IgCollectionItem item = clusters.create ();
	item[DET_ID] = static_cast<int> (detID);
	item[POS] = IgV3d(static_cast<double>(pos.x()/100.0), static_cast<double>(pos.y()/100.0), static_cast<double>(pos.z()/100.0));
      }
    }
  }
  else 
  {
    std::string error = "### Error: SiStripClusters "
			+ edm::TypeID (typeid (edm::DetSetVector<SiStripCluster>)).friendlyClassName () + ":" 
			+ inputTag_.label() + ":"
			+ inputTag_.instance() + ":" 
			+ inputTag_.process() + " are not found.";
    config->error (error);
  }
}

DEFINE_FWK_MODULE(ISpySiStripCluster);
