/***
  Class containing useful functions for SCE correction.

*/
#ifndef SCE_HELPER_CXX
#define SCE_HELPER_CXX

#include "SceHelper.h"

namespace stoppingcosmicmuonselection {

  SceHelper::SceHelper() {
    if (!sce->EnableCalSpatialSCE())
      std::cout << "SceHelper.cxx: ATTENTION - SCE is not enabled!" << std::endl;
  }

  SceHelper::~SceHelper() {

  }

  // Get corrected position given TVector3
  TVector3 SceHelper::GetCorrectedPos(const TVector3 &pos) {
    // Code taken from Calorimetry module.
    geo::Vector_t locOffsets = {0., 0., 0.,};
    geo::Point_t loc{pos.X(), pos.Y(), pos.Z()};

    // Get TPC index.
    unsigned int tpc = geoHelper.GetTPCFromPosition(pos);
    if (tpc == -INV_INT) return TVector3(INV_DBL, INV_DBL, INV_DBL);

    locOffsets = sce->GetCalPosOffsets(loc, tpc);

    TVector3 res(loc.X() - locOffsets.X(), loc.Y() + locOffsets.Y(), loc.Z() + locOffsets.Z());

    return res;
  }

  // Get corrected field vector at point.
  TVector3 SceHelper::GetFieldVector(const TVector3 &pos) {
    geo::Point_t loc{pos.X(), pos.Y(), pos.Z()};
    // Get TPC index.
    unsigned int tpc = geoHelper.GetTPCFromPosition(pos);
    if (tpc == -INV_INT) return TVector3(INV_DBL, INV_DBL, INV_DBL);
    geo::Vector_t E_field_offsets = {0., 0., 0.};
    double E_field_nominal = detprop->Efield();   // Electric Field in the drift region in KV/cm

    E_field_offsets = sce->GetCalEfieldOffsets(loc, tpc);
    TVector3 E_field_vector = {E_field_nominal*(1 + E_field_offsets.X()), E_field_nominal*E_field_offsets.Y(), E_field_nominal*E_field_offsets.Z()};

    return E_field_vector;
  }


} // end of namespace stoppingcosmicmuonselection

#endif