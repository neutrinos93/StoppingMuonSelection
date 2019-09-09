///////////////////////////////////////////////////////////////////////
// Class:       MichelStudyTmp
// Plugin Type: ******
// File:        MichelStudyTmp.h
////////////////////////////////////////////////////////////////////////
#include "dune/Protodune/Analysis/ProtoDUNEPFParticleUtils.h"
#include "art_root_io/TFileService.h"
//#include "messagefacility/MessageLogger/MessageLogger.h"
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "canvas/Persistency/Common/FindManyP.h"

#include <fstream>
#include "TTree.h"
#include "TH2.h"
#include "TProfile2D.h"
#include "TMath.h"

#include "DataTypes.h"
#include "GeometryHelper.h"
#include "SpacePointAlg.h"
#include "CalorimetryHelper.h"
#include "StoppingMuonSelectionAlg.h"

namespace stoppingcosmicmuonselection {

class MichelStudyTmp;

class MichelStudyTmp : public art::EDAnalyzer {
public:
  explicit MichelStudyTmp(fhicl::ParameterSet const & p);
  // The destructor generated by the compiler is fine for classes
  // without bare pointers or other resource use.

  // Plugins should not be copied or assigned.
  MichelStudyTmp(MichelStudyTmp const &) = delete;
  MichelStudyTmp(MichelStudyTmp &&) = delete;
  MichelStudyTmp & operator = (MichelStudyTmp const &) = delete;
  MichelStudyTmp & operator = (MichelStudyTmp &&) = delete;

  // Required functions.
  void analyze(art::Event const &evt) override;

  // Selected optional functions
  void beginJob() override;
  void endJob() override;
  void reconfigure(fhicl::ParameterSet const& p);

  void UpdateTTreeVariableWithTrackProperties(const trackProperties &trackProp);

private:

  // Declare some counters for statistic purposes
  int counter_T0_tagged_tracks = 0;
  int counter_total_number_events = 0;
  int counter_total_number_tracks = 0;

  GeometryHelper           geoHelper;
  SpacePointAlg            spAlg;        // need configuration
  StoppingMuonSelectionAlg selectorAlg;  // need configuration
  CalorimetryHelper        caloHelper;   // need configuration

  std::string fPFParticleTag, fSpacePointTag, fTrackerTag;
  protoana::ProtoDUNEPFParticleUtils   pfpUtil;

  // Track Tree stuff
  TTree *fTrackTree;
  // Tree variables
  size_t fEvNumber;
  int    fPdgID = INV_INT;
  double fTrackLength = INV_DBL;
  double fEndX = INV_DBL;
  double fEndY = INV_DBL;
  double fEndZ = INV_DBL;
  double fStartX = INV_DBL;
  double fStartY = INV_DBL;
  double fStartZ = INV_DBL;
  double fRecoTrackID = INV_DBL;
  double fTEndX = INV_DBL;
  double fTEndY = INV_DBL;
  double fTEndZ = INV_DBL;
  double fTStartX = INV_DBL;
  double fTStartY = INV_DBL;
  double fTStartZ = INV_DBL;
  double fTStartT = INV_DBL;
  double fTEndT = INV_DBL;
  double fT0_reco = INV_DBL;
  double fTrackID = INV_DBL;
  double ftheta_xz = INV_DBL;
  double ftheta_yz = INV_DBL;
  double fMinHitPeakTime = INV_DBL;
  double fMaxHitPeakTime = INV_DBL;
  bool fIsRecoSelectedCathodeCrosser = false;
  bool fIsTrueSelectedCathodeCrosser = false;

  // Histos
  TH2D *h_dQdxVsRR;
  TH2D *h_dQdxVsRR_TP075;
};

MichelStudyTmp::MichelStudyTmp(fhicl::ParameterSet const & p)
  :
  EDAnalyzer(p)
{
  reconfigure(p);
}

void MichelStudyTmp::beginJob()
{
  art::ServiceHandle<art::TFileService> tfs;
  fTrackTree = tfs->make<TTree>("TrackTree", "track by track info");
  fTrackTree->Branch("event", &fEvNumber, "fEvNumber/l");
  fTrackTree->Branch("PdgID", &fPdgID);
  fTrackTree->Branch("trackLength", &fTrackLength, "fTrackLength/d");
  fTrackTree->Branch("endX", &fEndX, "fEndX/d");
  fTrackTree->Branch("endY", &fEndY, "fEndY/d");
  fTrackTree->Branch("endZ", &fEndZ, "fEndZ/d");
  fTrackTree->Branch("startX", &fStartX, "fStartX/d");
  fTrackTree->Branch("startY", &fStartY, "fStartY/d");
  fTrackTree->Branch("startZ", &fStartZ, "fStartZ/d");
  fTrackTree->Branch("recoTrackID", &fRecoTrackID);
  fTrackTree->Branch("TEndX", &fTEndX, "fTEndX/d");
  fTrackTree->Branch("TEndY", &fTEndY, "fTEndY/d");
  fTrackTree->Branch("TEndZ", &fTEndZ, "fTEndZ/d");
  fTrackTree->Branch("TStartX", &fTStartX, "fStartX/d");
  fTrackTree->Branch("TStartY", &fTStartY, "fTStartY/d");
  fTrackTree->Branch("TStartZ", &fTStartZ, "fTStartZ/d");
  fTrackTree->Branch("TStartT", &fTStartT, "fTStartT/d");
  fTrackTree->Branch("TEndT", &fTEndT, "fTEndT/d");
  fTrackTree->Branch("trackID", &fTrackID, "fTrackID/d");
  fTrackTree->Branch("T0_reco", &fT0_reco, "fT0_reco/d");
  fTrackTree->Branch("minHitPeakTime", &fMinHitPeakTime, "fMinHitPeakTime/d");
  fTrackTree->Branch("maxHitPeakTime", &fMaxHitPeakTime, "fMaxHitPeakTime/d");
  fTrackTree->Branch("theta_xz", &ftheta_xz, "ftheta_xz/d");
  fTrackTree->Branch("theta_yz", &ftheta_yz, "ftheta_yz/d");
  fTrackTree->Branch("isRecoSelectedCathodeCrosser",&fIsRecoSelectedCathodeCrosser);
  fTrackTree->Branch("isTrueSelectedCathodeCrosser",&fIsTrueSelectedCathodeCrosser);

  // Histograms
  h_dQdxVsRR = tfs->make<TH2D>("h_dQdxVsRR","h_dQdxVsRR",200,0,200,800,0,800);
  h_dQdxVsRR_TP075 = tfs->make<TH2D>("h_dQdxVsRR_TP075","h_dQdxVsRR_TP075",200,0,200,800,0,800);
}

void MichelStudyTmp::endJob()
{
  mf::LogVerbatim("MichelStudyTmp") << "MichelStudyTmp finished job";
  std::cout << "Total number of events: " << counter_total_number_events << std::endl;
  std::cout << "Total number of tracks: " << counter_total_number_tracks << std::endl;
  std::cout << "Number of T0-tagged tracks: " << counter_T0_tagged_tracks << std::endl;
}

void MichelStudyTmp::reconfigure(fhicl::ParameterSet const& p)
{
  fTrackerTag = p.get<std::string>("TrackerTag");
  fPFParticleTag = p.get<std::string>("PFParticleTag");
  fSpacePointTag = p.get<std::string>("SpacePointTag");
  spAlg.reconfigure(p.get<fhicl::ParameterSet>("SpacePointAlg"));
  selectorAlg.reconfigure(p.get<fhicl::ParameterSet>("StoppingMuonSelectionAlg"));
  caloHelper.reconfigure(p.get<fhicl::ParameterSet>("CalorimetryHelper"));
}

void MichelStudyTmp::UpdateTTreeVariableWithTrackProperties(const trackProperties &trackInfo) {
  fEvNumber       = trackInfo.evNumber;
  fT0_reco        = trackInfo.trackT0;
  fStartX         = trackInfo.recoStartX;
  fStartY         = trackInfo.recoStartY;
  fStartZ         = trackInfo.recoStartZ;
  fEndX           = trackInfo.recoEndX;
  fEndY           = trackInfo.recoEndY;
  fEndZ           = trackInfo.recoEndZ;
  ftheta_xz       = trackInfo.theta_xz;
  ftheta_yz       = trackInfo.theta_yz;
  fMinHitPeakTime = trackInfo.minHitPeakTime;
  fMaxHitPeakTime = trackInfo.maxHitPeakTime;
  fTrackLength    = trackInfo.trackLength;
  fRecoTrackID    = trackInfo.trackID;
  fPdgID          = trackInfo.pdg;
  fTStartX        = trackInfo.trueStartX;
  fTStartY        = trackInfo.trueStartY;
  fTStartZ        = trackInfo.trueStartZ;
  fTEndX          = trackInfo.trueEndX;
  fTEndY          = trackInfo.trueEndY;
  fTEndZ          = trackInfo.trueEndZ;
  fTStartT        = trackInfo.trueStartT;;
  fTEndT          = trackInfo.trueEndT;
  fTrackID        = trackInfo.trueTrackID;
}

} // namespace
