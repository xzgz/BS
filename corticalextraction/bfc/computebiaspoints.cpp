// Copyright (C) 2016 The Regents of the University of California and
// the University of Southern California
//
// Created by David W. Shattuck, Ph.D.
//
// This file is part of BFC.
//
// BFC is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; version 2.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
// USA.
//

//#include <PVC/tissue.h>
#include <BFC/biasfieldcorrector.h>
#include <BFC/biasdensitymodel.h>
#include <BFC/roihistogram.h>
#include <DS/timer.h>
#include <iomanip>
#include <histogramsmoother.h>
#include <BFC/bfcmessages.h>

template<class T>
inline T computeSum(const T *a, const int n)
{
  T sum = (T)0;
  for (int i=0;i<n;i++) sum += a[i];
  return sum;
}

void smoothHistogram(std::vector<float> &hSmooth, std::vector<float> &Hin)
{
  HistogramSmoother smoother;
  smoother.smooth(hSmooth,Hin);
}

namespace BFCStats {
Means3 histTest(Histogram &h);
}

bool BiasFieldCorrector::computeNextPoint()
{
  return false;
}

void BiasFieldCorrector::computeBiasPoints()
{
  if (vol.start() == 0)
  {
    BFCMessages::status("No data");
    return;
  }
  if (verbosity>1) { BFCMessages::status("Computing bias estimate samples"); }
  biasEstimateMask.makeCompatible(vol);
  biasEstimateMask.set(0);
  biasEstimateIndex.makeCompatible(vol);
  biasEstimateIndex.set(0);
  biasSampleField.makeCompatible(vol);
  biasSampleField.set(0);
  biasCentroids.makeCompatible(vol);
  biasCentroids.set(0);
  int nInfoPoints = 0;
  const int cx = vol.cx;
  const int cy = vol.cy;
  const int cz = vol.cz;

  const int dx=biasEstimateSpacing;
  const int dy=(int)(dx * vol.rx/vol.ry);
  const int dz=(int)(dx * vol.rx/vol.rz);

  const int maxBetaPoints = ((cz/dz)+1) * ((cx/dx)+1) * ((cy/dy)+1);
  biasPoints.resize(maxBetaPoints);
  biasInfo.resize(maxBetaPoints);
  nBiasPoints = 0;
  if (!maskdata) maskdata = vol.start();

  Histogram globalHistogram;
  globalHistogram.compute(vol.start(),maskdata,vol.size());

  Means3 m3 = BFCStats::histTest(globalHistogram);
  if (verbosity>1)
  {
    std::cout<<"\nTissue means:\tCSF = "<<m3.mean0<< "  GM = "<<m3.mean1 <<"  WM = "<<m3.mean2<<"\n";
    std::cout<<"Std Dev: "<<m3.std<<"\tVariance: "<<m3.var<<"\n";
  }
  if (scaleVariance)
  {
    float alpha = varScale;
    if (alpha<0) alpha = (float)(1 - 0.002 * square(m3.mean2)/m3.var2);
    if (alpha>0)
    {
      m3.scaleVariance(alpha);
      if (verbosity>0)
      {
        std::cout<<"Scaled variance by "<<alpha<<".\nStd Dev: "<<m3.std<<"\tVariance: "<<m3.var<<"\n";
      }
    }
  }

  // compute local parameters
  BiasDensityModel model;
  model.epsilon = epsilon;
  model.biasEpsilon = biasEpsilon;
  model.verbosity = verbosity;

  Timer t; t.start();
  ROIHistogram localHistogram;
  localHistogram.setup(vol,(float)histogramRadius, (roiType==Block) ? ROIHistogram::Box : ROIHistogram::Ellipse);
  int minimum = 1000;
  int roiVolume = (int)localHistogram.nVoxels();
  if (verbosity>1) std::cout<<"ROI volume is "<<roiVolume<<" voxels."<<std::endl;
  if (minimum>roiVolume)
  {
    minimum = roiVolume/2;
    std::cout<<"Warning: Block volume is extremely small ("<<roiVolume<<" voxels).\nConsider using a larger histogram width."<<std::endl;
  }
  else
    minimum = roiVolume/8; // TODO: fix arbitrary size minimum
  if (debug) std::cout<<"Minimum count is "<<minimum<<std::endl;
  int mz = 0;
  int my = 0;
  int mx = 0;
  if (center == true)
  {
    mz = ((cz-1)%dz)/2;
    my = ((cy-1)%dy)/2;
    mx = ((cx-1)%dx)/2;
  }
  float H[256];
  float P[256];
  float thetaI[7];

  int nx = 0;
  int ny = 0;
  int nz = 0;

  for (int z=mz;z<cz;z+=dz)
  {
    nz++; ny = 0;
    for (int y=my;y<cy;y+=dy)
    {
      float theta0[7] = { 0.05f,  0.20f, 0.30f, 0.2f, 0.2f, 0.05f, 1.0f };
      ny++; nx = 0;
      for (int x=mx;x<cx;x+=dx)
      {
        if (halt) { return; }
        nx++;
        localHistogram.compute(vol,vol,x,y,z);
        float beta = 0.0;
        BiasInfo binfo;
        binfo.pos = z*cx*cy + y*cx + x;
        binfo.x = x;
        binfo.y = y;
        binfo.z = z;
        binfo.centroid = localHistogram.centroid;
        binfo.nBrainVoxels = localHistogram.total;
        binfo.valid = false;
        if ((localHistogram.total>0))
        {
          const float sum = (float)localHistogram.total;
          for (int i=0;i<256;i++) H[i] = localHistogram.count[i]/sum;
        }
        else
        {
          for (int i=0;i<256;i++) H[i] = 0;
        }
        std::vector<float> hVec(H,H+256);
        std::vector<float> Hsmooth;
        if (0) // smoothing currently disabled
          ::smoothHistogram(Hsmooth,hVec);
        else
          Hsmooth = hVec;
        float *Hs = &Hsmooth[0];
        if ((localHistogram.total<minimum))
        {
          binfo.rejected = BiasInfo::tooFew;
          for (int i=0;i<256;i++) { P[i]=0; H[i] = 0; Hs[i] = 0; }
          beta = 0;
        }
        else
        {
          float sum = computeSum(theta0,model.NClasses);
          for (int i=0;i<model.NClasses;i++) theta0[i] /= sum;

          for (int i=0;i<model.NParameters;i++)
            thetaI[i] = theta0[i];
          if (smooth)
          {
            model.fit(Hs,m3,thetaI,Weighting::Flat);
          }
          else
          {
            model.fit(H ,m3,thetaI,Weighting::Flat);
          }
          for (int i=0;i<model.NParameters;i++)
            thetaI[i] = model.theta[i];
          model.makeHistogram(P,m3);
          beta = model.theta[model.IBias];
          binfo.fitnessError = model.lastError;
          biasSampleField[binfo.pos] = beta;
          biasCentroids((int)binfo.centroid.x,(int)binfo.centroid.y,(int)binfo.centroid.z) = 255;
          if ((beta<biasRange.first)||(beta>biasRange.second))
          {
            binfo.bias = 0;
            binfo.valid = false;
            binfo.rejected = BiasInfo::outOfRange;
            theta0[6] = 1;
          }
          else
          {
            binfo.bias = beta;
            binfo.valid = true;
          }
        }
        if (debug)
        {
          std::cout<<"point "<<nInfoPoints<<":\t"<<binfo.bias<<"\n";
        }
        biasInfo[nInfoPoints++] = binfo;
      }
    }
    if (verbosity==2) BFCMessages::dot();
  }
  if (verbosity>1) { BFCMessages::reset(); BFCMessages::describe("Pruning"); }
  prune(biasInfo,nx,ny,nz,neighborhoodThreshold);
  if (verbosity>1) { BFCMessages::reset(); BFCMessages::describe("Finished pruning"); }

  for (int i=0;i<nInfoPoints;i++)
  {
    BiasInfo &bi= biasInfo[i];
    if (bi.valid)
    {
      biasEstimateMask[bi.pos] = 255;
      biasEstimateIndex[bi.pos] = nBiasPoints;
      biasPoints[nBiasPoints++] = bi.bias;
    }
    biasEstimateIndex[bi.pos] = 8+bi.rejected;
  }
  if (verbosity>1) { BFCMessages::reset(); BFCMessages::status("Finished computing bias points."); }
  t.stop();
  if (timer && (verbosity>1)) std::cout<<"bias point computation took "<<t.elapsed()<<std::endl;
}

float computeThreshold(std::vector<BiasInfo> &bi, const int nx, const int ny, const int nz, float threshmult=1.0f)
// biasInfo is a 3D lattice of estimates.
{
  const int sz = nx * ny;
  const int sy = nx;
  int index = 0;
  for (int i=0;i<nz;i++)
  {
    for (int j=0;j<ny;j++)
    {
      for (int k=0;k<nx;k++, index++)
      {
        if (bi[index].valid==false) continue;
        float b = bi[index].bias;
        float e = 0;
        float m = 0;
        int count = 0;
        if (i>0   )
          if (bi[index-sz].valid) { count++; e += square(b - bi[index-sz].bias); m += bi[index-sz].bias; }
        if (i<nz-1)
          if (bi[index+sz].valid) { count++; e += square(b - bi[index+sz].bias); m += bi[index+sz].bias; }
        if (j>0   )
          if (bi[index-sy].valid) { count++; e += square(b - bi[index-sy].bias); m += bi[index-sy].bias; }
        if (j<ny-1)
          if (bi[index+sy].valid) { count++; e += square(b - bi[index+sy].bias); m += bi[index+sy].bias; }
        if (k>0   )
          if (bi[index- 1].valid) { count++; e += square(b - bi[index- 1].bias); m += bi[index- 1].bias; }
        if (k<nx-1)
          if (bi[index+ 1].valid) { count++; e += square(b - bi[index+ 1].bias); m += bi[index+ 1].bias; }
        bi[index].neighborhood = (count>0) ? (e/count) : 0;
        bi[index].nbrs = count;
        bi[index].mean = m/count;
      }
    }
  }
  float m = 0;
  float s = 0;
  {
    float v = 0;
    float c = 0;
    const int np = nx*ny*nz;
    for (int i=0;i<np;i++)
    {
      BiasInfo &binfo = bi[i];
      if (binfo.valid==false) continue;
      if (binfo.neighborhood>0)
      {
        float f = (float)log(binfo.neighborhood);
        m += f;
        v += f*f;
        c++;
      }
    }
    m /= c;
    v /= c;
    if ((v - m*m)>0) s = (float)sqrt(v - m*m);
  }
  return (float)exp(m + threshmult * s);
}

void BiasFieldCorrector::prune(std::vector<BiasInfo> &bi, const int nx, const int ny, const int nz, float threshmult)
{
  {
    const int np = nx*ny*nz;
    float m = 0;
    float s = 0;
    {
      float v = 0;
      int c = 0;
      for (int i=0;i<np;i++)
      {
        BiasInfo &binfo = bi[i];
        if (binfo.valid==false) continue;
        if (binfo.fitnessError>0)
        {
          float f = (float)log(binfo.fitnessError);
          m += f;
          v += f*f;
          c++;
        }
      }
      m /= c;
      v /= c;
      s = (float)sqrt(v - m*m);
    }
    float thresh = (float)exp(m + threshmult * s);
    int rejcount = 0;
    int okaycount = 0;
    for (int i=0;i<np;i++)
    {
      BiasInfo &binfo = bi[i];
      if (binfo.valid==false) continue;
      if (binfo.fitnessError>thresh)
      {
        binfo.valid = false;
        binfo.rejected = binfo.bigError;
        rejcount++;
      }
      okaycount++;
    }
    if (debug)
    {
      std::cout<<"Rejected "<<rejcount<<"/"<<okaycount<<std::endl;
    }
  }

  const int N = 10;
  float thr = computeThreshold(bi,nx,ny,nz,threshmult);
  for (int i=0;i<N;i++)
  {
    localSmoothness(bi,nx,ny,nz,thr);
  }
}

int BiasFieldCorrector::localSmoothness(std::vector<BiasInfo> &bi, const int nx, const int ny, const int nz, float thresh, float )
// biasInfo is a 3D lattice of estimates.
{
  const int sz = nx * ny;
  const int sy = nx;
  const int np = nx*ny*nz;
  int index = 0;
  int rejcount = 0;
  int okaycount = 0;
  for (int i=0;i<nz;i++)
  {
    for (int j=0;j<ny;j++)
    {
      for (int k=0;k<nx;k++, index++)
      {
        if (bi[index].valid==false) continue;
        float b = bi[index].bias;
        float e = 0;
        float m = 0;
        int count = 0;
        // this is not super-efficient, but it is a small lattice (~4K pts), so there
        // is not a huge computational penalty.
        if (i>0   )
          if (bi[index-sz].valid) { count++; e += square(b - bi[index-sz].bias); m += bi[index-sz].bias; }
        if (i<nz-1)
          if (bi[index+sz].valid) { count++; e += square(b - bi[index+sz].bias); m += bi[index+sz].bias; }
        if (j>0   )
          if (bi[index-sy].valid) { count++; e += square(b - bi[index-sy].bias); m += bi[index-sy].bias; }
        if (j<ny-1)
          if (bi[index+sy].valid) { count++; e += square(b - bi[index+sy].bias); m += bi[index+sy].bias; }
        if (k>0   )
          if (bi[index- 1].valid) { count++; e += square(b - bi[index- 1].bias); m += bi[index- 1].bias; }
        if (k<nx-1)
          if (bi[index+ 1].valid) { count++; e += square(b - bi[index+ 1].bias); m += bi[index+ 1].bias; }
        bi[index].neighborhood = (count>0) ? (e/count) : 0;
        bi[index].nbrs = count;
        bi[index].mean = m/count;
      }
    }
  }
  if (replaceWithMean)
  {
    for (index=0;index<np;index++)
    {
      if (bi[index].valid==false) continue;
      if (bi[index].neighborhood>thresh)
      {
        bi[index].rejected = BiasInfo::neighbors;
        bi[index].bias = bi[index].mean;
        rejcount++;
      }
      okaycount++;
    }
  }
  else
  {
    for (index=0;index<np;index++)
    {
      if (bi[index].valid==false) continue;
      if (bi[index].neighborhood>thresh)
      {
        bi[index].rejected = BiasInfo::neighbors;
        bi[index].valid = false;
        rejcount++;
      }
      okaycount++;
    }
  }
  return rejcount;
}
