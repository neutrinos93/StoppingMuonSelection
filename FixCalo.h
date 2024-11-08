
#ifndef FIX_CALO_H
#define FIX_CALO_H
#include <math.h>
#include <string>

#include "larcoreobj/SimpleTypesAndConstants/PhysicalConstants.h"
#include "lardata/DetectorInfoServices/DetectorClocksService.h"
#include "lardata/DetectorInfoServices/DetectorPropertiesService.h"
#include "larreco/Calorimetry/CalorimetryAlg.h"

#include "larcorealg/CoreUtils/NumericUtils.h" // util::absDiff()
#include "larcorealg/Geometry/PlaneGeo.h"
#include "larcorealg/Geometry/WireGeo.h"
#include "lardata/ArtDataHelper/TrackUtils.h" // lar::util::TrackPitchInView()
#include "lardata/Utilities/AssociationUtil.h"
#include "lardataobj/AnalysisBase/Calorimetry.h"
#include "lardataobj/AnalysisBase/T0.h"
#include "lardataobj/RecoBase/Hit.h"
#include "lardataobj/RecoBase/SpacePoint.h"
#include "lardataobj/RecoBase/Track.h"
#include "lardataobj/RecoBase/TrackHitMeta.h"
#include "larevt/CalibrationDBI/Interface/ChannelStatusProvider.h"
#include "larevt/CalibrationDBI/Interface/ChannelStatusService.h"

#include "larevt/SpaceCharge/SpaceCharge.h"
#include "larevt/SpaceChargeServices/SpaceChargeService.h"

#include "protoduneana/StoppingMuonSelection/CalibrationHelper.h"
// ROOT includes
#include <TF1.h>
#include <TGraph.h>
#include <TMath.h>
#include <TVector3.h>

// Framework includes
#include "art/Framework/Core/EDProducer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "canvas/Persistency/Common/FindManyP.h"
#include "canvas/Persistency/Common/Ptr.h"
#include "cetlib/pow.h" // cet::sum_of_squares()
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

namespace stoppingcosmicmuonselection {

  class FixCalo {
  public:
    FixCalo();
    ~FixCalo();

    const std::vector<std::vector<double>> GetRightCalo(const art::Event& evt, const double &T0, const recob::Track &track);
    void GetPitch(detinfo::DetectorPropertiesData const& detprop,
                                art::Ptr<recob::Hit> const& hit,
                                std::vector<double> const& trkx,
                                std::vector<double> const& trky,
                                std::vector<double> const& trkz,
                                std::vector<double> const& trkw,
                                std::vector<double> const& trkx0,
                                double* xyz3d,
                                double& pitch,
                                double TickT0);
  private:
    CalibrationHelper calibHelper;

  };

  FixCalo::FixCalo() {}
  FixCalo::~FixCalo() {}

  //------------------------------------------------------------------------------------//
  const std::vector<std::vector<double>> FixCalo::GetRightCalo(const art::Event& evt, const double &T0, const recob::Track &track)
  {
    std::vector<std::vector<double>> to_be_returned;
    std::vector<double> XX, YY, ZZ;

    std::string fTrackModuleLabel = "pandoraTrack";
    std::string fSpacePointModuleLabel = "pandora";
    bool fUseArea = true;
    bool fSCE = true;
    bool fFlipTrack_dQdx = false;
    //bool fNotOnTrackZcut = false;
    int fnsps;
    std::vector<int>    fwire;
    std::vector<double> ftime;
    std::vector<double> fstime;
    std::vector<double> fetime;
    std::vector<double> fMIPs;
    std::vector<double> fdQdx;
    std::vector<double> fdEdx;
    std::vector<double> fResRng;
    std::vector<double> fpitch;
    std::vector<TVector3> fXYZ;
    std::vector<size_t> fHitIndex;

    auto const clock_data = art::ServiceHandle<detinfo::DetectorClocksService const>()->DataFor(evt);
    auto const detprop =
      art::ServiceHandle<detinfo::DetectorPropertiesService const>()->DataFor(evt, clock_data);
    auto const* sce = lar::providerFrom<spacecharge::SpaceChargeService>();

    double drift_velocity = detprop.DriftVelocity()*1e-3; // cm/ns
    art::Handle<std::vector<recob::Track>> trackListHandle;
    std::vector<art::Ptr<recob::Track>> tracklist;
    if (evt.getByLabel(fTrackModuleLabel, trackListHandle))
      art::fill_ptr_vector(tracklist, trackListHandle);

    // Get Geometry
    art::ServiceHandle<geo::Geometry const> geom;

    // channel quality
    lariov::ChannelStatusProvider const& channelStatus =
      art::ServiceHandle<lariov::ChannelStatusService const>()->GetProvider();

    size_t nplanes = geom->Nplanes();

        art::FindManyP<recob::Hit> fmht(trackListHandle, evt, fTrackModuleLabel);
    art::FindManyP<recob::Hit, recob::TrackHitMeta> fmthm(
      trackListHandle,
      evt,
      fTrackModuleLabel); //this has more information about hit-track association, only available in PMA for now


    for (size_t trkIter = 0; trkIter < tracklist.size(); ++trkIter) {
      if (tracklist[trkIter]->ID()!=track.ID()) continue;
      decltype(auto) larEnd = tracklist[trkIter]->Trajectory().End();

      // Some variables for the hit
      float time;             //hit time at maximum
      float stime;            //hit start time
      float etime;            //hit end time
      uint32_t channel = 0;   //channel number
      unsigned int cstat = 0; //hit cryostat number
      unsigned int tpc = 0;   //hit tpc number
      unsigned int wire = 0;  //hit wire number
      unsigned int plane = 0; //hit plane number

      std::vector<art::Ptr<recob::Hit>> allHits = fmht.at(trkIter);
      double TickT0 =0;
      TickT0 = T0 / sampling_rate(clock_data);

      std::vector<std::vector<unsigned int>> hits(nplanes);

      art::FindManyP<recob::SpacePoint> fmspts(allHits, evt, fSpacePointModuleLabel);
      for (size_t ah = 0; ah < allHits.size(); ++ah) {
        hits[allHits[ah]->WireID().Plane].push_back(ah);
      }
      //get hits in each plane
      for (size_t ipl = 0; ipl < nplanes; ++ipl) { //loop over all wire planes
        if (ipl != 2) continue;
        geo::PlaneID planeID; //(cstat,tpc,ipl);

        fwire.clear();
        ftime.clear();
        fstime.clear();
        fetime.clear();
        fMIPs.clear();
        fdQdx.clear();
        fdEdx.clear();
        fpitch.clear();
        fResRng.clear();
        fXYZ.clear();
        fHitIndex.clear();

        float Kin_En = 0.;
        float Trk_Length = 0.;
        std::vector<double> vdEdx;
        std::vector<double> vresRange;
        std::vector<double> vdQdx;
        std::vector<double> deadwire; //residual range for dead wires
        std::vector<TVector3> vXYZ;

        // Require at least 2 hits in this view
        if (hits[ipl].size() < 2) {
          if (hits[ipl].size() == 1) {
            mf::LogWarning("Calorimetry")
              << "Only one hit in plane " << ipl << " associated with track id " << trkIter;
          }
          return to_be_returned;
        }

        //range of wire signals
        unsigned int wire0 = 100000;
        unsigned int wire1 = 0;
        double PIDA = 0;
        int nPIDA = 0;

        // determine track direction. Fill residual range array
        bool GoingDS = true;
        // find the track direction by comparing US and DS charge BB
        double USChg = 0;
        double DSChg = 0;
        // temp array holding distance betweeen space points
        std::vector<double> spdelta;
        fnsps = 0; // number of space points
        std::vector<double> ChargeBeg;
        std::stack<double> ChargeEnd;

        // find track pitch
        double fTrkPitch = 0;
        for (size_t itp = 0; itp < tracklist[trkIter]->NumberTrajectoryPoints(); ++itp) {

          const auto& pos_tmp = tracklist[trkIter]->LocationAtPoint(itp);
          const auto& dir = tracklist[trkIter]->DirectionAtPoint(itp);

          double newX;
          if (pos_tmp.X()>0) {
            newX = pos_tmp.X() + (drift_velocity * T0);
          }
          else {
            newX = pos_tmp.X() - (drift_velocity * T0);
          }
          geo::Point_t pos{newX, pos_tmp.Y(), pos_tmp.Z()};
          const double Position[3] = {pos.X(), pos.Y(), pos.Z()};
          geo::TPCID tpcid = geom->FindTPCAtPosition(Position);
          if (tpcid.isValid) {
            try {
              fTrkPitch =
                lar::util::TrackPitchInView(*tracklist[trkIter], geom->Plane(ipl).View(), itp);

              //Correct for SCE
              geo::Vector_t posOffsets = {0., 0., 0.};
              geo::Vector_t dirOffsets = {0., 0., 0.};
              if (sce->EnableCalSpatialSCE() && fSCE)
                posOffsets = sce->GetCalPosOffsets(geo::Point_t(pos), tpcid.TPC);
              if (sce->EnableCalSpatialSCE() && fSCE)
                dirOffsets = sce->GetCalPosOffsets(geo::Point_t{pos.X() + fTrkPitch * dir.X(),
                                                                pos.Y() + fTrkPitch * dir.Y(),
                                                                pos.Z() + fTrkPitch * dir.Z()},
                                                   tpcid.TPC);
              TVector3 dir_corr = {fTrkPitch * dir.X() - dirOffsets.X() + posOffsets.X(),
                                   fTrkPitch * dir.Y() + dirOffsets.Y() - posOffsets.Y(),
                                   fTrkPitch * dir.Z() + dirOffsets.Z() - posOffsets.Z()};

              fTrkPitch = dir_corr.Mag();
            }
            catch (cet::exception& e) {
              mf::LogWarning("Calorimetry")
                << "caught exception " << e << "\n setting pitch (C) to " << util::kBogusD;
              fTrkPitch = 0;
            }
            break;
          }
        }

        // find the separation between all space points
        double xx = 0., yy = 0., zz = 0.;

        //save track 3d points
        std::vector<double> trkx;
        std::vector<double> trky;
        std::vector<double> trkz;
        std::vector<double> trkw;
        std::vector<double> trkx0;
        for (size_t i = 0; i < hits[ipl].size(); ++i) {
          //Get space points associated with the hit
          std::vector<art::Ptr<recob::SpacePoint>> sptv = fmspts.at(hits[ipl][i]);
          for (size_t j = 0; j < sptv.size(); ++j) {

            double t = allHits[hits[ipl][i]]->PeakTime() -
                       TickT0; // Want T0 here? Otherwise ticks to x is wrong?
            double x = detprop.ConvertTicksToX(t,
                                                allHits[hits[ipl][i]]->WireID().Plane,
                                                allHits[hits[ipl][i]]->WireID().TPC,
                                                allHits[hits[ipl][i]]->WireID().Cryostat);
            double w = allHits[hits[ipl][i]]->WireID().Wire;
            if (TickT0) {
              trkx.push_back(sptv[j]->XYZ()[0] -
                             detprop.ConvertTicksToX(TickT0,
                                                      allHits[hits[ipl][i]]->WireID().Plane,
                                                      allHits[hits[ipl][i]]->WireID().TPC,
                                                      allHits[hits[ipl][i]]->WireID().Cryostat));
            }
            else {
              trkx.push_back(sptv[j]->XYZ()[0]);
            }
            trky.push_back(sptv[j]->XYZ()[1]);
            trkz.push_back(sptv[j]->XYZ()[2]);
            trkw.push_back(w);
            trkx0.push_back(x);
          }
        }
        for (size_t ihit = 0; ihit < hits[ipl].size();
             ++ihit) { // loop over all hits on each wire plane

          if (!planeID.isValid) {
            plane = allHits[hits[ipl][ihit]]->WireID().Plane;
            tpc = allHits[hits[ipl][ihit]]->WireID().TPC;
            cstat = allHits[hits[ipl][ihit]]->WireID().Cryostat;
            planeID.Cryostat = cstat;
            planeID.TPC = tpc;
            planeID.Plane = plane;
            planeID.isValid = true;
          }

          wire = allHits[hits[ipl][ihit]]->WireID().Wire;
          time = allHits[hits[ipl][ihit]]->PeakTime(); // What about here? T0
          stime = allHits[hits[ipl][ihit]]->PeakTimeMinusRMS();
          etime = allHits[hits[ipl][ihit]]->PeakTimePlusRMS();
          const size_t& hitIndex = allHits[hits[ipl][ihit]].key();

          double charge = allHits[hits[ipl][ihit]]->PeakAmplitude();
          if (fUseArea) charge = allHits[hits[ipl][ihit]]->Integral();
          //get 3d coordinate and track pitch for the current hit
          //not all hits are associated with space points, the method uses neighboring spacepts to interpolate
          double xyz3d[3];
          double pitch;
          bool fBadhit = false;
          if (fmthm.isValid()) {
            auto vhit = fmthm.at(trkIter);
            auto vmeta = fmthm.data(trkIter);
            for (size_t ii = 0; ii < vhit.size(); ++ii) {
              if (vhit[ii].key() == allHits[hits[ipl][ihit]].key()) {
                if (vmeta[ii]->Index() == std::numeric_limits<int>::max()) {
                  fBadhit = true;
                  continue;
                }
                if (vmeta[ii]->Index() >= tracklist[trkIter]->NumberTrajectoryPoints()) {
                  throw cet::exception("Calorimetry_module.cc")
                    << "Requested track trajectory index " << vmeta[ii]->Index()
                    << " exceeds the total number of trajectory points "
                    << tracklist[trkIter]->NumberTrajectoryPoints() << " for track index " << trkIter
                    << ". Something is wrong with the track reconstruction. Please contact "
                       "tjyang@fnal.gov";
                }
                if (!tracklist[trkIter]->HasValidPoint(vmeta[ii]->Index())) {
                  fBadhit = true;
                  continue;
                }

                //Correct location for SCE
                geo::Point_t const loc_tmp = tracklist[trkIter]->LocationAtPoint(vmeta[ii]->Index());
                double newX;
                if (loc_tmp.X()>0) {
                  newX = loc_tmp.X() + (drift_velocity * T0);
                }
                else {
                  newX = loc_tmp.X() - (drift_velocity * T0);
                }
                geo::Point_t const loc{newX, loc_tmp.Y(), loc_tmp.Z()};
                geo::Vector_t locOffsets = {
                  0.,
                  0.,
                  0.,
                };
                if (sce->EnableCalSpatialSCE() && fSCE)
                  locOffsets = sce->GetCalPosOffsets(loc, vhit[ii]->WireID().TPC);
                xyz3d[0] = loc.X() - locOffsets.X();
                xyz3d[1] = loc.Y() + locOffsets.Y();
                xyz3d[2] = loc.Z() + locOffsets.Z();

                double angleToVert = geom->WireAngleToVertical(vhit[ii]->View(),
                                                               vhit[ii]->WireID().TPC,
                                                               vhit[ii]->WireID().Cryostat) -
                                     0.5 * ::util::pi<>();
                const geo::Vector_t& dir = tracklist[trkIter]->DirectionAtPoint(vmeta[ii]->Index());
                double cosgamma =
                  std::abs(std::sin(angleToVert) * dir.Y() + std::cos(angleToVert) * dir.Z());
                if (cosgamma) { pitch = geom->WirePitch(vhit[ii]->View()) / cosgamma; }
                else {
                  pitch = 0;
                }

                //Correct pitch for SCE
                geo::Vector_t dirOffsets = {0., 0., 0.};
                if (sce->EnableCalSpatialSCE() && fSCE)
                  dirOffsets = sce->GetCalPosOffsets(geo::Point_t{loc.X() + pitch * dir.X(),
                                                                  loc.Y() + pitch * dir.Y(),
                                                                  loc.Z() + pitch * dir.Z()},
                                                     vhit[ii]->WireID().TPC);
                const TVector3& dir_corr = {pitch * dir.X() - dirOffsets.X() + locOffsets.X(),
                                            pitch * dir.Y() + dirOffsets.Y() - locOffsets.Y(),
                                            pitch * dir.Z() + dirOffsets.Z() - locOffsets.Z()};

                pitch = dir_corr.Mag();

                break;
              }
            }
          }
          else
            GetPitch(detprop,
                     allHits[hits[ipl][ihit]],
                     trkx,
                     trky,
                     trkz,
                     trkw,
                     trkx0,
                     xyz3d,
                     pitch,
                     TickT0);

          if (fBadhit) continue;
          //if (fNotOnTrackZcut && (xyz3d[2] < fNotOnTrackZcut.value())) continue; //hit not on track
          if (pitch <= 0) pitch = fTrkPitch;
          if (!pitch) continue;

          if (fnsps == 0) {
            xx = xyz3d[0];
            yy = xyz3d[1];
            zz = xyz3d[2];
            spdelta.push_back(0);
          }
          else {
            double dx = xyz3d[0] - xx;
            double dy = xyz3d[1] - yy;
            double dz = xyz3d[2] - zz;
            spdelta.push_back(sqrt(dx * dx + dy * dy + dz * dz));
            Trk_Length += spdelta.back();
            xx = xyz3d[0];
            yy = xyz3d[1];
            zz = xyz3d[2];
          }

          ChargeBeg.push_back(charge);
          ChargeEnd.push(charge);

          double MIPs = charge;
          double dQdx = MIPs / pitch;
          //calibHelper.LifeTimeCorrNew(dQdx, xyz3d[0], evt);
          // let's do the correction in the analyzer
          double dEdx = 0;
          // if (fUseArea)
          //   dEdx = caloAlg.dEdx_AREA(clock_data, detprop, *allHits[hits[ipl][ihit]], pitch, T0);
          // else
          //   dEdx = caloAlg.dEdx_AMP(clock_data, detprop, *allHits[hits[ipl][ihit]], pitch, T0);
          double dQdx_e = dQdx / 1e-3;
          double rho = detprop.Density();
          double Wion = 1000./util::kGeVToElectrons;
          double E_field_nominal = detprop.Efield();
          geo::Vector_t E_field_offsets = {0., 0., 0.};
          E_field_offsets = sce->GetCalEfieldOffsets(geo::Point_t{xyz3d[0], xyz3d[1], xyz3d[2]},planeID.TPC);
          TVector3 E_field_vector = {E_field_nominal*(1 + E_field_offsets.X()), E_field_nominal*E_field_offsets.Y(), E_field_nominal*E_field_offsets.Z()};
          double E_field = E_field_vector.Mag();
          double Beta = 0.212 / (rho * E_field);
          double Alpha = 0.93;
          dEdx = (exp(Beta * Wion * dQdx_e) - Alpha) / Beta;
          //std::cout << "dQdx: " << dQdx << std::endl;
          Kin_En = Kin_En + dEdx * pitch;

          if (allHits[hits[ipl][ihit]]->WireID().Wire < wire0)
            wire0 = allHits[hits[ipl][ihit]]->WireID().Wire;
          if (allHits[hits[ipl][ihit]]->WireID().Wire > wire1)
            wire1 = allHits[hits[ipl][ihit]]->WireID().Wire;

          fMIPs.push_back(MIPs);
          fdEdx.push_back(dEdx);
          fdQdx.push_back(dQdx);
          fwire.push_back(wire);
          ftime.push_back(time);
          fstime.push_back(stime);
          fetime.push_back(etime);
          fpitch.push_back(pitch);
          TVector3 v(xyz3d[0], xyz3d[1], xyz3d[2]);
          fXYZ.push_back(v);
          fHitIndex.push_back(hitIndex);
          ++fnsps;
        }
        if (fnsps < 2) {
          return to_be_returned;
        }
        for (int isp = 0; isp < fnsps; ++isp) {
          if (isp > 3) break;
          USChg += ChargeBeg[isp];
        }
        int countsp = 0;
        while (!ChargeEnd.empty()) {
          if (countsp > 3) break;
          DSChg += ChargeEnd.top();
          ChargeEnd.pop();
          ++countsp;
        }
        if (fFlipTrack_dQdx) {
          // Going DS if charge is higher at the end
          GoingDS = (DSChg > USChg);
        }
        else {
          // Use the track direction to determine the residual range
          if (!fXYZ.empty()) {
            TVector3 track_start(tracklist[trkIter]->Trajectory().Vertex().X(),
                                 tracklist[trkIter]->Trajectory().Vertex().Y(),
                                 tracklist[trkIter]->Trajectory().Vertex().Z());
            TVector3 track_end(tracklist[trkIter]->Trajectory().End().X(),
                               tracklist[trkIter]->Trajectory().End().Y(),
                               tracklist[trkIter]->Trajectory().End().Z());

            if ((fXYZ[0] - track_start).Mag() + (fXYZ.back() - track_end).Mag() <
                (fXYZ[0] - track_end).Mag() + (fXYZ.back() - track_start).Mag()) {
              GoingDS = true;
            }
            else {
              GoingDS = false;
            }
          }
        }

        // determine the starting residual range and fill the array
        fResRng.resize(fnsps);
        if (fResRng.size() < 2 || spdelta.size() < 2) {
          mf::LogWarning("Calorimetry")
            << "fResrng.size() = " << fResRng.size() << " spdelta.size() = " << spdelta.size();
        }
        if (GoingDS) {
          fResRng[fnsps - 1] = spdelta[fnsps - 1] / 2;
          for (int isp = fnsps - 2; isp > -1; isp--) {
            fResRng[isp] = fResRng[isp + 1] + spdelta[isp + 1];
          }
        }
        else {
          fResRng[0] = spdelta[1] / 2;
          for (int isp = 1; isp < fnsps; isp++) {
            fResRng[isp] = fResRng[isp - 1] + spdelta[isp];
          }
        }

        MF_LOG_DEBUG("CaloPrtHit") << " pt wire  time  ResRng    MIPs   pitch   dE/dx    Ai X Y Z\n";

        double Ai = -1;
        for (int i = 0; i < fnsps; ++i) { //loop over all 3D points
          vresRange.push_back(fResRng[i]);
          vdEdx.push_back(fdEdx[i]);
          vdQdx.push_back(fdQdx[i]);
          vXYZ.push_back(fXYZ[i]);
          XX.push_back((fXYZ[i]).X());
          YY.push_back((fXYZ[i]).Y());
          ZZ.push_back((fXYZ[i]).Z());
          if (i != 0 && i != fnsps - 1) { // ignore the first and last point
            // Calculate PIDA
            Ai = fdEdx[i] * pow(fResRng[i], 0.42);
            nPIDA++;
            PIDA += Ai;
          }

          MF_LOG_DEBUG("CaloPrtHit")
            << std::setw(4) << trkIter << std::setw(4) << ipl << std::setw(4) << i << std::setw(4)
            << fwire[i] << std::setw(6) << (int)ftime[i]
            << std::setiosflags(std::ios::fixed | std::ios::showpoint) << std::setprecision(2)
            << std::setw(8) << fResRng[i] << std::setprecision(1) << std::setw(8) << fMIPs[i]
            << std::setprecision(2) << std::setw(8) << fpitch[i] << std::setw(8) << fdEdx[i]
            << std::setw(8) << Ai << std::setw(8) << fXYZ[i].x() << std::setw(8) << fXYZ[i].y()
            << std::setw(8) << fXYZ[i].z() << "\n";
        } // end looping over 3D points
        if (nPIDA > 0) { PIDA = PIDA / (double)nPIDA; }
        else {
          PIDA = -1;
        }
        MF_LOG_DEBUG("CaloPrtTrk") << "Plane # " << ipl << "TrkPitch= " << std::setprecision(2)
                                   << fTrkPitch << " nhits= " << fnsps << "\n"
                                   << std::setiosflags(std::ios::fixed | std::ios::showpoint)
                                   << "Trk Length= " << std::setprecision(1) << Trk_Length << " cm,"
                                   << " KE calo= " << std::setprecision(1) << Kin_En << " MeV,"
                                   << " PIDA= " << PIDA << "\n";

        // look for dead wires
        for (unsigned int iw = wire0; iw < wire1 + 1; ++iw) {
          plane = allHits[hits[ipl][0]]->WireID().Plane;
          tpc = allHits[hits[ipl][0]]->WireID().TPC;
          cstat = allHits[hits[ipl][0]]->WireID().Cryostat;
          channel = geom->PlaneWireToChannel(plane, iw, tpc, cstat);
          if (channelStatus.IsBad(channel)) {
            MF_LOG_DEBUG("Calorimetry") << "Found dead wire at Plane = " << plane << " Wire =" << iw;
            unsigned int closestwire = 0;
            unsigned int endwire = 0;
            unsigned int dwire = 100000;
            double mindis = 100000;
            double goodresrange = 0;
            for (size_t ihit = 0; ihit < hits[ipl].size(); ++ihit) {
              channel = allHits[hits[ipl][ihit]]->Channel();
              if (channelStatus.IsBad(channel)) continue;
              // grab the space points associated with this hit
              std::vector<art::Ptr<recob::SpacePoint>> sppv = fmspts.at(hits[ipl][ihit]);
              if (sppv.size() < 1) continue;
              // only use the first space point in the collection, really each hit
              // should only map to 1 space point
              const recob::Track::Point_t xyz{
                sppv[0]->XYZ()[0], sppv[0]->XYZ()[1], sppv[0]->XYZ()[2]};
              double dis1 = (larEnd - xyz).Mag2();
              if (dis1) dis1 = std::sqrt(dis1);
              if (dis1 < mindis) {
                endwire = allHits[hits[ipl][ihit]]->WireID().Wire;
                mindis = dis1;
              }
              if (util::absDiff(wire, iw) < dwire) {
                closestwire = allHits[hits[ipl][ihit]]->WireID().Wire;
                dwire = util::absDiff(allHits[hits[ipl][ihit]]->WireID().Wire, iw);
                goodresrange = dis1;
              }
            }
            if (closestwire) {
              if (iw < endwire) {
                deadwire.push_back(goodresrange + (int(closestwire) - int(iw)) * fTrkPitch);
              }
              else {
                deadwire.push_back(goodresrange + (int(iw) - int(closestwire)) * fTrkPitch);
              }
            }
          }
        }
        to_be_returned.push_back(vdQdx);
        to_be_returned.push_back(vresRange);
        to_be_returned.push_back(fpitch);
        to_be_returned.push_back(XX);
        to_be_returned.push_back(YY);
        to_be_returned.push_back(ZZ);
        to_be_returned.push_back(vdEdx);

      } //end looping over planes
    }   //end looping over tracks

    return to_be_returned;
  }

  void FixCalo::GetPitch(detinfo::DetectorPropertiesData const& detprop,
                              art::Ptr<recob::Hit> const& hit,
                              std::vector<double> const& trkx,
                              std::vector<double> const& trky,
                              std::vector<double> const& trkz,
                              std::vector<double> const& trkw,
                              std::vector<double> const& trkx0,
                              double* xyz3d,
                              double& pitch,
                              double TickT0)
  {
    // Get 3d coordinates and track pitch for each hit
    // Find 5 nearest space points and determine xyz and curvature->track pitch

    // Get services
    art::ServiceHandle<geo::Geometry const> geom;
    auto const* sce = lar::providerFrom<spacecharge::SpaceChargeService>();

    //save distance to each spacepoint sorted by distance
    std::map<double, size_t> sptmap;
    //save the sign of distance
    std::map<size_t, int> sptsignmap;

    double wire_pitch = geom->WirePitch(0);

    double t0 = hit->PeakTime() - TickT0;
    double x0 =
      detprop.ConvertTicksToX(t0, hit->WireID().Plane, hit->WireID().TPC, hit->WireID().Cryostat);
    double w0 = hit->WireID().Wire;

    for (size_t i = 0; i < trkx.size(); ++i) {
      double distance = cet::sum_of_squares((trkw[i] - w0) * wire_pitch, trkx0[i] - x0);
      if (distance > 0) distance = sqrt(distance);
      sptmap.insert(std::pair<double, size_t>(distance, i));
      if (w0 - trkw[i] > 0)
        sptsignmap.emplace(i, 1);
      else
        sptsignmap.emplace(i, -1);
    }

    //x,y,z vs distance
    std::vector<double> vx;
    std::vector<double> vy;
    std::vector<double> vz;
    std::vector<double> vs;

    double kx = 0, ky = 0, kz = 0;

    int np = 0;
    for (auto isp = sptmap.begin(); isp != sptmap.end(); isp++) {
      double xyz[3];
      xyz[0] = trkx[isp->second];
      xyz[1] = trky[isp->second];
      xyz[2] = trkz[isp->second];

      double distancesign = sptsignmap[isp->second];
      if (np == 0 && isp->first > 30) { // hit not on track
        xyz3d[0] = std::numeric_limits<double>::lowest();
        xyz3d[1] = std::numeric_limits<double>::lowest();
        xyz3d[2] = std::numeric_limits<double>::lowest();
        pitch = -1;
        return;
      }
      if (np < 5) {
        vx.push_back(xyz[0]);
        vy.push_back(xyz[1]);
        vz.push_back(xyz[2]);
        vs.push_back(isp->first * distancesign);
      }
      else {
        break;
      }
      np++;
    }
    if (np >= 2) { // at least two points
      TGraph* xs = new TGraph(np, &vs[0], &vx[0]);
      try {
        if (np > 2) { xs->Fit("pol2", "Q"); }
        else {
          xs->Fit("pol1", "Q");
        }
        TF1* pol = 0;
        if (np > 2)
          pol = (TF1*)xs->GetFunction("pol2");
        else
          pol = (TF1*)xs->GetFunction("pol1");
        xyz3d[0] = pol->Eval(0);
        kx = pol->GetParameter(1);
      }
      catch (...) {
        mf::LogWarning("Calorimetry::GetPitch") << "Fitter failed";
        xyz3d[0] = vx[0];
      }
      delete xs;
      TGraph* ys = new TGraph(np, &vs[0], &vy[0]);
      try {
        if (np > 2) { ys->Fit("pol2", "Q"); }
        else {
          ys->Fit("pol1", "Q");
        }
        TF1* pol = 0;
        if (np > 2)
          pol = (TF1*)ys->GetFunction("pol2");
        else
          pol = (TF1*)ys->GetFunction("pol1");
        xyz3d[1] = pol->Eval(0);
        ky = pol->GetParameter(1);
      }
      catch (...) {
        mf::LogWarning("Calorimetry::GetPitch") << "Fitter failed";
        xyz3d[1] = vy[0];
      }
      delete ys;
      TGraph* zs = new TGraph(np, &vs[0], &vz[0]);
      try {
        if (np > 2) { zs->Fit("pol2", "Q"); }
        else {
          zs->Fit("pol1", "Q");
        }
        TF1* pol = 0;
        if (np > 2)
          pol = (TF1*)zs->GetFunction("pol2");
        else
          pol = (TF1*)zs->GetFunction("pol1");
        xyz3d[2] = pol->Eval(0);
        kz = pol->GetParameter(1);
      }
      catch (...) {
        mf::LogWarning("Calorimetry::GetPitch") << "Fitter failed";
        xyz3d[2] = vz[0];
      }
      delete zs;
    }
    else if (np) {
      xyz3d[0] = vx[0];
      xyz3d[1] = vy[0];
      xyz3d[2] = vz[0];
    }
    else {
      xyz3d[0] = std::numeric_limits<double>::lowest();
      xyz3d[1] = std::numeric_limits<double>::lowest();
      xyz3d[2] = std::numeric_limits<double>::lowest();
      pitch = -1;
      return;
    }
    pitch = -1;
    if (kx * kx + ky * ky + kz * kz) {
      double tot = sqrt(kx * kx + ky * ky + kz * kz);
      kx /= tot;
      ky /= tot;
      kz /= tot;
      //get pitch
      double wirePitch =
        geom->WirePitch(hit->WireID().Plane, hit->WireID().TPC, hit->WireID().Cryostat);
      double angleToVert = geom->Plane(hit->WireID().Plane, hit->WireID().TPC, hit->WireID().Cryostat)
                             .Wire(0)
                             .ThetaZ(false) -
                           0.5 * TMath::Pi();
      double cosgamma = TMath::Abs(TMath::Sin(angleToVert) * ky + TMath::Cos(angleToVert) * kz);
      if (cosgamma > 0) pitch = wirePitch / cosgamma;
      bool fSCE = true;
      //Correct for SCE
      geo::Vector_t posOffsets = {0., 0., 0.};
      geo::Vector_t dirOffsets = {0., 0., 0.};
      if (sce->EnableCalSpatialSCE() && fSCE)
        posOffsets =
          sce->GetCalPosOffsets(geo::Point_t{xyz3d[0], xyz3d[1], xyz3d[2]}, hit->WireID().TPC);
      if (sce->EnableCalSpatialSCE() && fSCE)
        dirOffsets = sce->GetCalPosOffsets(
          geo::Point_t{xyz3d[0] + pitch * kx, xyz3d[1] + pitch * ky, xyz3d[2] + pitch * kz},
          hit->WireID().TPC);

      xyz3d[0] = xyz3d[0] - posOffsets.X();
      xyz3d[1] = xyz3d[1] + posOffsets.Y();
      xyz3d[2] = xyz3d[2] + posOffsets.Z();

      // Correct pitch for SCE
      TVector3 dir = {pitch * kx - dirOffsets.X() + posOffsets.X(),
                      pitch * ky + dirOffsets.Y() - posOffsets.Y(),
                      pitch * kz + dirOffsets.Z() - posOffsets.Z()};
      pitch = dir.Mag();
    }
  }

}
#endif
