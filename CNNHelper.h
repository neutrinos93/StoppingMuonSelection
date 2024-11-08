/***
  Class containing useful functions for Michel electron selection.

*/
#ifndef CNN_HELPER_H
#define CNN_HELPER_H

#include "lardataobj/RecoBase/Hit.h"
#include "art/Framework/Core/EDAnalyzer.h"
#include "lardata/ArtDataHelper/MVAReader.h"
#include "TVector3.h"
#include "TMath.h"
#include "TProfile2D.h"
#include "TGraph2D.h"

#include "DataTypes.h"
#include "GeometryHelper.h"

namespace stoppingcosmicmuonselection {

  class CNNHelper {

  public:
    CNNHelper();
    ~CNNHelper();

    // Get the score for a given hit.
    float GetHitMichelScore(const anab::MVAReader<recob::Hit,4> &hitResults, const art::Ptr<recob::Hit> &hit);

    // Get the number of michel hits according to a threshold.
    size_t GetNumbMichelHits(const anab::MVAReader<recob::Hit,4> &hitResults, const artPtrHitVec &hits, float threshold);

    // Get the vector of scores.
    std::vector<double> GetScoreVector(const anab::MVAReader<recob::Hit,4> &hitResults, const artPtrHitVec &hits);

    // Remove hits with score above a threshold and return the remaining in a vector.
    const artPtrHitVec RemoveMichelHits(const anab::MVAReader<recob::Hit,4> &hitResults, const artPtrHitVec &hits, const double &thr);

    // Fill the 2D graph of hits in the plane according to the score from the CNN.
    void FillHitScoreGraph2D(TGraph2D *graph,
                             const anab::MVAReader<recob::Hit,4> &hitResults,
                             const artPtrHitVec &hits);

    // Fill the 2D image of hits in the plane according to the score from the CNN.
    void FillHitScoreImage(TProfile2D *image,
                           const anab::MVAReader<recob::Hit,4> &hitResults,
                           const artPtrHitVec &hits);

    // Fill 1D histogram with the score for a given vector.
    void FillScoreDistribution(TH1D *h, const anab::MVAReader<recob::Hit,4> &hitResults, const artPtrHitVec &hits);

  private:
    GeometryHelper geoHelper;

  };
}

#endif
