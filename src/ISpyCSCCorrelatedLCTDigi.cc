#include "ISpy/Analyzers/interface/ISpyCSCCorrelatedLCTDigi.h"
#include "ISpy/Analyzers/interface/ISpyService.h"
#include "ISpy/Services/interface/IgCollection.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "FWCore/Utilities/interface/Exception.h"

#include "Geometry/CSCGeometry/interface/CSCLayer.h"
#include "Geometry/CSCGeometry/interface/CSCLayerGeometry.h"

#include "DataFormats/MuonDetId/interface/CSCDetId.h"

#include <iostream>

using namespace edm::service;
using namespace edm;

ISpyCSCCorrelatedLCTDigi::ISpyCSCCorrelatedLCTDigi(const edm::ParameterSet& iConfig)
  : inputTag_(iConfig.getParameter<edm::InputTag>("iSpyCSCCorrelatedLCTDigiTag"))
{
  digiToken_ = consumes<CSCCorrelatedLCTDigiCollection>(inputTag_);
  cscGeometryToken_ = esConsumes<CSCGeometry, MuonGeometryRecord>();
}

void ISpyCSCCorrelatedLCTDigi::analyze(const edm::Event& event, const edm::EventSetup& eventSetup)
{
  edm::Service<ISpyService> config;

  if ( ! config.isAvailable() )
  {
    throw cms::Exception ("Configuration")
      << "ISpyCSCCorrelatedLCTDigi requires the ISpyService\n"
      "which is not present in the configuration file.\n"
      "You must add the service in the configuration file\n"
      "or remove the module that requires it";
  }

  IgDataStorage *storage = config->storage();

  cscGeometry_ = &eventSetup.getData(cscGeometryToken_);

  if ( ! cscGeometry_ )
  {
    std::string error = 
      "### Error: ISpyCSCCorrelatedLCTDigi::analyze: Invalid MuonGeometryRecord ";
    config->error (error);
    return;
  }

  edm::Handle<CSCCorrelatedLCTDigiCollection> collection;
  event.getByToken(digiToken_, collection);

  if(collection.isValid())
  {
    std::string product = "CSCCorrelatedLCTDigis "
			  + edm::TypeID (typeid (CSCCorrelatedLCTDigiCollection)).friendlyClassName() + ":"
			  + inputTag_.label() + ":"
			  + inputTag_.instance() + ":"
			  + inputTag_.process();

    IgCollection& products = storage->getCollection("Products_V1");
    IgProperty PROD = products.addProperty("Product", std::string ());
    IgCollectionItem item = products.create();
    item[PROD] = product;

    IgCollection& digis = storage->getCollection("CSCCorrelatedLCTDigis_V2");

    IgProperty POS1 = digis.addProperty("pos1", IgV3d());
    IgProperty POS2 = digis.addProperty("pos2", IgV3d());
    IgProperty POS3 = digis.addProperty("pos3", IgV3d());
    IgProperty POS4 = digis.addProperty("pos4", IgV3d());

    IgProperty DETID = digis.addProperty("detid", int(0));
    IgProperty EC = digis.addProperty("endcap", int(0));
    IgProperty ST = digis.addProperty("station", int(0));
    IgProperty RG = digis.addProperty("ring", int(0));
    IgProperty CH = digis.addProperty("chamber", int(0));
        
    for(CSCCorrelatedLCTDigiCollection::DigiRangeIterator dri = collection->begin(), driEnd = collection->end();
	dri != driEnd; ++dri )
    {
      const CSCDetId& cscDetId = (*dri).first;

      const CSCCorrelatedLCTDigiCollection::Range& range = (*dri).second;

      for ( CSCCorrelatedLCTDigiCollection::const_iterator dit = range.first;
	        dit != range.second; ++dit)
      {      
	IgCollectionItem digi = digis.create();

        // T. Cox
        // 1) We can only identify ME1/1A or B once we know half-strip range 
        // 2) Since xOfStrip requires int strip we cannot use it for half-strip ids in the trigger primitive,
        // so jump through (my own) hoops in order to get a precise local x for a half-strip value

	int halfstrip = dit->getStrip(); // BEWARE: LCT counts from 0
        short iring = cscDetId.ring(); // for ME1/1A will need to reset from 1 to 4

        GlobalPoint gST, gSB, gW1, gW2;

        bool me11a = false;
        bool me11 = (cscDetId.station()==1) && (iring==1); // in ME1/1?
        if ( me11 ) {
          me11a = (halfstrip > 127 ); // 0-127 <-> 64 channels of ME1/1B; after that  48 channels of ME1/1A
          if ( me11a ) {
            iring = 4;
            const CSCDetId id1 = CSCDetId( cscDetId.endcap(), cscDetId.station(), iring, cscDetId.chamber(), 3 ); // layer 3 id
            const CSCLayer* layer1 = cscGeometry_->layer( id1 );
            const CSCLayerGeometry* layerGeom1 = layer1->geometry();

            int hs1 = halfstrip - 128; // reset halfstrip from 128... to 0...
            float fstrip = float( hs1 + 0.5 )/2.; // 0->0.25, 1->0.75, 2->1.25, ... 159->79.75
            LocalPoint lST = layerGeom1->topology()->localPosition( MeasurementPoint( fstrip, 0.5 ) );
            LocalPoint lSB = layerGeom1->topology()->localPosition( MeasurementPoint( fstrip, -0.5 ) );
            gST = layer1->toGlobal( lST );
            gSB = layer1->toGlobal( lSB );

            float WGCenter = layerGeom1->middleWireOfGroup( dit->getKeyWG() + 1 ); // BEWARE: LCT counts from 0
            std::pair< LocalPoint, LocalPoint > wP = layerGeom1->wireTopology()->wireEnds( WGCenter );
            gW1 = layer1->toGlobal( wP.first );
            gW2 = layer1->toGlobal( wP.second );

          }
        }
        if ( ! me11a ) {
          const CSCLayer* layer = cscGeometry_->chamber( cscDetId )->layer( 3 );
          const CSCLayerGeometry* layerGeom = layer->geometry();

          float fstrip = float( halfstrip + 0.5 )/2.; // 0->0.25, 1->0.75, 2->1.25, ... 159->79.75
          LocalPoint lST = layerGeom->topology()->localPosition( MeasurementPoint( fstrip, 0.5 ) );
          LocalPoint lSB = layerGeom->topology()->localPosition( MeasurementPoint( fstrip, -0.5 ) );
          gST = layer->toGlobal( lST );
          gSB = layer->toGlobal( lSB );

          float WGCenter = layerGeom->middleWireOfGroup( dit->getKeyWG() + 1 ); // BEWARE: LCT counts from 0
          std::pair< LocalPoint, LocalPoint > wP = layerGeom->wireTopology()->wireEnds( WGCenter );
          gW1 = layer->toGlobal( wP.first );
          gW2 = layer->toGlobal( wP.second );

	} 
 
        digi[POS1] = IgV3d(static_cast<double>(gST.x()/100.0),
                           static_cast<double>(gST.y()/100.0),
                           static_cast<double>(gST.z()/100.0));
        digi[POS2] = IgV3d(static_cast<double>(gSB.x()/100.0),
                           static_cast<double>(gSB.y()/100.0),
                           static_cast<double>(gSB.z()/100.0));

        digi[POS3] = IgV3d(static_cast<double>(gW1.x()/100.0),
                           static_cast<double>(gW1.y()/100.0),
                           static_cast<double>(gW1.z()/100.0));
        digi[POS4] = IgV3d(static_cast<double>(gW2.x()/100.0),
                           static_cast<double>(gW2.y()/100.0),
                           static_cast<double>(gW2.z()/100.0));

        digi[DETID] = static_cast<int>(cscDetId.rawId());
        digi[EC] = static_cast<int>(cscDetId.endcap());
        digi[ST] = static_cast<int>(cscDetId.station());
        digi[RG] = static_cast<int>( iring );
        digi[CH] = static_cast<int>(cscDetId.chamber());
      }
    }
  }
  else
  {
    std::string error = "### Error: CSCLCTDigis "
			+ edm::TypeID (typeid (CSCCorrelatedLCTDigiCollection)).friendlyClassName() + ":"
			+ inputTag_.label() + ":"
			+ inputTag_.instance() + ":"
			+ inputTag_.process() + " are not found.";
    config->error (error);
  }
}

DEFINE_FWK_MODULE(ISpyCSCCorrelatedLCTDigi);
