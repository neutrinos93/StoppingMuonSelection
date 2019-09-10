#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#include "TVector3.h"

namespace stoppingcosmicmuonselection {

  const int INV_INT = -999;
  const double INV_DBL = -9999999;

  struct trackProperties {
    // Reconstructed information
    size_t evNumber;
    double trackT0;
    TVector3 recoStartPoint;
    TVector3 recoEndPoint;
    double theta_xz, theta_yz;
    double minHitPeakTime, maxHitPeakTime;
    double trackLength;
    double trackID;

    // Truth information
    int pdg;
    TVector3 trueStartPoint;
    TVector3 trueEndPoint;
    double trueStartT, trueEndT;
    double trueTrackID;

    void Reset() {
      evNumber = INV_INT;
      trackT0 = INV_DBL;
      recoStartPoint.SetXYZ(INV_DBL,INV_DBL,INV_DBL);
      recoEndPoint.SetXYZ(INV_DBL,INV_DBL,INV_DBL);
      theta_xz = INV_DBL;
      theta_yz = INV_DBL;
      minHitPeakTime = INV_DBL;
      maxHitPeakTime = INV_DBL;
      trackLength = INV_DBL;
      trackID = INV_DBL;
      pdg = INV_INT;
      trueStartPoint.SetXYZ(INV_DBL,INV_DBL,INV_DBL);
      trueEndPoint.SetXYZ(INV_DBL,INV_DBL,INV_DBL);
      trueStartT = INV_DBL;
      trueEndT = INV_DBL;
      trueTrackID = INV_DBL;
    }
  };

}

#endif
