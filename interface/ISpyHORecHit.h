#ifndef ANALYZER_ISPY_HOREC_HIT_H
#define ANALYZER_ISPY_HOREC_HIT_H

#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Utilities/interface/InputTag.h"

#include "DataFormats/HcalRecHit/interface/HcalRecHitCollections.h"
#include "Geometry/CaloGeometry/interface/CaloGeometry.h"
#include "Geometry/Records/interface/CaloGeometryRecord.h"

class ISpyHORecHit : public edm::one::EDAnalyzer<>
{
public:
  explicit ISpyHORecHit(const edm::ParameterSet&);
  virtual ~ISpyHORecHit(void) {}
       
  virtual void analyze(const edm::Event&, const edm::EventSetup&);
private:
  edm::InputTag	inputTag_;
  edm::EDGetTokenT<HORecHitCollection> rechitToken_;
  edm::ESGetToken<CaloGeometry, CaloGeometryRecord> caloGeometryToken_;
  const CaloGeometry* caloGeometry_;

};

#endif // ANALYZER_ISPY_HOREC_HIT_H
