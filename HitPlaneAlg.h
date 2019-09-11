/***
  Class containing useful algorithms for hit on a plane.

*/
#ifndef HIT_PLANE_ALG_H
#define HIT_PLANE_ALG_H

#include "TVector3.h"
#include "TMath.h"

#include "DataTypes.h"
#include "HitHelper.h"

namespace stoppingcosmicmuonselection {

  class HitPlaneAlg {

  public:
    HitPlaneAlg(artPtrHitVec &hitsOnPlane, const size_t start_index);
    ~HitPlaneAlg();

    // Order hits based on their 2D (wire-time) position.
    const artPtrHitVec GetOrderedHitArray();

  private:

    artPtrHitVec &_hitsOnPlane;
    const size_t &_start_index;

  };
}

#endif
