// Copyright (C) 2016 The Regents of the University of California and
// the University of Southern California
//
// Created by David W. Shattuck, Ph.D.
//
// This file is part of PVC.
//
// PVC is free software; you can redistribute it and/or
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

#include <numeric>
#include <cmath>
#include <PVC/pvmodel.h>
#include <PVC/gaussian.h>
#include <algorithm>

inline float PartialVolumeModel::average(float a, float b) { return (a+b)/2; }
inline float PartialVolumeModel::square(const float f) { return f*f; }
inline float PartialVolumeModel::square(const int f) { return (float)(f*f); }
const float PartialVolumeModel::SR2 = std::sqrt(2.0f);
const float PartialVolumeModel::SPI = std::sqrt((float)M_PI);
const float PartialVolumeModel::SQRT2PI = sqrtf(2.0f * (float)(M_PI));
const int PartialVolumeModel::backgroundLabel = 0;

float PartialVolumeModel::computeVariance(int *p, int n)
{
  int s = 0;
  for (int i=0;i<n;i++) s += p[i];
  float v = 0;
  for (int i=0;i<n;i++) v += square(i) * p[i];
  if (s<=0) return -1;
  v /= s;
  return v;
}

namespace Potentials {
enum Potentials { Va=2,Vb=1,Vc=1,Vd=-1 };
}

using namespace Potentials;
int PartialVolumeModel::cliquePotential1stOrder[9][9] =
{
  //bg  c  g  w  cg gw co
  {  0, Va, Vd, Vd, Vb, Vd, Vb,  0, 0 },//csf
  {  0, Vd, Va, Vd, Vb, Vb, Vd,  0, 0  },//gm
  {  0, Vd, Vd, Va, Vd, Vc, Vd,  0, 0  },//wm
  {  0, Vb, Vb, Vd, Va, Vc, Vc,  0, 0  },//cg
  {  0, Vd, Vb, Vb, Vc, Va, Vd,  0, 0  },//gw
  {  0, Vb, Vd, Vd, Vc, Vd, Va,  0, 0  },//co
  {  0,  0,  0,  0,  0,  0,  0, Va, 0  },
  {  0,  0,  0,  0,  0,  0,  0,  0, Va },
  { Va,  0,  0,  0,  0,  0,  0,  0, 0  },
};

float PartialVolumeModel::cliquePotential2ndOrder[9][9];

inline float PartialVolumeModel::merf(const float f)
{
  return 2.0f*(float)Gaussian::erf(SR2*f);	//	return 2*erf(f); TODO: replace with C++11 std::erf
}

void PartialVolumeModel::generatePmeas(const float theta[])
{
  pPure (pt[CSF],theta[MeanCSF],theta[Stdev]);
  pPure (pt[GM], theta[MeanGM ],theta[Stdev]);
  pPure (pt[WM], theta[MeanWM ],theta[Stdev]);
  pMixed(pt[CG], theta[MeanCSF],theta[MeanGM],theta[Stdev]);
  pMixed(pt[GW], theta[MeanGM ],theta[MeanWM],theta[Stdev]);
  pMixed(pt[CO], theta[MeanCSF],0,            theta[Stdev]);
}

void PartialVolumeModel::pPure(std::vector<float> &pdf, float m, float s)
{
  pdf.resize(256);
  const size_t n = pdf.size();
  const float K = 1.0f/(SQRT2PI*s);
  for (size_t i=0;i<n;i++) pdf[i] = K * exp(-0.5f*square((m-i)/s));
}

void PartialVolumeModel::pMixed(std::vector<float> &pdf, float u, float v, float sigma)
// TODO: could optimize sinv/SR2 by combining it to remove a division.
{
  pdf.resize(256);
  float r = 1.0f/(u - v);
  float sinv = 1.0f/(sigma);
  for (int x=0;x<256;x++)
  {
    pdf[x]  = -0.5f * r * (merf((x-u)*sinv/SR2) - merf((x-v)*sinv/SR2));
  }
}

void PartialVolumeModel::initTheta(const std::vector<float> &histogram, float theta[])
// TODO: bounds for ranges should be variables
{
  const size_t n = histogram.size();
  float accum = 0;
  size_t iCSF = 0;
  size_t iGM= 0;
  size_t iWM= 0;
  for (size_t i=0;i<n;i++) {	accum += histogram[i]; if (accum>=0.01) { iCSF = i;  break; } }
  for (size_t i=iCSF+1;i<n;i++) { accum += histogram[i]; if (accum>=0.40) { iGM  = i; break; }}
  for (size_t i=iGM+1;i<n;i++) { accum += histogram[i]; if (accum>=0.90) {iWM  = i; break; }}
  if (verbosity>1) std::cout<<"Initial means: mCSF="<<iCSF<<"\tmGM="<<iGM<<"\tmWM="<<iWM<<std::endl;
  theta[PartialVolumeModel::MeanCSF] = 0;//iCSF;
  theta[PartialVolumeModel::MeanGM] = (float)(iGM);
  theta[PartialVolumeModel::MeanWM] = (float)(iWM);
}

void PartialVolumeModel::cluster3(std::vector<int> &histogram, float theta[])	// three class cluster model.
{
  if (verbosity>1)
    std::cout<<"performing 3-class clustering"<<std::endl;
  {
    std::vector<float> fhist;
    fhist.resize(256);
    const float s = (float) std::accumulate(histogram.begin(),histogram.end(),0);
    for (int q=0;q<256;q++)
      fhist[q] = histogram[q]/s;
    initTheta(fhist,theta);
  }

  if (verbosity>1) { std::cout<<"means(init):\t"<<theta[0]<<'\t'<<theta[1]<<'\t'<<theta[2]<<std::endl; }
  std::vector<int> label(256);
  for (int i=0;i<50;i++)
  {
    float t1 = (theta[0] + theta[1])/2.0f;
    float t2 = (theta[1] + theta[2])/2.0f;
    float c1 = 0;
    float c2 = 0;
    float c3 = 0;
    float s1 = 0;
    float s2 = 0;
    float s3 = 0;
    for (int j=0;j<256;j++)
    {
      if (j<t1)
      {
        c1 += j * histogram[j];
        s1 += histogram[j];
      }
      else
      {
        if (j<t2)
        {
          c2 += j * histogram[j];
          s2 += histogram[j];
        }
        else
        {
          c3 += j * histogram[j];
          s3 += histogram[j];
        }
      }
    }
    if (s1 > 0) c1/=s1; else (c1 = theta[0]);
    if (s2 > 0) c2/=s2; else (c2 = theta[1]);
    if (s3 > 0) c3/=s3; else (c3 = theta[2]);
    float d2 = square(theta[0]-c1)+square(theta[1]-c2)+square(theta[2]-c3);
    theta[0] = c1;
    theta[1] = c2;
    theta[2] = c3;
    if (verbosity>1) std::cout<<"means("<<i<<"):  \t"<<theta[0]<<'\t'<<theta[1]<<'\t'<<theta[2]<<'\t'<<"(stopping criteria: "<<d2<<")"<<std::endl;
    if (d2<0.001) break;
  }
  int t1 = (int)((theta[0] + theta[1])/2.0f);
  int t2 = (int)((theta[1] + theta[2])/2.0f);
  int p2 = t1+findPeak(&histogram[t1],t2-t1);
  int p3 = t2+findPeak(&histogram[t2],256-t2);
  if (1) { theta[1] = (float)(p2); theta[2] = (float)(p3); }
  float variance = computeVariance(&histogram[p3],256-p3) ;
  if (variance>0) theta[3] = sqrt(variance);
  if (verbosity>2) std::cout<<"means(peaks):\t"<<theta[0]<<'\t'<<theta[1]<<'\t'<<theta[2]<<std::endl;
  if (verbosity>2) std::cout<<"Std = "<<theta[3]<<'\n'<<std::endl;
}

void PartialVolumeModel::initICM(float theta[])
{
  float r2 = sqrt(2.0f);
  {
    for (int a=0;a<9;a++)
      for (int b=0;b<9;b++)
        cliquePotential2ndOrder[a][b] = r2 * cliquePotential1stOrder[a][b];
  }
  logLikelihood.resize(6);
  generatePmeas(theta);
  for (int a=0;a<6;a++)
  {
    logLikelihood[a].resize(256);
    for (int b=0;b<256;b++)
    {
      logLikelihood[a][b] = std::log(pt[a][b]+0.000001f);
    }
  }
}

int PartialVolumeModel::ICM(Vol3D<uint8> &vImage, Vol3D<uint8> &vLabel, float theta[], float lambda, const int maxIterations)
{
  if (verbosity>1) { std::cout<<"initializing ICM"<<std::endl; }
  initICM(theta);
  const int cxs = vImage.cx - 1;
  const int cys = vImage.cy - 1;
  const int czs = vImage.cz - 1;
  const int zStride = vImage.cx * vImage.cy;
  const int yStride = vImage.cx;
  const int xStride = 1;
  uint8 *image = vImage.start();
  uint8 *label = vLabel.start();
  int lastChange = 0;

  for (int q=0;q<maxIterations;q++)
  {
    int change = 0;
    for (int z=1;z<czs;z++)
    {
      for (int y=1;y<cys;y++)
      {
        int idx = z*zStride + y*yStride + 1;
        for (int x=1;x<cxs;x++,idx++)
        {
          if (image[idx]==0) { label[idx]=backgroundLabel; continue; }
          float E[6] = {0,0,0,0,0,0};
          for (int k=0;k<6;k++) // number of classes
          {
            float L = logLikelihood[k][image[idx]];
            float score=0.0f;
            score += cliquePotential1stOrder[k][label[idx-xStride]];
            score += cliquePotential1stOrder[k][label[idx+xStride]];
            score += cliquePotential1stOrder[k][label[idx-yStride]];
            score += cliquePotential1stOrder[k][label[idx+yStride]];
            score += cliquePotential1stOrder[k][label[idx-zStride]];
            score += cliquePotential1stOrder[k][label[idx+zStride]];
            score += cliquePotential2ndOrder[k][label[idx-yStride-xStride]];
            score += cliquePotential2ndOrder[k][label[idx-yStride+xStride]];
            score += cliquePotential2ndOrder[k][label[idx+yStride-xStride]];
            score += cliquePotential2ndOrder[k][label[idx+yStride+xStride]];
            score += cliquePotential2ndOrder[k][label[idx-zStride-xStride]];
            score += cliquePotential2ndOrder[k][label[idx-zStride+xStride]];
            score += cliquePotential2ndOrder[k][label[idx+zStride-xStride]];
            score += cliquePotential2ndOrder[k][label[idx+zStride+xStride]];
            score += cliquePotential2ndOrder[k][label[idx-zStride-yStride]];
            score += cliquePotential2ndOrder[k][label[idx-zStride+yStride]];
            score += cliquePotential2ndOrder[k][label[idx+zStride-yStride]];
            score += cliquePotential2ndOrder[k][label[idx+zStride+yStride]];
            E[k] = L+lambda*score;
          }
          uint8 choice = 0;
          for (int k=1;k<6;k++)
          {
            if (E[k]>E[choice]) { choice = k; }
          }
          if (choice+1!=label[idx])
          {
            label[idx] = choice+1;
            change++;
          }
        }
      }
    }
    if (verbosity>1) { std::cout<<"changed "<<change<<" labels"<<std::endl; }
    lastChange = change;
    if (change==0) break;
  }
  if (verbosity>1)
  {
    std::cout<<"finished ICM"<<std::endl;
  }
  return lastChange;
}

void PartialVolumeModel::crisp(Vol3D<uint8> &vImage, Vol3D<uint8> &vLabel, float theta[])
{
  float t1 = average(theta[0],theta[1]);
  float t2 = average(theta[1],theta[2]);
  const int ds = vImage.size();
  uint8 *image = vImage.start();
  uint8 *label = vLabel.start();
  for (int i=0;i<ds;i++)
  {
    switch (label[i])
    {
      case 3: label[i] = (image[i]<t1) ? 0 : 1; break;
      case 4: label[i] = (image[i]<t2) ? 1 : 2; break;
      case 5: label[i] = 0; break;
    }
  }
}

void PartialVolumeModel::classify6(Vol3D<uint8> &vImage, Vol3D<uint8> &vLabel, float theta[])
{
  if (verbosity>1) std::cout<<"performing initial 6-class classification"<<std::endl;
  uint8 map[256];
  generatePmeas(theta);
  for (int i=0;i<256;i++)
  {
    int m = 0;
    float v = pt[0][i];
    for (int j=1;j<6;j++)
    {
      if (pt[j][i]>v) { v = pt[j][i]; m = j; }
    }
    map[i] = m+1;
  }
  map[0] = backgroundLabel;
  vLabel.makeCompatible(vImage);
  const int n = vImage.size();
  uint8 *l = vLabel.start();
  const uint8 *d = vImage.start();
  for (int i=0;i<n;i++) l[i] = map[d[i]];
}

int PartialVolumeModel::findPeak(const int *d, int n)
{
  int r = 0;
  int mx = d[r];
  for (int i=1;i<n;i++) { if (d[i]>mx) { mx = d[i]; r = i; }  }
  return r;
}

int PartialVolumeModel::findPeak(const int *d, int m, int n)
{
  int r = m;
  int mx = d[r];
  for (int i=m+1;i<n;i++) { if (d[i]>mx) { mx = d[i]; r = i; }  }
  return r;
}

void PartialVolumeModel::cluster6(std::vector<int> &histogram, float theta[])	// three class cluster model.
{
  if (verbosity>1) std::cout<<"estimating class means"<<std::endl; //performing 6-class clustering"<<std::endl;
  float mCG = average(theta[0],theta[1]);	// mean of CG
  float mGW = average(theta[1],theta[2]);	// mean of GW
  int t2 = (int)(average(mCG,theta[MeanGM]));
  int t3 = (int)(average(theta[MeanGM],mGW));
  int t4 = (int)(average(mGW,theta[MeanWM]));
  int p2 = findPeak(&histogram[0],t2,t3);
  int p3 = findPeak(&histogram[0],t4,256);
  theta[1] = (float)(p2);
  theta[2] = (float)(p3);

  //TODO: move to separate function?
  {
    int p3 = (int)(theta[2]);
    const float variance = (computeVariance(&histogram[p3],256-p3));
    if (variance>0)
      theta[3] = sqrt(variance);
    {
      float mCG = average(theta[0],theta[1]);	// mean of CG
      float mGW = average(theta[1],theta[2]);	// mean of GW
      float T[6];
      T[0] = theta[0]/2;
      T[1] = average(theta[MeanCSF],mCG);
      T[2] = average(mCG,theta[MeanGM]);
      T[3] = average(theta[MeanGM],mGW);
      T[4] = average(mGW,theta[MeanWM]);
      T[5] = 256;
      float C[6];
      float total=0;
      for (int i=0,p=0;i<6;i++)
      {
        float s = 0;
        for (;p<T[i];p++)
        {
          s += histogram[p];
        }
        C[i] = s;
        total += s;
      }
      if (total>0)
      {
        theta[PCO]  = C[0]/total;
        theta[PCSF] = C[1]/total;
        theta[PCG]  = C[2]/total;
        theta[PGM]  = C[3]/total;
        theta[PGW]  = C[4]/total;
        theta[PWM]  = C[5]/total;
      }
      else
      {
        if (verbosity>0) std::cout<<"warning : no non-zero voxels detected."<<std::endl;
        theta[PCO]  = C[0];
        theta[PCSF] = C[1];
        theta[PCG]  = C[2];
        theta[PGM]  = C[3];
        theta[PGW]  = C[4];
        theta[PWM]  = C[5];
      }
      int intensityOrder[] = { 5, 0, 3, 1, 4, 2 };  // maps them in order
      const std::string tissueNames[] = { "CSF", "GM", "WM", "CSF/GM", "GM/WM", "CSF/Other" };
      if (verbosity>0)
      {
        for (int i=0;i<6;i++)
        {
          if (C[i]<=0)
            std::cout<<"warning: no "<<tissueNames[intensityOrder[i]]<<" voxels detected during intialization."<<std::endl;
        }
      }
    }
  }
}

