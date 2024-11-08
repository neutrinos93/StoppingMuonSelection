/***
  Class containing useful functions for SCE correction.

*/
#ifndef SCE_HELPER_H
#define SCE_HELPER_H

#include "protoduneana/protoduneana/Utilities/ProtoDUNETrackUtils.h"
#include "protoduneana/protoduneana/Utilities/ProtoDUNEPFParticleUtils.h"
#include "fhiclcpp/ParameterSet.h"
#include "lardataobj/RecoBase/Hit.h"
#include "lardataobj/RecoBase/Track.h"
#include "lardataobj/RecoBase/Cluster.h"
#include "lardataobj/RecoBase/PFParticle.h"
#include "larreco/Calorimetry/CalorimetryAlg.h"
#include "lardataobj/AnalysisBase/Calorimetry.h"
#include "larreco/RecoAlg/PMAlg/Utilities.h"
#include "larcore/Geometry/Geometry.h"
#include "larcorealg/Geometry/TPCGeo.h"
#include "larcorealg/Geometry/GeometryCore.h"
#include "lardata/DetectorInfoServices/DetectorPropertiesService.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "larevt/SpaceChargeServices/SpaceChargeService.h"
#include "TVector3.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TFile.h"
#include "TMath.h"

#include "DataTypes.h"
#include "GeometryHelper.h"

namespace stoppingcosmicmuonselection {

  class SceHelper {

  public:
    SceHelper();
    SceHelper(detinfo::DetectorPropertiesData const& detprop);
    ~SceHelper();

    // Get corrected position given TVector3
    TVector3 GetCorrectedPos(const TVector3 &pos);

    // Get corrected field vector at point.
    TVector3 GetFieldVector(const TVector3 &pos);

  private:

    GeometryHelper geoHelper;
    // Handle for space charge service
    const spacecharge::SpaceCharge* sce = lar::providerFrom<spacecharge::SpaceChargeService>();
    double _Efield;
  };
}

#endif
