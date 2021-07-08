////////////////////////////////////////////////////////////////////////
// Class:       GenieAnalyzer
// Module Type: analyzer
// File:        GenieAnalyzer_module.cc
//
// Generated at Fri Jun 25 19:32:19 2021 by Ivan Lepetic using artmod
// from cetpkgsupport v1_14_01.
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "fhiclcpp/ParameterSet.h"
#include "art_root_io/TFileService.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include "canvas/Persistency/Common/Ptr.h"
#include "canvas/Persistency/Common/PtrVector.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"

#include "larcore/Geometry/Geometry.h"
#include "larcorealg/Geometry/GeometryCore.h"

#include "TTree.h"
#include "nusimdata/SimulationBase/MCParticle.h"
#include "nusimdata/SimulationBase/MCTruth.h"
#include "nusimdata/SimulationBase/GTruth.h"
#include "nusimdata/SimulationBase/MCFlux.h"

class GenieAnalyzer;

class GenieAnalyzer : public art::EDAnalyzer {
public:
  explicit GenieAnalyzer(fhicl::ParameterSet const & p);
  // The destructor generated by the compiler is fine for classes
  // without bare pointers or other resource use.

  // Plugins should not be copied or assigned.
  GenieAnalyzer(GenieAnalyzer const &) = delete;
  GenieAnalyzer(GenieAnalyzer &&) = delete;
  GenieAnalyzer & operator = (GenieAnalyzer const &) = delete;
  GenieAnalyzer & operator = (GenieAnalyzer &&) = delete;

  // Required functions.
  void analyze(art::Event const & e) override;

  // Selected optional functions.
  void beginJob() override;
  void beginRun(art::Run const & r) override;
  void beginSubRun(art::SubRun const & sr) override;
  void endJob() override;
  void endRun(art::Run const & r) override;
  void endSubRun(art::SubRun const & sr) override;
  void reconfigure(fhicl::ParameterSet const & p);
  void respondToCloseInputFile(art::FileBlock const & fb) override;
  void respondToCloseOutputFiles(art::FileBlock const & fb) override;
  void respondToOpenInputFile(art::FileBlock const & fb) override;
  void respondToOpenOutputFiles(art::FileBlock const & fb) override;

private:

  // Declare member data here.

  std::string m_neutrinoproducer;


  TTree *fEventTree;

  std::vector<Int_t> nuPDG_truth;
  std::vector<double> enu_truth;
  std::vector<double> nuvtxx_truth;
  std::vector<double> nuvtxy_truth;
  std::vector<double> nuvtxz_truth;
  std::vector<double> nupx_truth;
  std::vector<double> nupy_truth;
  std::vector<double> nupz_truth;


};


GenieAnalyzer::GenieAnalyzer(fhicl::ParameterSet const & p)
  :
  EDAnalyzer(p)  // ,
 // More initializers here.
{

  art::ServiceHandle<art::TFileService> tfs;
      this->reconfigure(p);

      fEventTree = tfs->make<TTree>("Event", "Event Tree");
      fEventTree->Branch("PDG", "std::vector<Int_t>", &nuPDG_truth);
      fEventTree->Branch("Energy", "std::vector<double>", &enu_truth);
      fEventTree->Branch("VertexX", "std::vector<double>", &nuvtxx_truth);
      fEventTree->Branch("VertexY", "std::vector<double>", &nuvtxy_truth);
      fEventTree->Branch("VertexZ", "std::vector<double>", &nuvtxz_truth);
      fEventTree->Branch("Px", "std::vector<double>", &nupx_truth);
      fEventTree->Branch("Py", "std::vector<double>", &nupy_truth);
      fEventTree->Branch("Pz", "std::vector<double>", &nupz_truth);



}

void GenieAnalyzer::analyze(art::Event const & e)
{
  // Implementation of required member function here.

  nuPDG_truth.clear();
  enu_truth.clear();
  nuvtxx_truth.clear();
  nuvtxy_truth.clear();
  nuvtxz_truth.clear();
  nupx_truth.clear();
  nupy_truth.clear();
  nupz_truth.clear();

  art::Handle< std::vector<simb::MCTruth> > mctruthListHandle;
   std::vector<art::Ptr<simb::MCTruth> > mclist;
   e.getByLabel(m_neutrinoproducer,mctruthListHandle);
     art::fill_ptr_vector(mclist, mctruthListHandle);

   std::cout << "Neutrino producer: " << m_neutrinoproducer << std::endl;

     for (unsigned int i_mctruth = 0; i_mctruth < mclist.size(); i_mctruth++){
             //fetch an mctruth
             art::Ptr<simb::MCTruth> curr_mctruth = mclist[i_mctruth];
             //Check if it's a neutrino
             if (!curr_mctruth->NeutrinoSet()) continue;

            nuPDG_truth.push_back(curr_mctruth->GetNeutrino().Nu().PdgCode());
            enu_truth.push_back(curr_mctruth->GetNeutrino().Nu().E());
            nuvtxx_truth.push_back(curr_mctruth->GetNeutrino().Nu().Vx());
            nuvtxy_truth.push_back(curr_mctruth->GetNeutrino().Nu().Vy());
            nuvtxz_truth.push_back(curr_mctruth->GetNeutrino().Nu().Vz());
            nupx_truth.push_back(curr_mctruth->GetNeutrino().Nu().Px());
            nupy_truth.push_back(curr_mctruth->GetNeutrino().Nu().Py());
            nupz_truth.push_back(curr_mctruth->GetNeutrino().Nu().Pz());
      }

      fEventTree->Fill();

}

void GenieAnalyzer::beginJob()
{
  // Implementation of optional member function here.
}

void GenieAnalyzer::beginRun(art::Run const & r)
{
  // Implementation of optional member function here.
}

void GenieAnalyzer::beginSubRun(art::SubRun const & sr)
{
  // Implementation of optional member function here.
}

void GenieAnalyzer::endJob()
{
  // Implementation of optional member function here.
}

void GenieAnalyzer::endRun(art::Run const & r)
{
  // Implementation of optional member function here.
}

void GenieAnalyzer::endSubRun(art::SubRun const & sr)
{
  // Implementation of optional member function here.
}

void GenieAnalyzer::reconfigure(fhicl::ParameterSet const & p)
{
    m_neutrinoproducer = p.get<std::string>("neutrinoproducer", "generator");
  // Implementation of optional member function here.
}

void GenieAnalyzer::respondToCloseInputFile(art::FileBlock const & fb)
{
  // Implementation of optional member function here.
}

void GenieAnalyzer::respondToCloseOutputFiles(art::FileBlock const & fb)
{
  // Implementation of optional member function here.
}

void GenieAnalyzer::respondToOpenInputFile(art::FileBlock const & fb)
{
  // Implementation of optional member function here.
}

void GenieAnalyzer::respondToOpenOutputFiles(art::FileBlock const & fb)
{
  // Implementation of optional member function here.
}

DEFINE_ART_MODULE(GenieAnalyzer)
