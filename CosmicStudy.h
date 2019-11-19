///////////////////////////////////////////////////////////////////////
// Class:       CosmicStudy
// Plugin Type: ******
// File:        CosmicStudy.h
////////////////////////////////////////////////////////////////////////
#include "dune/Protodune/Analysis/ProtoDUNEPFParticleUtils.h"
#include "dune/Protodune/Analysis/ProtoDUNETruthUtils.h"
#include "art_root_io/TFileService.h"
//#include "messagefacility/MessageLogger/MessageLogger.h"
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Core/FileBlock.h"
#include "canvas/Persistency/Common/FindManyP.h"
#include "larcore/Geometry/Geometry.h"
#include "larcorealg/Geometry/TPCGeo.h"
#include "larcorealg/Geometry/GeometryCore.h"
#include "lardata/DetectorInfoServices/DetectorPropertiesService.h"

#include <fstream>
#include <string>
#include "TTree.h"
#include "TH1.h"
#include "TH2.h"
#include "TProfile2D.h"
#include "TMath.h"
#include "TGraphErrors.h"
#include "TGraph2D.h"

#include "DataTypes.h"
#include "GeometryHelper.h"
#include "StoppingMuonSelectionAlg.h"

namespace stoppingcosmicmuonselection {

class CosmicStudy;

class CosmicStudy : public art::EDAnalyzer {
public:
  explicit CosmicStudy(fhicl::ParameterSet const & p);
  // The destructor generated by the compiler is fine for classes
  // without bare pointers or other resource use.

  // Plugins should not be copied or assigned.
  CosmicStudy(CosmicStudy const &) = delete;
  CosmicStudy(CosmicStudy &&) = delete;
  CosmicStudy & operator = (CosmicStudy const &) = delete;
  CosmicStudy & operator = (CosmicStudy &&) = delete;

  // Required functions.
  void analyze(art::Event const &evt) override;

  // Selected optional functions
  void beginJob() override;
  void endJob() override;
  void reconfigure(fhicl::ParameterSet const& p);

  size_t numberOfEvents = 0;
  size_t numberOfStoppingMuons = 0;
  size_t numberOfT0TaggedMuons = 0;
  size_t numberOfStoppingT0CCMuons = 0;
  size_t numberOfStoppingT0ACMuons = 0;

private:

  // Utils
  protoana::ProtoDUNEPFParticleUtils   pfpUtil;
  protoana::ProtoDUNETruthUtils        truthUtil;
  StoppingMuonSelectionAlg selectorAlg;
  GeometryHelper           geoHelper;

  // Handle for geometry service
  const geo::GeometryCore *geom = lar::providerFrom<geo::Geometry>();
  // Declare handle for detector properties
  const detinfo::DetectorProperties *detprop = lar::providerFrom<detinfo::DetectorPropertiesService>();

};

void CosmicStudy::beginJob()
{
  // Print some detector properties
  std::cout << "**************************  " << std::endl
            << "SamplingRate: " << detprop->SamplingRate() << std::endl
            << "NumberTimeSamples: " << detprop->NumberTimeSamples() << std::endl
            << "Window size: " << detprop->SamplingRate()*detprop->NumberTimeSamples()/1000. << std::endl
            << "ReadOutWindowSize: " << detprop->ReadOutWindowSize() << std::endl
            << "Drift velocity: " << detprop->DriftVelocity()*1e-3 << " cm/ns" << std::endl
            << "Trigger Offset: " << detprop->TriggerOffset() << std::endl
            << "Electric Field: " << detprop->Efield() << std::endl
            << "Temperature: " << detprop->Temperature() << std::endl
            << "Density: " << detprop->Density(detprop->Temperature()) << std::endl
            << "**************************  " << std::endl;

  for (geo::PlaneID const &pID : geom->IteratePlaneIDs()) {
    geo::PlaneGeo const& planeHandle = geom->Plane(pID);
    std::cout << "Plane ID: " << pID.Plane << "| Coordinates: x=" << planeHandle.GetCenter().X() << " y=" << planeHandle.GetCenter().Y() << " z=" << planeHandle.GetCenter().Z() << std::endl;
  }

}

void CosmicStudy::endJob()
{
  mf::LogVerbatim("CosmicStudy") << "CosmicStudy finished job";
  std::cout << "Total number of events: " << numberOfEvents << std::endl;
  std::cout << "Number of stopping muons: " << numberOfStoppingMuons << std::endl;
  std::cout << "Number of T0-tagged muons: " << numberOfT0TaggedMuons << std::endl;
  std::cout << "Number of Stopping T0-tagged Cathode-crossing muons: " << numberOfStoppingT0CCMuons << std::endl;
  std::cout << "Number of Stopping T0-tagged Anode-crossing muons: " << numberOfStoppingT0ACMuons << std::endl;
}

CosmicStudy::CosmicStudy(fhicl::ParameterSet const & p)
  :
  EDAnalyzer(p)
{
  reconfigure(p);
}

void CosmicStudy::reconfigure(fhicl::ParameterSet const& p)
{
  selectorAlg.reconfigure(p);
}




} // namespace
