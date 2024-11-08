/***
  Class containing useful functions for calibration.

*/
#ifndef CALIBRATION_HELPER_CXX
#define CALIBRATION_HELPER_CXX

#include "CalibrationHelper.h"

namespace stoppingcosmicmuonselection {

  CalibrationHelper::CalibrationHelper() {
  }

  CalibrationHelper::~CalibrationHelper() {

  }

  // Get the histos.
  void CalibrationHelper::Set(art::Event const &evt) {
    auto const clockData = art::ServiceHandle<detinfo::DetectorClocksService>()->DataFor(evt);
    auto const detprop = art::ServiceHandle<detinfo::DetectorPropertiesService>()->DataFor(evt, clockData);
    sceHelper = new SceHelper(detprop);
    drift_velocity = detprop.DriftVelocity()*1e-3;

    std::string filetype;
    std::string filenameX;
    std::string filenameYZ;
    if (!evt.isRealData()) {
      filetype = "sce";
      filenameX = "Xcalo_prod4_sceon.root";
      filenameYZ = "YZcalo_prod4_sceon.root";
    }
    else {
      size_t runNumber = evt.id().run();
      filetype = "r" + std::to_string(runNumber);
      filenameX = "Xcalo_r" + std::to_string(runNumber) + ".root";
      filenameYZ = "YZcalo_r" + std::to_string(runNumber) + ".root";
    }
    std::cout << "CalibrationHelper.cxx: filenameX = " << filenameX <<std::endl;
    std::cout << "CalibrationHelper.cxx: filenameYZ = " << filenameYZ <<std::endl;
    TFile fileX(filenameX.c_str());
    TFile fileYZ(filenameYZ.c_str());

    h_x = (TH1D*)fileX.Get("dqdx_X_correction_hist_2");
    h_yz_neg = (TH2D*)fileYZ.Get("correction_dqdx_ZvsY_negativeX_hist_2");
    h_yz_pos = (TH2D*)fileYZ.Get("correction_dqdx_ZvsY_positiveX_hist_2");
  }

  // Get X correction factor.
  double CalibrationHelper::GetXCorr(const TVector3 &hitPos) {
    return h_x->GetBinContent(h_x->FindBin(hitPos.X()));
  }

  // Get YZ correction factor.
  double CalibrationHelper::GetYZCorr(const TVector3 &hitPos) {
    double factor = INV_DBL;

    if (hitPos.X() > 0)
      factor = h_yz_pos->GetBinContent(h_yz_pos->FindBin(hitPos.Z(),hitPos.Y()));
    else
      factor = h_yz_neg->GetBinContent(h_yz_neg->FindBin(hitPos.Z(),hitPos.Y()));

    return factor;
  }

  // Get both factors at the same time.
  double CalibrationHelper::GetXYZCorr(const TVector3 &hitPos) {
    return GetXCorr(hitPos)*GetYZCorr(hitPos);
  }

  // Get vector of factors for X.
  std::vector<double> CalibrationHelper::GetXCorr_V(const std::vector<double> &hit_xs) {
    std::vector<double> result;

    for (auto const &x : hit_xs) {
      TVector3 hitPos(x, 0, 0);
      result.push_back(GetXCorr(hitPos));
    }

    return result;
  }

  // Get vector of factors for YZ.
  std::vector<double> CalibrationHelper::GetYZCorr_V(const std::vector<double> &hit_xs, const std::vector<double> &hit_ys, const std::vector<double> &hit_zs) {
    std::vector<double> result;

    for (size_t id = 0; id < hit_xs.size(); id++) {
      TVector3 hitPos(hit_xs[id], hit_ys[id], hit_zs[id]);
      result.push_back(GetYZCorr(hitPos));
    }

    return result;
  }

  void CalibrationHelper::LifeTimeCorrNew(double &dQdx, const double &hitX, const art::Event &evt)  {
    auto const clockData = art::ServiceHandle<detinfo::DetectorClocksService>()->DataFor(evt);
    auto const detProp = art::ServiceHandle<detinfo::DetectorPropertiesService const>()->DataFor(evt);
    double vDrift = detProp.DriftVelocity(); //cm/us
    double xAnode = std::abs(detProp.ConvertTicksToX(trigger_offset(clockData),0,0,0));
    std::cout << "X ANODE: " << xAnode << std::endl;
    double fLifetime = INV_DBL;

    if (evt.isRealData()) {
      // Electron lifetime from database calibration service provider
      art::ServiceHandle<calib::LifetimeCalibService> lifetimecalibHandler;
      calib::LifetimeCalibService & lifetimecalibService = *lifetimecalibHandler;
      calib::LifetimeCalib *lifetimecalib = lifetimecalibService.provider();
      fLifetime = lifetimecalib->GetLifetime()*1000.0; // [ms]*1000.0 -> [us]
      std::cout << "LIFETIME: " << fLifetime << std::endl;
      //fLifetime = 17518.3; // us?
    }
    else {
      fLifetime = detProp.ElectronLifetime();
    }
    dQdx = dQdx * TMath::Exp((xAnode-std::abs(hitX))/(fLifetime*vDrift));
  }

  double CalibrationHelper::GetLifeTimeCorrFactor(const double &lt, const double &hitX, const art::Event &evt) {
    
    auto const clockData = art::ServiceHandle<detinfo::DetectorClocksService>()->DataFor(evt);
    auto const detProp = art::ServiceHandle<detinfo::DetectorPropertiesService const>()->DataFor(evt);
    double vDrift = detProp.DriftVelocity(); //cm/us
    double xAnode = std::abs(detProp.ConvertTicksToX(trigger_offset(clockData),0,0,0));
    
    return TMath::Exp((xAnode-std::abs(hitX))/(lt * vDrift));
  }

  // Get vector of directions.
  std::vector<TVector3> CalibrationHelper::GetHitDirVec(const std::vector<double> &hit_x, const std::vector<double> &hit_y, const std::vector<double> &hit_z) {

    std::vector<TVector3> dirs;
    if (hit_x[0]!=INV_DBL && hit_y[0]!=INV_DBL && hit_z[0]!=INV_DBL)
      dirs.push_back(TVector3(hit_x[1]-hit_x[0], hit_y[1]-hit_y[0], hit_z[1]-hit_z[0]));
    else
      dirs.push_back(TVector3(INV_DBL, INV_DBL, INV_DBL));

    for (size_t i = 1; i < hit_x.size(); i++) {
      if (hit_x[i]==INV_DBL || hit_y[i]==INV_DBL || hit_z[i]==INV_DBL)
        dirs.push_back(TVector3(INV_DBL, INV_DBL, INV_DBL));
      else {
        dirs.push_back(TVector3(hit_x[i]-hit_x[i-1], hit_y[i]-hit_y[i-1], hit_z[i]-hit_z[i-1]));
      }
    }

    return dirs;

  }

  // Get vector of Fields.
  std::vector<TVector3> CalibrationHelper::GetHitPosField(const std::vector<double> &hit_x, const std::vector<double> &hit_y, const std::vector<double> &hit_z) {

    std::vector<TVector3> fields;

    for (size_t i = 0; i < hit_x.size(); i++) {
      if (hit_x[i]==INV_DBL || hit_y[i]==INV_DBL || hit_z[i]==INV_DBL)
        fields.push_back(TVector3(INV_DBL, INV_DBL, INV_DBL));
      else {
        fields.push_back(sceHelper->GetFieldVector(TVector3(hit_x[i], hit_y[i], hit_z[i])));
      }
    }

    return fields;

  }

  // Get vector of angles phi.
  std::vector<double> CalibrationHelper::PitchFieldAngle(const std::vector<double> &hit_xs, const std::vector<double> &hit_ys, const std::vector<double> &hit_zs) {
    std::vector<double> phis;

    std::vector<TVector3> dirs = GetHitDirVec(hit_xs, hit_ys, hit_zs);
    std::vector<TVector3> fields = GetHitPosField(hit_xs, hit_ys, hit_zs);

    for (size_t i = 0; i < TMath::Min(dirs.size(), fields.size()); i++) {
      if (dirs[i].X()==INV_DBL || dirs[i].Y()==INV_DBL || dirs[i].Z()==INV_DBL || fields[i].X()==INV_DBL || fields[i].Y()==INV_DBL || fields[i].Z()==INV_DBL)
        phis.push_back(INV_DBL);
      else {
        double phi = dirs[i].Angle(fields[i]);
        phis.push_back(phi);
      }
    }

    return phis;

  }

  void CalibrationHelper::CorrectXPosition(std::vector<double> &hit_xs, const double &startX, const double &endX, const double &t0) {

    if (startX <= endX) {
      if (startX <= 0) {
        for(size_t i=0; i<hit_xs.size(); i++) {
          hit_xs[i] = hit_xs[i] - (drift_velocity * t0);
        }
      }
    }
    else {
      if (startX >= 0) {
        for(size_t i=0; i<hit_xs.size(); i++) {
          hit_xs[i] = hit_xs[i] + (drift_velocity * t0);
        }
      }
    }
  }

} // end of namespace stoppingcosmicmuonselection

#endif
