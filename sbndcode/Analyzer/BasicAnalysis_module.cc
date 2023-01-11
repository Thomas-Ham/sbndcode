//Standard
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "canvas/Utilities/InputTag.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

//Sim Base
#include "nusimdata/SimulationBase/MCParticle.h"
#include "nusimdata/SimulationBase/MCTruth.h"

//Tools
#include "canvas/Persistency/Common/FindManyP.h"
#include "canvas/Persistency/Common/FindOneP.h"

//LArSoft
#include "larsim/Utils/TruthMatchUtils.h"
#include "lardataobj/Simulation/SimEnergyDeposit.h"

//Root
#include "art_root_io/TFileService.h"
#include "TTree.h"

//SBNDCODE
#include "sbndcode/Geometry/GeometryWrappers/TPCGeoAlg.h"

constexpr int def_int     = -999;
constexpr float def_float = -999.0f;

class BasicAnalysis;

class BasicAnalysis : public art::EDAnalyzer {
public:
  explicit BasicAnalysis(fhicl::ParameterSet const& pset);

  BasicAnalysis(BasicAnalysis const&) = delete;
  BasicAnalysis(BasicAnalysis&&) = delete;
  BasicAnalysis& operator=(BasicAnalysis const&) = delete;
  BasicAnalysis& operator=(BasicAnalysis&&) = delete;

  void analyze(art::Event const &e) override;

private:

  void SimDepositProcessor(art::Event const &e);
  void TruthProcessor(art::Event const &e);

  std::string fLArGeantModuleLabel, fSimEnergyDepositsModuleLabel, fSimEnergyDepositsInstanceLabel;
};


BasicAnalysis::BasicAnalysis(fhicl::ParameterSet const &pset)
  : EDAnalyzer{pset},
  fLArGeantModuleLabel (pset.get<std::string>("LArGeantModuleLabel")),
  fSimEnergyDepositsModuleLabel (pset.get<std::string>("SimEnergyDepositsModuleLabel")),
  fSimEnergyDepositsInstanceLabel (pset.get<std::string>("SimEnergyDepositsInstanceLabel"))
  {
  }


void BasicAnalysis::SimDepositProcessor(art::Event const &e)
{
  art::Handle<std::vector<sim::SimEnergyDeposit> > handleSEDs;
  e.getByLabel(fSimEnergyDepositsModuleLabel,fSimEnergyDepositsInstanceLabel,handleSEDs);

  std::map<int,int> depositsMap;

  for(unsigned SED_i = 0; SED_i < handleSEDs->size(); ++SED_i) {
    const art::Ptr<sim::SimEnergyDeposit> SED(handleSEDs,SED_i);
    depositsMap[SED->TrackID()]++;
  }

  std::cout << "\n===== Sim Deposits =====" << std::endl;

  for(auto const& [id, deposits] : depositsMap) std::cout << "Track ID: " << id << " with " << deposits << " deposits" << std::endl;
}

void BasicAnalysis::TruthProcessor(art::Event const &e)
{
  art::Handle<std::vector<simb::MCParticle> > handleParticles;
  e.getByLabel(fLArGeantModuleLabel,handleParticles);

  std::cout << "\n===== MCParticles =====" << std::endl;

  for(unsigned int nu_i = 0; nu_i < handleParticles->size(); ++nu_i){
    const art::Ptr<simb::MCParticle> particle(handleParticles,nu_i);

    std::cout << "Particle " << particle->TrackId() << '\n'
	      << "PDG: " << particle->PdgCode() << '\n'
	      << "Mother: " << particle->Mother() << '\n' << std::endl;

  }
}



void BasicAnalysis::analyze(art::Event const &e)
{
  SimDepositProcessor(e);
  TruthProcessor(e);
}

DEFINE_ART_MODULE(BasicAnalysis)
