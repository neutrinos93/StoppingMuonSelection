/***
  Class containing useful functions for hit on a plane.

*/
#ifndef HIT_PLANE_ALG_CXX
#define HIT_PLANE_ALG_CXX

#include "HitPlaneAlg.h"

namespace stoppingcosmicmuonselection {

  HitPlaneAlg::HitPlaneAlg(artPtrHitVec &hitsOnPlane,
                           const size_t &start_index,
                           const size_t &planeNumber) :
                           _hitsOnPlane(hitsOnPlane),
                           _start_index(start_index),
                           _planeNumber(planeNumber) {
    OrderHitVec();
  }

  HitPlaneAlg::~HitPlaneAlg() {

  }

  // Order hits based on their 2D (wire-time) position.
  void HitPlaneAlg::OrderHitVec() {
    artPtrHitVec newVector;
    newVector.reserve(_hitsOnPlane.size());
    newVector.push_back(_hitsOnPlane.at(_start_index));
    _hitsOnPlane.erase(_hitsOnPlane.begin() + _start_index);

    double min_dist = DBL_MAX;
    int min_index = -1;

    while (_hitsOnPlane.size() != 0) {

      min_dist = DBL_MAX;
      min_index = -1;

      for (size_t i = 0; i < _hitsOnPlane.size(); i++) {
        // For previous hit.
        double hitPeakTime = newVector.back()->PeakTime();
        unsigned int wireID = newVector.back()->WireID().Wire;
        size_t wireOffset = geoHelper.GetWireOffset(newVector.back(), _planeNumber);
        TVector3 pt1(hitPeakTime,wireID+wireOffset,0);
        if (i==0) {
          _effectiveWireID.push_back(wireID+wireOffset);
        }
        // For current hit.
        double hitPeakTime2 = _hitsOnPlane.at(i)->PeakTime();
        unsigned int wireID2 = _hitsOnPlane.at(i)->WireID().Wire;
        size_t wireOffset2 = geoHelper.GetWireOffset(_hitsOnPlane.at(i), _planeNumber);
        TVector3 pt2(hitPeakTime2,wireID2+wireOffset2,0);
        double dist = (pt1-pt2).Mag();
        if (dist < min_dist) {
          min_index = i;
          min_dist = dist;
        }
      }
      auto const &hit = _hitsOnPlane.at(min_index);
      newVector.push_back(hit);
      _effectiveWireID.push_back(hit->WireID().Wire + geoHelper.GetWireOffset(hit,_planeNumber));
      _hitsOnPlane.erase(_hitsOnPlane.begin() + min_index);
    }
    _isOrdered = true;
    std::swap(_hitsOnPlane, newVector);
    return;
  }

  // Get the ordered hit vector.
  const artPtrHitVec HitPlaneAlg::OrderedHitVec() {
    if (!_isOrdered)
      OrderHitVec();
    return _hitsOnPlane;
  }

  // Work out the vector of ordered dQds.
  const std::vector<double> HitPlaneAlg::GetOrderedDqds() {
    if (!_isOrdered)
      OrderHitVec();
    std::vector<double> dQds;
    double ds;

    for (size_t i = 0; i < _hitsOnPlane.size()-1; i++) {
      auto const &hit = _hitsOnPlane[i];
      auto const &nextHit = _hitsOnPlane[i+1];
      double XThisPoint = detprop->ConvertTicksToX(hit->PeakTime(),hit->WireID().Plane,hit->WireID().TPC,hit->WireID().Cryostat);
      double XNextPoint = detprop->ConvertTicksToX(nextHit->PeakTime(),nextHit->WireID().Plane,nextHit->WireID().TPC,nextHit->WireID().Cryostat);

      TVector3 thisPoint(_effectiveWireID[i]*geoHelper.GetWirePitch(_planeNumber), XThisPoint, 0);
      TVector3 nextPoint(_effectiveWireID[i+1]*geoHelper.GetWirePitch(_planeNumber), XNextPoint, 0);

      ds = (thisPoint - nextPoint).Mag();
      dQds.push_back(hit->Integral() / ds);
    }

    // Need final point.
    dQds.push_back(_hitsOnPlane.at(_hitsOnPlane.size())->Integral() / ds);

    return dQds;
  }

} // end of namespace stoppingcosmicmuonselection

#endif
