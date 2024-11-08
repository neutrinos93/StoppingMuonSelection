/***
  Class containing useful functions for geometry.

*/
#ifndef TRUEDEDX_HELPER_H
#define TRUEDEDX_HELPER_H

#include "larcore/Geometry/Geometry.h"
#include "larcorealg/Geometry/TPCGeo.h"
#include "TVector3.h"
#include "TMath.h"
#include "TFile.h"
#include "TH1.h"

namespace stoppingcosmicmuonselection {

  class TruedEdxHelper {

  public:
    TruedEdxHelper();
    ~TruedEdxHelper();

    // Return MPV of dEdx according to landau-vavilov
    double LandauVav(double *x, double *p, const double &LArdensity);

    // Return MPV of dEdx according to landau-vavilov
    double LandauVav(double &resRange, const double &trackPitch, const double &LArdensity);

    // Get the dEdx from the MC simulation.
    double GetMCdEdx(const double &resRange);

    // Work out the density effect based on Sternheimer parametrization.
    double DensityEffect(const double &yb);

    // Get relativist beta given the kinetic energy
    double GetBetaFromkEnergy(const double &kEnergy);

    // Get beta-gamma relativist factor given the kinetic energy
    double GetBetaGammaFromKEnergy(const double &kEnergy);

    // Get kinetic energy from res range. (Wrap for the spline)
    double ResRangeToKEnergy(const double &resRange);

    // Definition of the spline to go from res range to kinetic energy for a muon
    double Spline3(const double &x);


  private:

    TH1D *_h_dEdx;

    const double m_muon = 105.6583745; //MeV
    const double c = 299792458;
  };
}

#endif
