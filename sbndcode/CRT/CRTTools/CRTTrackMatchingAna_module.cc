////////////////////////////////////////////////////////////////////////
// Class:       CRTTrackMatchingAna
// Module Type: analyzer
// File:        CRTTrackMatchingAnaAna_module.cc
//
// Tom Brooks (tbrooks@fnal.gov)
////////////////////////////////////////////////////////////////////////

// sbndcode includes
#include "sbndcode/RecoUtils/RecoUtils.h"
#include "sbndcode/CRT/CRTProducts/CRTTrack.hh"

// LArSoft includes
#include "lardataobj/RecoBase/Hit.h"
#include "lardataobj/RecoBase/Track.h"
#include "lardata/DetectorInfoServices/DetectorPropertiesService.h"
#include "lardata/DetectorInfoServices/DetectorClocksService.h"
#include "larcore/Geometry/Geometry.h"
#include "larcore/Geometry/AuxDetGeometry.h"
#include "lardataobj/Simulation/AuxDetSimChannel.h"
#include "larcorealg/Geometry/GeometryCore.h"
#include "larcoreobj/SimpleTypesAndConstants/geo_types.h"
#include "nusimdata/SimulationBase/MCParticle.h"
#include "larsim/Simulation/LArG4Parameters.h"
#include "larcorealg/CoreUtils/NumericUtils.h" // util::absDiff()

// Framework includes
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art/Framework/Services/Optional/TFileService.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "canvas/Persistency/Common/FindManyP.h"
#include "canvas/Utilities/Exception.h"
#include "larsim/MCCheater/BackTrackerService.h"


// Utility libraries
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "fhiclcpp/ParameterSet.h"
#include "fhiclcpp/types/Table.h"
#include "fhiclcpp/types/Atom.h"

// ROOT includes. Note: To look up the properties of the ROOT classes,
// use the ROOT web site; e.g.,
// <https://root.cern.ch/doc/master/annotated.html>
#include "TVector3.h"
#include "TH1.h"
#include "TH2.h"
#include "TCanvas.h"
#include "TPolyLine3D.h"

// C++ includes
#include <map>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

namespace {
  // Local namespace for local functions
  // Declare here, define later

}

namespace sbnd {

  struct TrackMatch{
    int tpc;
    int trueID;
    // TODO maybe change to vector
    std::vector<int> crtIDs;
    double trueTime;
  };

  struct RecoCRTTrack{
    int crtID;
    int tpc;
    TVector3 start;
    TVector3 end;
    double trueTime;
    bool complete;
  };

  struct RecoTruth{
    std::vector<simb::MCParticle> particles;
    std::vector<recob::Track> tpcTracks;
    std::vector<crt::CRTTrack> crtTracks;
    std::map<int, TrackMatch> matchingMap;
    std::vector<RecoCRTTrack> recoCrtTracks;
  };

  class CRTTrackMatchingAna : public art::EDAnalyzer {
  public:

    // Describes configuration parameters of the module
    struct Config {
      using Name = fhicl::Name;
      using Comment = fhicl::Comment;
 
      // One Atom for each parameter
      fhicl::Atom<art::InputTag> SimModuleLabel {
        Name("SimModuleLabel"),
        Comment("tag of detector simulation data product")
      };

      fhicl::Atom<art::InputTag> CrtTrackModuleLabel {
        Name("CrtTrackModuleLabel"),
        Comment("tag of CRT track producer data product")
      };

      fhicl::Atom<art::InputTag> TpcTrackModuleLabel {
        Name("TpcTrackModuleLabel"),
        Comment("tag of TPC track producer data product")
      };

      fhicl::Atom<bool> Verbose {
        Name("Verbose"),
        Comment("Print information about what's going on")
      };

      fhicl::Atom<double> MaxAngleDiff {
        Name("MaxAngleDiff"),
        Comment("Maximum difference between CRT and TPC track angles")
      };

      fhicl::Atom<double> MaxDistance {
        Name("MaxDistance"),
        Comment("Maximum distance between CRT and TPC track start/end positions")
      };
      
    }; // Config
 
    using Parameters = art::EDAnalyzer::Table<Config>;
 
    // Constructor: configures module
    explicit CRTTrackMatchingAna(Parameters const& config);
 
    // Called once, at start of the job
    virtual void beginJob() override;
 
    // Called once per event
    virtual void analyze(const art::Event& event) override;

    // Called once, at end of the job
    virtual void endJob() override;

    std::vector<RecoCRTTrack> CrtToRecoTrack(crt::CRTTrack, int id);

    std::vector<RecoCRTTrack> CreateRecoCRTTrack(TVector3 start, TVector3 end, double shift, int tpc, int id, double time, bool complete);

    //std::vector<RecoCRTTrack> CreateRecoCRTTrackErrors(crt::CRTTrack track, int tpc, int id);

    void DrawTrueTracks(RecoTruth truthMatch, bool truth, bool tpctracks, bool crttracks, bool crtreco, int id);

    void DrawCube(TCanvas *c1, double *rmin, double *rmax, int col);

    bool CrossesTPC(crt::CRTTrack track);

    double DistToCrtTrack(TVector3 trackPos, TVector3 crtCross, TVector3 crtErr);

    std::pair<TVector3, TVector3> TpcCrossPoints(simb::MCParticle const& particle);

    double TrackAveDistSq(recob::Track track, TVector3 start, TVector3 end);

  private:

    // fcl file parameters
    art::InputTag fSimModuleLabel;      ///< name of detsim producer
    art::InputTag fCrtTrackModuleLabel; ///< name of CRT track producer
    art::InputTag fTpcTrackModuleLabel; ///< name of TPC track producer
    bool          fVerbose;             ///< print information about what's going on
    double        fMaxAngleDiff;        ///< max difference between CRT and TPC angles
    double        fMaxDistance;         ///< max distance between CRT and TPC start/end positions

    // histograms
    TH1D* fDeltaLengthMatch;
    TH1D* fDeltaThetaMatch;
    TH1D* fDeltaPhiMatch;
    TH1D* fDeltaDistMatch;
    TH1D* fChiSqMatch;
    TH2D* fDeltaAnglesMatch;
    TH1D* fDeltaLengthNoMatch;
    TH1D* fDeltaThetaNoMatch;
    TH1D* fDeltaPhiNoMatch;
    TH1D* fDeltaDistNoMatch;
    TH1D* fChiSqNoMatch;
    TH2D* fDeltaAnglesNoMatch;
    TH1D* fNCrtCandidates;
    TH1D* fNTpcCandidates;

    // Other variables shared between different methods.
    geo::GeometryCore const* fGeometryService;                 ///< pointer to Geometry provider
    detinfo::DetectorProperties const* fDetectorProperties;    ///< pointer to detector properties provider
    detinfo::DetectorClocks const* fDetectorClocks;            ///< pointer to detector clocks provider

    // Performance Counters
    int nCorrectMatch = 0;
    int nIncorrectMatch = 0;
    int nCorrectNoMatch = 0;
    int nIncorrectNoMatch = 0;
    int nCorrectTime = 0;
    int nIncorrectTime = 0;

  }; // class CRTTrackMatchingAna

  // Constructor
  CRTTrackMatchingAna::CRTTrackMatchingAna(Parameters const& config)
    : EDAnalyzer(config)
    , fSimModuleLabel       (config().SimModuleLabel())
    , fCrtTrackModuleLabel  (config().CrtTrackModuleLabel())
    , fTpcTrackModuleLabel  (config().TpcTrackModuleLabel())
    , fVerbose              (config().Verbose())
    , fMaxAngleDiff         (config().MaxAngleDiff())
    , fMaxDistance          (config().MaxDistance())
  {
    // Get a pointer to the geometry service provider
    fGeometryService    = lar::providerFrom<geo::Geometry>();
    fDetectorProperties = lar::providerFrom<detinfo::DetectorPropertiesService>(); 
    fDetectorClocks     = lar::providerFrom<detinfo::DetectorClocksService>(); 
  }

  void CRTTrackMatchingAna::beginJob()
  {
    // Access tfileservice to handle creating and writing histograms
    art::ServiceHandle<art::TFileService> tfs;
    fDeltaLengthMatch   = tfs->make<TH1D>("deltalengthmatch", ";#Delta length (cm)", 50, -700, 700);
    fDeltaThetaMatch    = tfs->make<TH1D>("deltathetamatch", ";#Delta #theta (deg)", 200, -180, 180);
    fDeltaPhiMatch      = tfs->make<TH1D>("deltaphimatch", ";#Delta #phi (deg)", 200, -180, 180);
    fDeltaDistMatch     = tfs->make<TH1D>("deltadistmatch", ";#Delta distance (cm)", 50, 0, 700);
    fChiSqMatch         = tfs->make<TH1D>("chisqmatch", ";#chi^2", 50, 0, 5000);
    fDeltaAnglesMatch   = tfs->make<TH2D>("deltaanglesmatch", "#Delta #theta (rad);#Delta #phi (rad)", 50, -4, 4, 50, -4, 4);
    fDeltaLengthNoMatch = tfs->make<TH1D>("deltalengthnomatch", ";#Delta length (cm)", 50, -700, 700);
    fDeltaThetaNoMatch  = tfs->make<TH1D>("deltathetanomatch", ";#Delta #theta (deg)", 200, -180, 180);
    fDeltaPhiNoMatch    = tfs->make<TH1D>("deltaphinomatch", ";#Delta #phi (deg)", 200, -180, 180);
    fDeltaDistNoMatch   = tfs->make<TH1D>("deltadistnomatch", ";#Delta distance (cm)", 50, 0, 700);
    fChiSqNoMatch       = tfs->make<TH1D>("chisqnomatch", ";#chi^2", 50, 0, 5000);
    fDeltaAnglesNoMatch = tfs->make<TH2D>("deltaanglesnomatch", "#Delta #theta (rad);#Delta #phi (rad)", 50, -4, 4, 50, -4, 4);
    fNCrtCandidates     = tfs->make<TH1D>("ncrtcandidates", ";N CRT candidates", 5, 0, 5);
    fNTpcCandidates     = tfs->make<TH1D>("ntpccandidates", ";N CRT candidates", 5, 0, 5);

    // Initial output
    if(fVerbose) std::cout<<"----------------- CRT Track Matching Ana Module -------------------"<<std::endl;

  } // CRTTrackMatchingAna::beginJob()

  void CRTTrackMatchingAna::analyze(const art::Event& event)
  {

    // Fetch basic event info
    if(fVerbose){
      std::cout<<"============================================"<<std::endl
               <<"Run = "<<event.run()<<", SubRun = "<<event.subRun()<<", Event = "<<event.id().event()<<std::endl
               <<"============================================"<<std::endl;
    }

    // Retrieve all the truth info in the events
    auto particleHandle = event.getValidHandle<std::vector<simb::MCParticle>>(fSimModuleLabel);
    // Fill a map of true particles
    std::map<int, simb::MCParticle> particles;
    std::vector<simb::MCParticle> parts;
    for (auto const& particle: (*particleHandle)){
      int partId = particle.TrackId();
      particles[partId] = particle;
      parts.push_back(particle);
    }

    if(fVerbose) std::cout<<"Number of true particles = "<<particles.size()<<std::endl;

    // Retrieve list of CRT tracks
    auto crtTrackHandle = event.getValidHandle<std::vector<crt::CRTTrack>>(fCrtTrackModuleLabel);

    // Retrieve the TPC tracks
    auto tpcTrackHandle = event.getValidHandle<std::vector<recob::Track>>(fTpcTrackModuleLabel);

    // Get track to hit associations
    art::FindManyP<recob::Hit> findManyHits(tpcTrackHandle, event, fTpcTrackModuleLabel);

    if(fVerbose) std::cout<<"Number of CRT tracks = "<<crtTrackHandle->size()<<std::endl
                          <<"Number of TPC tracks = "<<tpcTrackHandle->size()<<std::endl;


    //----------------------------------   Truth Matching   -----------------------------------
    if(fVerbose) std::cout<<"\n----------------------------------   Truth Matching   -----------------------------------\n";

    std::map<int, TrackMatch> matchingMap;
    std::vector<recob::Track> tpcTracks;
    // Loop over TPC tracks
    for (auto const& tpcTrack : (*tpcTrackHandle)){
      TrackMatch trackMatch;
      trackMatch.trueTime = -99999;
      int trackID = tpcTrack.ID();
      if(fVerbose) std::cout<<"\n-->TPC Track:"<<trackID<<"\n";

      tpcTracks.push_back(tpcTrack);
      TVector3 tpcStart = tpcTrack.Vertex();
      TVector3 tpcEnd = tpcTrack.End();

      // Get the associated hits
      std::vector<art::Ptr<recob::Hit>> hits = findManyHits.at(tpcTrack.ID());
      int tpc = hits[0]->WireID().TPC;
      trackMatch.tpc = tpc;

      // Find the true particle ID and check it exists
      if (fVerbose) std::cout<<"Number of hits = "<<hits.size()<<", TPC = "<<trackMatch.tpc<<std::endl;
      int trueId = RecoUtils::TrueParticleIDFromTotalRecoHits(hits);
      trackMatch.trueID = trueId;

      if (particles.find(trueId) == particles.end()){ 
        if (fVerbose) std::cout<<"No valid true tpcTrack!\n"; 
        matchingMap[trackID] = trackMatch; 
        continue; 
      }

      simb::MCParticle particle = particles[trueId];

      // Match angles to get rid of secondary particles
      std::pair<TVector3, TVector3> trueStartEnd = TpcCrossPoints(particle);
      TVector3 trueStart = trueStartEnd.first;
      TVector3 trueEnd = trueStartEnd.second;
      double tpcTheta = (tpcStart - tpcEnd).Theta();
      double tpcPhi = (tpcStart - tpcEnd).Phi();
      double trueTheta = (trueStart - trueEnd).Theta();
      double truePhi = (trueStart - trueEnd).Phi();
      if(std::max((trueStart-tpcStart).Mag(), (trueEnd-tpcEnd).Mag()) > std::max((trueStart-tpcEnd).Mag(), (trueEnd-tpcStart).Mag())){
        trueTheta = (trueEnd - trueStart).Theta();
        truePhi = (trueEnd - trueStart).Phi();
      }
      double dTheta = atan2(sin(tpcTheta - trueTheta), cos(tpcTheta - trueTheta));
      double dPhi = atan2(sin(tpcPhi - truePhi), cos(tpcPhi - truePhi));

      // Get the true t0
      double trueTimeTicks = fDetectorClocks->TPCG4Time2Tick(particle.T()); // ns -> ticks
      trackMatch.trueTime = trueTimeTicks;
      size_t nTrajPoints = particle.NumberTrajectoryPoints();

      if(fVerbose) std::cout<<"True particle information:\n"<<"PDG = "<<particle.PdgCode()<<", length = "
                            <<particle.Trajectory().TotalLength()<<" cm, time = "<<trueTimeTicks<<" ticks\n"
                            <<"Reco start = ("<<tpcStart.X()<<", "<<tpcStart.Y()<<", "<<tpcStart.Z()
                            <<") reco end = ("<<tpcEnd.X()<<", "<<tpcEnd.Y()<<", "<<tpcEnd.Z()<<") length = "<<tpcTrack.Length()<<"\n";

      if(std::abs(dTheta) > 0.1 || std::abs(dPhi) > 0.1){ 
        if (fVerbose) std::cout<<"True-reco track angles don't match! dTheta = "<<dTheta<<", dPhi = "<<dPhi<<"\n";
        matchingMap[trackID] = trackMatch;
        continue;
      }

      // Loop over the CRT tracks
      int crtIndex = 0;
      for (auto const& crtTrack : (*crtTrackHandle)){
        // Only look at tracks which cross the TPC
        if(!CrossesTPC(crtTrack)){ crtIndex++; continue; }
        // Find the time difference with the true particle
        double crtTimeTicks = fDetectorClocks->TPCG4Time2Tick((double)(int)crtTrack.ts1_ns/10); // ns -> ticks
        double timeDiff = std::abs(crtTimeTicks - trueTimeTicks);

        // Find the distance between start and end points and the true particle
        TVector3 crtStart(crtTrack.x1_pos, crtTrack.y1_pos, crtTrack.z1_pos);
        TVector3 crtEnd(crtTrack.x2_pos, crtTrack.y2_pos, crtTrack.z2_pos);
        TVector3 crtStartErr(crtTrack.x1_err, crtTrack.y1_err, crtTrack.z1_err);
        TVector3 crtEndErr(crtTrack.x2_err, crtTrack.y2_err, crtTrack.z2_err);
        double minStartDist = 99999;
        double minEndDist = 99999;
        for (size_t i = 0; i < nTrajPoints; i++){
          TVector3 trajPoint(particle.Vx(i), particle.Vy(i), particle.Vz(i));
          double startDist = DistToCrtTrack(trajPoint, crtStart, crtStartErr);
          if(startDist < minStartDist) minStartDist = startDist;
          double endDist = DistToCrtTrack(trajPoint, crtEnd, crtEndErr);
          if(endDist < minEndDist) minEndDist = endDist;
        }
        // If there is a track at the same time and the start and end points are within a certain 
        //dist of the true trajectory then mark as "matchable"
        // Store in some kind of map with the track ID and crt track index
        if(timeDiff<20 && fVerbose){ 
          std::cout<<"CRT Track:"<<crtIndex<<"\n"
                   <<"Time = "<<crtTimeTicks<<" start = ("<<crtStart.X()<<", "<<crtStart.Y()<<", "<<crtStart.Z()
                   <<") end = ("<<crtEnd.X()<<", "<<crtEnd.Y()<<", "<<crtEnd.Z()<<")\n"
                   <<"Time diff = "<<timeDiff<<", min start dist = "<<minStartDist<<", min end dist = "<<minEndDist<<"\n";
        }

        if(timeDiff < 2. && minStartDist < 40. && minEndDist < 40.){ 
          trackMatch.crtIDs.push_back(crtIndex);
          if(fVerbose) std::cout<<"Matches CRT track, time diff = "<<timeDiff<<", min start dist = "<<minStartDist<<", min end dist = "<<minEndDist<<"\n";
        }

        crtIndex++;
      }
      matchingMap[trackID] = trackMatch;
    }

    //---------------------------------- Matching Algorithm -----------------------------------
    if(fVerbose) std::cout<<"\n---------------------------------- CRTTrack->RecoCRTTrack -----------------------------------\n";

    //TODO Account for crt track errors
    int crtIndex = 0;
    std::vector<crt::CRTTrack> crtTracks;
    std::vector<RecoCRTTrack> recoCrtTracks;
    // Loop over CRT tracks
    for (auto const& crtTrack : (*crtTrackHandle)){

      crtTracks.push_back(crtTrack);

      //Check that crt track crosses tpc volume, if not skip it
      if(!CrossesTPC(crtTrack)){ crtIndex++; continue; }

      std::vector<RecoCRTTrack> tempTracks = CrtToRecoTrack(crtTrack, crtIndex);
      recoCrtTracks.insert(recoCrtTracks.end(), tempTracks.begin(), tempTracks.end());

      crtIndex++;
    }

    if(fVerbose) std::cout<<"\n---------------------------------- Matching Algorithm -----------------------------------\n";
    // Loop over the reco crt tracks
    std::map<int, int> nTpcCandidates;
    for (auto const& recoCrtTrack : recoCrtTracks){
      if(fVerbose) std::cout<<"\nRecoCRTTrack: "<<recoCrtTrack.crtID<<"\n";
      std::vector<std::pair<int, double>> crtTpcMatchCandidates;
      // Loop over the TPC tracks
      for (auto const& tpcTrack : (*tpcTrackHandle)){
        int trackID = tpcTrack.ID();
        //if (tpcTrack.Length() < 10.){ if(fVerbose) std::cout<<"Track too short!\n"; continue; }
        // If the tpcTrack has been stitched across the CPA it already has an associated t0
        std::vector<art::Ptr<recob::Hit>> hits = findManyHits.at(tpcTrack.ID());
        int tpc = hits[0]->WireID().TPC;
        //if (tpc != (int)hits[hits.size()-1]->WireID().TPC){ if(fVerbose) std::cout<<"Track has been stitched!\n"; continue;}

        // Get the length, angle and start and end position of the TPC track
        TVector3 tpcStart = tpcTrack.Vertex();
        TVector3 tpcEnd = tpcTrack.End();
        double tpcLength = (tpcStart - tpcEnd).Mag();
        double tpcTheta = (tpcStart - tpcEnd).Theta();
        double tpcPhi = (tpcStart - tpcEnd).Phi();

        // Get the length, angle and start and end position of the TPC track
        TVector3 crtStart = recoCrtTrack.start;
        TVector3 crtEnd = recoCrtTrack.end;
        double crtLength = (crtStart - crtEnd).Mag();
        double crtTheta = (crtStart - crtEnd).Theta();
        double crtPhi = (crtStart - crtEnd).Phi();

        // Find the difference with the CRT track
        double dDist1 = (crtStart-tpcStart).Mag();
        double dDist2 = (crtEnd-tpcEnd).Mag();
        if(std::max((crtStart-tpcStart).Mag(), (crtEnd-tpcEnd).Mag()) > std::max((crtStart-tpcEnd).Mag(), (crtEnd-tpcStart).Mag())){
          crtTheta = (crtEnd - crtStart).Theta();
          crtPhi = (crtEnd - crtStart).Phi();
          dDist1 = (crtEnd-tpcStart).Mag();
          dDist2 = (crtStart-tpcEnd).Mag();
        }
        double dLength = tpcLength - crtLength;
        double dTheta = atan2(sin(tpcTheta - crtTheta), cos(tpcTheta - crtTheta));
        double dPhi = atan2(sin(tpcPhi - crtPhi), cos(tpcPhi - crtPhi));
        double aveDistSq = TrackAveDistSq(tpcTrack, crtStart, crtEnd);

        // Plot difference for tracks with and without true match
        if(std::find(matchingMap[trackID].crtIDs.begin(), matchingMap[trackID].crtIDs.end(), recoCrtTrack.crtID) != matchingMap[trackID].crtIDs.end()
           && matchingMap[trackID].tpc == recoCrtTrack.tpc){

          if(fVerbose) std::cout<<"TPC track start = ("<<tpcStart.X()<<", "<<tpcStart.Y()<<", "<<tpcStart.Z()
                                <<"), end = ("<<tpcEnd.X()<<", "<<tpcEnd.Y()<<", "<<tpcEnd.Z()<<")\n"
                                <<"True match: ID = "<<trackID<<" tpc len = "<<tpcLength<<" crt len = "<<crtLength<<" diff = "<<dLength
                                <<" tpc theta = "<<tpcTheta<<" crt theta = "<<crtTheta<<" diff = "<<dTheta<<" tpc phi = "
                                <<tpcPhi<<" crt phi = "<<crtPhi<<" diff = "<<dPhi<<" dist1 = "<<dDist1<<" dist2 = "<<dDist2<<"\n";

          fDeltaLengthMatch->Fill(dLength);
          fDeltaThetaMatch->Fill(dTheta*180/(TMath::Pi()));
          fDeltaPhiMatch->Fill(dPhi*180/(TMath::Pi()));
          fDeltaDistMatch->Fill(std::min(dDist1,dDist2));
          fChiSqMatch->Fill(aveDistSq);
          fDeltaAnglesMatch->Fill(dTheta, dPhi);
        }
        else {
          fDeltaLengthNoMatch->Fill(dLength);
          fDeltaThetaNoMatch->Fill(dTheta*180/(TMath::Pi()));
          fDeltaPhiNoMatch->Fill(dPhi*180/(TMath::Pi()));
          fDeltaDistNoMatch->Fill(std::min(dDist1,dDist2));
          fChiSqNoMatch->Fill(aveDistSq);
          fDeltaAnglesNoMatch->Fill(dTheta, dPhi);
        }

        // Do the actual matching
        if((std::abs(dTheta) < fMaxAngleDiff && std::abs(dPhi) < fMaxAngleDiff && tpc == recoCrtTrack.tpc) && (dDist1<fMaxDistance||dDist2<fMaxDistance)){
          crtTpcMatchCandidates.push_back(std::make_pair(trackID, std::abs(dTheta)));
          nTpcCandidates[trackID]++;

          if(std::find(matchingMap[trackID].crtIDs.begin(), matchingMap[trackID].crtIDs.end(), recoCrtTrack.crtID) != matchingMap[trackID].crtIDs.end()){ 
            if(fVerbose) std::cout<<"Correct match! ID = "<<trackID<<" dTheta = "<<dTheta<<" dPhi = "<<dPhi<<" dist1 = "<<dDist1
                                  <<" dist2 = "<<dDist2<<" dLength = "<<dLength<<" aveDistSq = "<<aveDistSq<<"\n";
          }
          else{ 
            if(fVerbose) std::cout<<"Incorrect match! ID = "<<trackID<<" dTheta = "<<dTheta<<" dPhi = "<<dPhi<<" dist1 = "<<dDist1
                                  <<" dist2 = "<<dDist2<<" dLength = "<<dLength<<" aveDistSq = "<<aveDistSq<<"\n";
          }

        }
        else if(std::find(matchingMap[trackID].crtIDs.begin(), matchingMap[trackID].crtIDs.end(), recoCrtTrack.crtID) != matchingMap[trackID].crtIDs.end() && tpc == recoCrtTrack.tpc){
          if(fVerbose) std::cout<<"Incorrect non match! ID = "<<trackID<<" Theta = "<<dTheta<<" dPhi = "<<dPhi<<" dist1 = "<<dDist1
                                <<" dist2 = "<<dDist2<<" dLength = "<<dLength<<" aveDistSq = "<<aveDistSq<<"\n";
        } 
      }
      // Choose the track which matches the closest
      int matchedTrackID = -99999;
      if(crtTpcMatchCandidates.size() > 0){
        std::sort(crtTpcMatchCandidates.begin(), crtTpcMatchCandidates.end(), [](auto& left, auto& right){
                  return left.second < right.second;});
        matchedTrackID = crtTpcMatchCandidates[0].first;
      }
      fNCrtCandidates->Fill(crtTpcMatchCandidates.size());

      if(matchedTrackID != -99999){
        if(std::find(matchingMap[matchedTrackID].crtIDs.begin(), matchingMap[matchedTrackID].crtIDs.end(), recoCrtTrack.crtID) != matchingMap[matchedTrackID].crtIDs.end()){
          if(fVerbose) std::cout<<"Matched correctly: Tpc ID = "<<matchedTrackID<<" crt ID = "<<recoCrtTrack.crtID<<" true time = "<<matchingMap[matchedTrackID].trueTime
                                <<" crt time = "<<recoCrtTrack.trueTime<<" n cands = "<<crtTpcMatchCandidates.size()<<"\n";
          nCorrectMatch++;
        }
        else{ 
          if(fVerbose) std::cout<<"Matched incorrectly: Tpc ID = "<<matchedTrackID<<" crt ID = "<<recoCrtTrack.crtID<<" true time = "<<matchingMap[matchedTrackID].trueTime
                                <<" crt time = "<<recoCrtTrack.trueTime<<" n cands = "<<crtTpcMatchCandidates.size()<<"\n";
          nIncorrectMatch++;
        }
        if(std::abs(matchingMap[matchedTrackID].trueTime - recoCrtTrack.trueTime) < 5.) nCorrectTime++;
        else nIncorrectTime++;
      }
      else{
        bool matchable = false;
        for(auto const& matching : matchingMap){
          if(std::find(matching.second.crtIDs.begin(), matching.second.crtIDs.end(), recoCrtTrack.crtID) != matching.second.crtIDs.end() && matching.second.tpc == recoCrtTrack.tpc){ 
            if(fVerbose) std::cout<<"Should have matched: Tpc ID = "<<matching.first<<" crt ID = "<<recoCrtTrack.crtID
                                  <<" true time = "<<matching.second.trueTime<<" crt time = "<<recoCrtTrack.trueTime<<"\n";
            matchable = true;
          }
        }
        if(matchable){ 
          nIncorrectNoMatch++;
        }
        else{
          if(fVerbose) std::cout<<"Correctly didn't match anything: crt ID = "<<recoCrtTrack.crtID<<" crt time = "<<recoCrtTrack.trueTime<<"\n";
          nCorrectNoMatch++;
        }
      }
    }

    for(auto const& tpcCands : nTpcCandidates){
      fNTpcCandidates->Fill(tpcCands.second);
    }

    RecoTruth truthMatch;
    truthMatch.particles = parts;
    truthMatch.crtTracks = crtTracks;
    truthMatch.tpcTracks = tpcTracks;
    truthMatch.matchingMap = matchingMap;
    truthMatch.recoCrtTracks = recoCrtTracks;
    //DrawTrueTracks(truthMatch, true, true, true, true, 49);

    //--------------------------------- Performance Analysis ----------------------------------

  } // CRTTrackMatchingAna::analyze()

  void CRTTrackMatchingAna::endJob(){

    std::cout<<"----------------- RESULTS --------------------\n"
             <<"Number of correct matches       = "<<nCorrectMatch<<"\n"
             <<"Number of incorrect matches     = "<<nIncorrectMatch<<"\n"
             <<"Number of correct non matches   = "<<nCorrectNoMatch<<"\n"
             <<"Number of incorrect non matches = "<<nIncorrectNoMatch<<"\n"
             <<"Efficiency = "<<((double)(nCorrectMatch+nCorrectNoMatch))/((double)(nCorrectMatch+nCorrectNoMatch+nIncorrectMatch+nIncorrectNoMatch))<<"\n"
             <<"Purity = "<<((double)(nCorrectMatch))/((double)(nCorrectMatch+nIncorrectMatch))<<"\n"
             <<"Number of matches with correct time = "<<nCorrectTime<<", incorrect time = "<<nIncorrectTime<<" purity = "<<((double)nCorrectTime)/(nCorrectTime+nIncorrectTime)<<"\n";
  
  } // CRTTrackMatchingAna::endJob()

  std::vector<RecoCRTTrack> CRTTrackMatchingAna::CrtToRecoTrack(crt::CRTTrack track, int id){
    std::vector<RecoCRTTrack> recoCrtTracks;
    // Get the time of the track
    double crtTimeTicks = fDetectorClocks->TPCG4Time2Tick((double)(int)track.ts1_ns/10); // ns -> tick
    // Convert time into a x shift
    double xShift = 0.5*crtTimeTicks*fDetectorProperties->DriftVelocity(); //FIXME
    double xShift2 = fDetectorClocks->TPCTick2Time(crtTimeTicks)*fDetectorProperties->DriftVelocity(); //FIXME
    std::cout<<"X shift = "<<xShift<<"  "<<xShift2<<"\n";

    // Shift track, remembering to take into account the tpc, if the track crosses the cpa and 
    //the size of the readout window
    TVector3 crtStart(track.x1_pos, track.y1_pos, track.z1_pos);
    TVector3 crtEnd(track.x2_pos, track.y2_pos, track.z2_pos);
    if(crtStart.Y() < crtEnd.Y()) std::swap(crtStart, crtEnd);
    if(!track.complete){
      // Find point where track crosses bottom plane
      TVector3 diff = (crtEnd - crtStart).Unit();
      TVector3 newEnd = crtStart + 5000*diff; //FIXME
      crtEnd = newEnd;
    }

    if(fVerbose) std::cout<<"\nCRT Track:"<<id<<"\n"
                          <<"Time = "<<crtTimeTicks<<" start = ("<<crtStart.X()<<", "<<crtStart.Y()<<", "<<crtStart.Z()
                          <<") end = ("<<crtEnd.X()<<", "<<crtEnd.Y()<<", "<<crtEnd.Z()<<")\n";

    TVector3 cpaCrossStart, cpaCrossEnd;
    // Calculate the expected reconstructed length, angle and start and end position
    std::vector<int> crtTpc;
    if(crtStart.X() < 0. && crtEnd.X() < 0.){ //FIXME
      // Track in TPC 0
      std::vector<RecoCRTTrack> tempTracks = CreateRecoCRTTrack(crtStart, crtEnd, xShift, 0, id, crtTimeTicks, track.complete);
      recoCrtTracks.insert(recoCrtTracks.end(), tempTracks.begin(), tempTracks.end());
    }
    else if(crtStart.X() > 0. && crtEnd.X() > 0.){ //FIXME
      // Track in TPC 1
      std::vector<RecoCRTTrack> tempTracks = CreateRecoCRTTrack(crtStart, crtEnd, xShift, 1, id, crtTimeTicks, track.complete);
      recoCrtTracks.insert(recoCrtTracks.end(), tempTracks.begin(), tempTracks.end());
    }
    else {
      // Track in both TPCs and will be split
      TVector3 direction = crtStart - crtEnd;
      double step = (0. - crtStart.X())/direction.X(); //FIXME
      TVector3 cpaCross(0., crtStart.Y() + step*direction.Y(), crtStart.Z() + step*direction.Z()); //FIXME
      cpaCrossStart = cpaCross;
      cpaCrossEnd = cpaCross;
      if(crtStart.X() < 0.){ //FIXME
        std::vector<RecoCRTTrack> tempTracks0 = CreateRecoCRTTrack(crtStart, cpaCrossStart, xShift, 0, id, crtTimeTicks, track.complete);
        recoCrtTracks.insert(recoCrtTracks.end(), tempTracks0.begin(), tempTracks0.end());

        std::vector<RecoCRTTrack> tempTracks1 = CreateRecoCRTTrack(crtEnd, cpaCrossEnd, xShift, 1, id, crtTimeTicks, track.complete);
        recoCrtTracks.insert(recoCrtTracks.end(), tempTracks1.begin(), tempTracks1.end());
      }
      else {
        std::vector<RecoCRTTrack> tempTracks0 = CreateRecoCRTTrack(crtEnd, cpaCrossEnd, xShift, 0, id, crtTimeTicks, track.complete);
        recoCrtTracks.insert(recoCrtTracks.end(), tempTracks0.begin(), tempTracks0.end());

        std::vector<RecoCRTTrack> tempTracks1 = CreateRecoCRTTrack(crtStart, cpaCrossStart, xShift, 1, id, crtTimeTicks, track.complete);
        recoCrtTracks.insert(recoCrtTracks.end(), tempTracks1.begin(), tempTracks1.end());
      }
    }
    return recoCrtTracks;
  } // CRTTrackMatchingAna::CRTToRecoTrack()

  std::vector<RecoCRTTrack> CRTTrackMatchingAna::CreateRecoCRTTrack(TVector3 start, TVector3 end, double shift, int tpc, int id, double time, bool complete){
    std::vector<RecoCRTTrack> recoCrtTracks;

    // Get the true entry and exit points in the TPC
    double xmin = -2.0 * fGeometryService->DetHalfWidth();
    double xmax = 2.0 * fGeometryService->DetHalfWidth();
    double ymin = -fGeometryService->DetHalfHeight();
    double ymax = fGeometryService->DetHalfHeight();
    double zmin = 0.;
    double zmax = fGeometryService->DetLength();
    // Get track info
    TVector3 diff = end - start;
    TVector3 startTPC, endTPC;
    bool first = true;
    // Loop over trajectory points
    int npts = 1000;
    for (int traj_i = 0; traj_i <= npts; traj_i++){
      TVector3 trajPoint = start + ((traj_i)/((double)npts))*diff;
      // Check if point is within reconstructable volume
      if (trajPoint[0] >= xmin && trajPoint[0] <= xmax && trajPoint[1] >= ymin && trajPoint[1] <= ymax 
          && trajPoint[2] >= zmin && trajPoint[2] <= zmax && first){
        first = false;
        startTPC = trajPoint;
      }
      if (trajPoint[0] >= xmin && trajPoint[0] <= xmax && trajPoint[1] >= ymin && trajPoint[1] <= ymax 
          && trajPoint[2] >= zmin && trajPoint[2] <= zmax){
        endTPC = trajPoint;
      }
    }

    // Don't shift if not inside TPC
    if(startTPC.X()==0 && endTPC.X()==0){ //FIXME
      if(fVerbose) std::cout<<"Doesn't pass through TPC "<<tpc<<"!\n";
      return recoCrtTracks;
    }

    // Shift in x depending on TPC
    if(tpc == 1){
      // Track in TPC 1
      startTPC[0] -= shift;
      endTPC[0] -= shift;
    }
    else if(tpc == 0){
      // Track in TPC 0
      startTPC[0] += shift;
      endTPC[0] += shift;
    }
    
    double readoutWindow  = (double)fDetectorProperties->ReadOutWindowSize();
    double driftTimeTicks = 2.0*(2.*fGeometryService->DetHalfWidth())/fDetectorProperties->DriftVelocity(); //FIXME
    double deltaX = (readoutWindow - driftTimeTicks)*0.5*fDetectorProperties->DriftVelocity(); //FIXME

    if(tpc == 0) xmax = deltaX;
    if(tpc == 1) xmin = -deltaX;

    // Get track info
    TVector3 diffTPC = endTPC - startTPC;
    TVector3 startCut, endCut;
    first = true;
    // Loop over trajectory points
    for (int traj_i = 0; traj_i <= npts; traj_i++){
      TVector3 trajPoint = startTPC + ((traj_i)/((double)npts))*diffTPC;
      // Check if point is within reconstructable volume
      if (trajPoint[0] >= xmin && trajPoint[0] <= xmax && trajPoint[1] >= ymin && trajPoint[1] <= ymax 
          && trajPoint[2] >= zmin && trajPoint[2] <= zmax && first){
        first = false;
        startCut = trajPoint;
      }
      if (trajPoint[0] >= xmin && trajPoint[0] <= xmax && trajPoint[1] >= ymin && trajPoint[1] <= ymax 
          && trajPoint[2] >= zmin && trajPoint[2] <= zmax){
        endCut = trajPoint;
      }
    }

    if(!(startCut.X()==0 && endCut.X()==0)){ //FIXME
      RecoCRTTrack recoCrtTrack = {id, tpc, startCut, endCut, time, complete};
      recoCrtTracks.push_back(recoCrtTrack);

      if(fVerbose) std::cout<<"TPC "<<tpc<<": shifted start = ("<<startCut.X()<<", "<<startCut.Y()<<", "<<startCut.Z()
                            <<") end = ("<<endCut.X()<<", "<<endCut.Y()<<", "<<endCut.Z()<<")\n";
    }
    else{
      if(fVerbose) std::cout<<"Shifted outside of reconstructable window!\n";
    }

    return recoCrtTracks;

  } // CRTTrackMatchingAna::CreateRecoCRTTrack()
  
  void CRTTrackMatchingAna::DrawTrueTracks(RecoTruth rt, bool truth, bool tpcTracks, bool crtTracks, bool crtreco, int id){
    // Positions of the CRT planes
    std::vector<double> crtPlanes = {-359.1, -357.3, 357.3, 359.1, -358.9, -357.1, 661.52, 663.32, 865.52, 867.32, -240.65, -238.85, 655.35, 657.15};
    std::vector<int> fixCoord   = {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2}; // Fixed coordinate for each plane
    std::vector<int> widthCoord = {2, 1, 2, 1, 0, 2, 2, 0, 2, 0, 1, 0, 1, 0}; // Width direction for each plane
    std::vector<int> lenCoord   = {1, 2, 1, 2, 2, 0, 0, 2, 0, 2, 0, 1, 0, 1}; // Length direction for each plane
    // Create a canvas 
    TCanvas *c1 = new TCanvas("c1","",700,700);
    // Draw the tagger planes
    for(int tag_i = 0; tag_i < 7; tag_i++){
      double tagCenter[3] = {0, 0, 208.25};
      tagCenter[fixCoord[tag_i*2]] = (crtPlanes[tag_i*2]+crtPlanes[tag_i*2+1])/2;
      double tagDim[3] = {0, 0, 0};
      if(tag_i==0 || tag_i==1){ tagDim[0] = 1.8; tagDim[1] = 360; tagDim[2] = 450; }
      if(tag_i==2){ tagDim[0] = 399.5; tagDim[1] = 1.8; tagDim[2] = 478; }
      if(tag_i==3 || tag_i==4){ tagDim[0] = 450; tagDim[1] = 1.8; tagDim[2] = 450; }
      if(tag_i==5 || tag_i==6){ tagDim[0] = 360; tagDim[1] = 360; tagDim[2] = 1.8; }
      double rmin[3] = {tagCenter[0]-tagDim[0],tagCenter[1]-tagDim[1],tagCenter[2]-tagDim[2]};
      double rmax[3] = {tagCenter[0]+tagDim[0],tagCenter[1]+tagDim[1],tagCenter[2]+tagDim[2]};
      DrawCube(c1, rmin, rmax, 1);
    }

    double xmin = -2.0 * fGeometryService->DetHalfWidth();
    double xmax = 2.0 * fGeometryService->DetHalfWidth();
    double ymin = -fGeometryService->DetHalfHeight();
    double ymax = fGeometryService->DetHalfHeight();
    double zmin = 0.;
    double zmax = fGeometryService->DetLength();
    double rmin[3] = {xmin, ymin, zmin};
    double rmax[3] = {xmax, ymax, zmax};
    DrawCube(c1, rmin, rmax, 2);

    // Draw the true particles
    TPolyLine3D *trajectories[100];
    TPolyLine3D *crttrack[100];
    TPolyLine3D *crtrecotrack[100];
    TPolyLine3D *tpctrack[100];
    int ncrtTracks = 0;
    int ncrt = 0;
    int nparts = 0;
    size_t lim = rt.particles.size();
    if(truth){
      for(size_t i = 0; i < lim; i++){
        int trueID = rt.particles[i].TrackId();
        bool plot = false;
        for(auto& matching : (rt.matchingMap)){
          if(matching.second.trueID == trueID) plot = true;
        }
        if(plot){
          int nTraj = rt.particles[i].NumberTrajectoryPoints();
          trajectories[nparts] = new TPolyLine3D(nTraj);
          int ipt = 0;
          for(int j = 0; j < nTraj; j++){
            double px = rt.particles[i].Vx(j);
            double py = rt.particles[i].Vy(j);
            double pz = rt.particles[i].Vz(j);
            if(abs(px) < 500 && py < 900 && py > -450 && pz < 700 && pz > -400){
              trajectories[nparts]->SetPoint(ipt, px, py, pz);
              ipt++;
            }
          }
          trajectories[nparts]->SetLineColor(4);
          trajectories[nparts]->SetLineWidth(2);
          if(id==-1||rt.matchingMap[id].trueID==trueID){ 
            trajectories[nparts]->Draw();
            nparts++;
          }
        }
      }
    }
    if(crtTracks){
      // Plot the tracks
      for(size_t i = 0; i < rt.crtTracks.size(); i++){
        // Get the start and end points
        crt::CRTTrack tr = rt.crtTracks[i];
        crttrack[ncrtTracks] = new TPolyLine3D(2);
        crttrack[ncrtTracks]->SetPoint(0, tr.x1_pos, tr.y1_pos, tr.z1_pos);
        crttrack[ncrtTracks]->SetPoint(1, tr.x2_pos, tr.y2_pos, tr.z2_pos);
        // Draw a line between them
        crttrack[ncrtTracks]->SetLineColor(2);
        crttrack[ncrtTracks]->SetLineWidth(2);
        if ((id==-1 && CrossesTPC(tr)) || std::find(rt.matchingMap[id].crtIDs.begin(), rt.matchingMap[id].crtIDs.end(), (int)i) != rt.matchingMap[id].crtIDs.end()){
          if(tr.complete){
            crttrack[ncrtTracks]->Draw();
            ncrtTracks++;
          }
          if(!tr.complete){
            TVector3 start(tr.x1_pos, tr.y1_pos, tr.z1_pos);
            TVector3 end(tr.x2_pos, tr.y2_pos, tr.z2_pos);
            if(start.Y() < end.Y()) std::swap(start, end);
            TVector3 diff = (end - start).Unit();
            TVector3 newEnd = start + 1000*diff;
            crttrack[ncrtTracks]->SetPoint(0, start.X(), start.Y(), start.Z());
            crttrack[ncrtTracks]->SetPoint(1, newEnd.X(), newEnd.Y(), newEnd.Z());
            crttrack[ncrtTracks]->Draw();
            ncrtTracks++;
          }
        }
      }
    }
    if(crtreco){
      // Plot the tracks
      for(size_t i = 0; i < rt.recoCrtTracks.size(); i++){
        // Get the start and end points
        TVector3 st = rt.recoCrtTracks[i].start;
        TVector3 ed = rt.recoCrtTracks[i].end;
        crtrecotrack[ncrt] = new TPolyLine3D(2);
        crtrecotrack[ncrt]->SetPoint(0, st.X(), st.Y(), st.Z());
        crtrecotrack[ncrt]->SetPoint(1, ed.X(), ed.Y(), ed.Z());
        // Draw a line between them
        crtrecotrack[ncrt]->SetLineColor(46);
        crtrecotrack[ncrt]->SetLineWidth(2);
        if (!(st.X()==0 && ed.X()==0) && (id==-1||std::find(rt.matchingMap[id].crtIDs.begin(), rt.matchingMap[id].crtIDs.end(), rt.recoCrtTracks[i].crtID) != rt.matchingMap[id].crtIDs.end())){
          crtrecotrack[ncrt]->Draw();
          ncrt++;
        }
      }
    }
    if(tpcTracks){
      // Plot the tracks
      for(size_t i = 0; i < rt.tpcTracks.size(); i++){
        // Get the start and end points
        recob::Track tr = rt.tpcTracks[i];
        size_t npts = tr.NumberTrajectoryPoints();
        tpctrack[i] = new TPolyLine3D(npts);
        for(size_t j = 0; j < npts; j++){
          TVector3 pos = tr.LocationAtPoint(j);
          tpctrack[i]->SetPoint(j, pos.X(), pos.Y(), pos.Z());
        }
        // Draw a line between them
        tpctrack[i]->SetLineColor(3);
        tpctrack[i]->SetLineWidth(2);
        if(id == -1 || tr.ID() == id) tpctrack[i]->Draw();
      }
    }

    c1->SaveAs("crtTagger.root");
  }

  void CRTTrackMatchingAna::DrawCube(TCanvas *c1, double *rmin, double *rmax, int col){
    c1->cd();
    TList *outline = new TList;
    TPolyLine3D *p1 = new TPolyLine3D(4);
    TPolyLine3D *p2 = new TPolyLine3D(4);
    TPolyLine3D *p3 = new TPolyLine3D(4);
    TPolyLine3D *p4 = new TPolyLine3D(4);
    p1->SetLineColor(col);
    p1->SetLineWidth(2);
    p1->Copy(*p2);
    p1->Copy(*p3);
    p1->Copy(*p4);
    outline->Add(p1);
    outline->Add(p2);
    outline->Add(p3);
    outline->Add(p4); 
    TPolyLine3D::DrawOutlineCube(outline, rmin, rmax);
    p1->Draw();
    p2->Draw();
    p3->Draw();
    p4->Draw();
  } 

  bool CRTTrackMatchingAna::CrossesTPC(crt::CRTTrack track){
    // Check if particle enters the TPC
    bool enters = false;
    double xmin = -2.0 * fGeometryService->DetHalfWidth();
    double xmax = 2.0 * fGeometryService->DetHalfWidth();
    double ymin = -fGeometryService->DetHalfHeight();
    double ymax = fGeometryService->DetHalfHeight();
    double zmin = 0.;
    double zmax = fGeometryService->DetLength();
    if(track.complete){
      // Get track info
      TVector3 start(track.x1_pos, track.y1_pos, track.z1_pos);
      TVector3 end(track.x2_pos, track.y2_pos, track.z2_pos);
      TVector3 diff = end - start;
      // Loop over trajectory points
      int npts = 100;
      for (int traj_i = 0; traj_i < npts; traj_i++){
        TVector3 trajPoint = start + ((traj_i+1)/100.)*diff;
        // Check if point is within reconstructable volume
        if (trajPoint[0] >= xmin-5 && trajPoint[0] <= xmax+5 && trajPoint[1] >= ymin-5 && trajPoint[1] <= ymax+5 && trajPoint[2] >= zmin-5 && trajPoint[2] <= zmax+5){
          enters = true;
        }
      }
    }
    // If track just between top two planes
    else{
      //
      TVector3 start(track.x1_pos, track.y1_pos, track.z1_pos);
      TVector3 end(track.x2_pos, track.y2_pos, track.z2_pos);
      if(start.Y() < end.Y()) std::swap(start, end);
      TVector3 diff = (end - start).Unit();
      int npts = 100;
      for (int traj_i = 0; traj_i < npts; traj_i++){
        TVector3 trajPoint = start + (100.*(traj_i+1))*diff; //FIXME
        // Check if point is within reconstructable volume
        if (trajPoint[0] >= xmin-5 && trajPoint[0] <= xmax+5 && trajPoint[1] >= ymin-5 && trajPoint[1] <= ymax+5 && trajPoint[2] >= zmin-5 && trajPoint[2] <= zmax+5){
          enters = true;
        }
      }
    }
    return enters;
  }

  double CRTTrackMatchingAna::DistToCrtTrack(TVector3 trackPos, TVector3 crtCross, TVector3 crtErr){
    double minDistX = 99999;
    // Loop over size of hit to find the min dist
    for(int i = 0; i < 20.; i++){
      double xpos = crtCross.X() + ((i+1.)/10. - 1.)*crtErr.X();
      double distX = std::abs(trackPos.X() - xpos);
      if(distX < minDistX) minDistX = distX;
    }
    double minDistY = 99999;
    // Loop over size of hit to find the min dist
    for(int i = 0; i < 20.; i++){
      double ypos = crtCross.Y() + ((i+1.)/10. - 1.)*crtErr.Y();
      double distY = std::abs(trackPos.Y() - ypos);
      if(distY < minDistY) minDistY = distY;
    }
    double minDistZ = 99999;
    // Loop over size of hit to find the min dist
    for(int i = 0; i < 20.; i++){
      double zpos = crtCross.Z() + ((i+1.)/10. - 1.)*crtErr.Z();
      double distZ = std::abs(trackPos.Z() - zpos);
      if(distZ < minDistZ) minDistZ = distZ;
    }
    double dist = std::sqrt(std::pow(minDistX, 2) + std::pow(minDistY, 2) + std::pow(minDistZ, 2));
    return dist;
  } // CRTTrackMatchingAna::DistToCrtTrack()

  std::pair<TVector3, TVector3> CRTTrackMatchingAna::TpcCrossPoints(simb::MCParticle const& particle){
    double xmin = -2.0 * fGeometryService->DetHalfWidth();
    double xmax = 2.0 * fGeometryService->DetHalfWidth();
    double ymin = -fGeometryService->DetHalfHeight();
    double ymax = fGeometryService->DetHalfHeight();
    double zmin = 0.;
    double zmax = fGeometryService->DetLength();
    TVector3 start, end;
    bool first = true;
    // Get the trajectory of the true particle
    size_t npts = particle.NumberTrajectoryPoints();
    // Loop over particle trajectory
    for (size_t i = 0; i < npts; i++){
      TVector3 trajPoint(particle.Vx(i), particle.Vy(i), particle.Vz(i));
      // If the particle is inside the tagger volume then set to true.
      if(trajPoint[0]>xmin && trajPoint[0]<xmax &&
         trajPoint[1]>ymin && trajPoint[1]<ymax &&
         trajPoint[2]>zmin && trajPoint[2]<zmax){
        if(first) start = trajPoint;
        first = false;
        end = trajPoint;
      }
    }
    return std::make_pair(start, end);
  } // CRTTrackMatchingAna::TpcCrossPoints()

  double CRTTrackMatchingAna::TrackAveDistSq(recob::Track track, TVector3 start, TVector3 end){
    // Draw line between start and end
    TVector3 diff = start - end;
    double p0 = start.X() - (start.Z()*(diff.X())/(diff.Z()));
    double p1 = (diff.X())/(diff.Z());
    double p2 = start.Y() - (start.Z()*(diff.Y())/(diff.Z()));
    double p3 = (diff.Y())/(diff.Z());
    TVector3 x0(p0, p2, 0.);
    TVector3 x1(p0+p1, p2+p3, 1.);
    TVector3 u = (x1-x0).Unit();
    // Loop over track trajectory
    int nTraj = track.NumberTrajectoryPoints();
    int ipt = 0;
    double xres = 0;
    for(int j = 0; j < nTraj; j++){
      TVector3 xp = track.LocationAtPoint(j);
      double d2 = ((xp-x0).Cross(u)).Mag2();
      xres += d2;
      ipt++;
    }
    return xres/ipt;
  } // CRTTrackMatchingAna::TrackAveDistSq()

  DEFINE_ART_MODULE(CRTTrackMatchingAna)
} // namespace sbnd

// Back to local namespace.
namespace {


} // local namespace


