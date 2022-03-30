#ifndef ANALYZER_ISPY_EBDIGI_H
#define ANALYZER_ISPY_EBDIGI_H

#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Utilities/interface/InputTag.h"

#include "DataFormats/EcalDigi/interface/EcalDigiCollections.h"
#include "DataFormats/EcalRecHit/interface/EcalRecHitCollections.h"

class ISpyEBDigi : public edm::one::EDAnalyzer<>
{
public:
  explicit ISpyEBDigi(const edm::ParameterSet&);
  virtual ~ISpyEBDigi(void){}
  virtual void analyze(const edm::Event&, const edm::EventSetup&);
private:
  edm::InputTag inputDigiTag_;
  edm::InputTag inputRecHitTag_;

  edm::EDGetTokenT<EBDigiCollection> digiToken_;
  edm::EDGetTokenT<EcalRecHitCollection> rechitToken_;
};
#endif // ANALYZER_ISPY_EBDIGI_H
