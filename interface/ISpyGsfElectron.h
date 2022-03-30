#ifndef ANALYZER_ISPY_GSFELECTRON_H
#define ANALYZER_ISPY_GSFELECTRON_H

#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "DataFormats/EgammaCandidates/interface/GsfElectronFwd.h"

class ISpyGsfElectron : public edm::one::EDAnalyzer<>
{
public:
  explicit ISpyGsfElectron(const edm::ParameterSet&);
  virtual ~ISpyGsfElectron(){}
  
  virtual void analyze(const edm::Event&, const edm::EventSetup&);
private:
  edm::InputTag inputTag_;
  edm::EDGetTokenT<reco::GsfElectronCollection> electronToken_;
};

#endif // ANALYZER_ISPY_GSFELECTRON_H
