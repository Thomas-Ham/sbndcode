////////////////////////////////////////////////////////////////////////
// Class:       ForwardFilter
// Module Type: filter
// File:        ForwardFilter_module.cc
//
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDFilter.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include <memory>

#include "larsim/MCCheater/ParticleInventoryService.h"
#include "nusimdata/SimulationBase/MCTruth.h"
#include "canvas/Persistency/Common/Ptr.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"

namespace filt {

  class ForwardFilter : public art::EDFilter {
    public:
      explicit ForwardFilter(fhicl::ParameterSet const & p);
      // The destructor generated by the compiler is fine for classes
      // without bare pointers or other resource use.

      // Plugins should not be copied or assigned.
      ForwardFilter(ForwardFilter const &) = delete;
      ForwardFilter(ForwardFilter &&) = delete;
      ForwardFilter & operator = (ForwardFilter const &) = delete;
      ForwardFilter & operator = (ForwardFilter &&) = delete;

      // Required functions.
      bool filter(art::Event & e) override;


    private:

      // Declare member data here.
      std::string fGenieGenModuleLabel;

  };


  ForwardFilter::ForwardFilter(fhicl::ParameterSet const & pset): art::EDFilter{pset}
  // Initialize member data here.
  {
    fGenieGenModuleLabel = pset.get<std::string>("GenieGenModuleLabel");
  }

  bool ForwardFilter::filter(art::Event & evt)
  {
    const art::ServiceHandle<cheat::ParticleInventoryService> particleInventory;

    //List the particles in the event
    const sim::ParticleList& particles = particleInventory->ParticleList();

    for (sim::ParticleList::const_iterator particleIt = particles.begin(); particleIt != particles.end(); ++particleIt){
      const simb::MCParticle *particle = particleIt->second;

      //Only keep primary
      if(particle->Mother() == 0){
        if (particle->Pz()/particle->P() < 0.5){
          return false;
        }
      }
    }
    return true;
  }


  DEFINE_ART_MODULE(ForwardFilter)
}
