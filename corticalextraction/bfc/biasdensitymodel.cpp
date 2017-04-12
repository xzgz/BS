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

#include <cmath>
#include <BFC/biasdensitymodel.h>
#include <PVC/gaussian.h>

template<class T>
inline T computeSum(const T *a, const int n)
{
  T sum = (T)0;
  for (int i=0;i<n;i++) sum += a[i];
  return sum;
}

BiasDensityModel::BiasDensityModel() : epsilon(0.000000001f), biasEpsilon(1), verbosity(1)
{
  for (int i=0;i<NParameters;i++) Pv[i] = new float[256];
  for (int i=0;i<NParameters;i++) GPv[i] = new float[256];
}

BiasDensityModel::~BiasDensityModel()
{
  for (int i=0;i<NParameters;i++) delete[] Pv[i];
  for (int i=0;i<NParameters;i++) delete[] GPv[i];
}

void BiasDensityModel::makeHistogram(float h[], const Means3& m3, float beta, float pt[])
{
  computePv(Pv,m3.mean0,m3.mean1,m3.mean2,m3.std,beta);
  for (int v = 0; v<256; v++) h[v] = 0;
  for (int i=0;i<NClasses;i++)
  {
    float *pv = Pv[i];
    float p   = pt[i];
    for (int v = 0; v<256; v++)
      h[v] += p*pv[v];
  }
  float sum = computeSum(h,256);
  for (int v = 0; v<256; v++) h[v] /= sum;
}

void BiasDensityModel::pmixed(float P[], float ia, float ib, float sigma)
{
  float r = (float)(1.0f/((ia - ib) * sqrt(2*M_PI)));
  float s2 = 1.0f/sigma;
  for (int v=0;v<256;v++)
  {
    float Q0 = (ib-v)*s2;
    float Q1 = (ia-v)*s2;
    P[v]  = (float)(r * Gaussian::integral(Q0,Q1));
  }
}

float BiasDensityModel::gmixed(float v, float ia, float ib, float sigma, float beta)
{
  static const float rt2pi = (float)sqrt(2*M_PI);
  float Q0 = (beta*ib-v)/sigma;
  float Q1 = (beta*ia-v)/sigma;
  float BDI = beta * (ia - ib);
  float ERF = (float)(Gaussian::integral(Q0,Q1)/square(BDI));
  float den = rt2pi * sigma * BDI;
  float GG = (float)(ia * exp(-0.5 * square(Q1)) - ib * exp(-0.5 * square(Q0)));
  float g = GG/den - ERF;
  return g;
}

void BiasDensityModel::ppure(float p[], float m, float s)
{
  static const float d = (float)(1.0f/sqrt(2*M_PI));
  float c = d/s;
  for (int v = 0; v<256; v++)
    p[v] = (float)(c * exp(-0.5*square((v-m)/s)));
}

void BiasDensityModel::computePv(float *P[NClasses],
                                 float ic, float ig, float iw, float sigma, float beta)
{
  ppure (P[0], beta*ic,sigma);
  ppure (P[1], beta*ig,sigma);
  ppure (P[2], beta*iw,sigma);
  pmixed(P[3],ic,ig,sigma,beta);
  pmixed(P[4],ig,iw,sigma,beta);
  pmixed(P[5],ic/2,ic,sigma,beta);
}

void BiasDensityModel::computeGPv(float *Gv[NClasses],float *Pv_[NClasses],
                                  float ic,float ig,float iw, float sigma,float beta)
// Computes the gradient of [Pv] w.r.t. beta
// Pv[t][v] is the p(v|beta,tissue==t)
{
  float s2 = square(sigma);
  for (int v = 0; v<256; v++)
  {
    Gv[0][v] = Pv_[0][v] * (v-beta*ic)*ic/s2;
    Gv[1][v] = Pv_[1][v] * (v-beta*ig)*ig/s2;
    Gv[2][v] = Pv_[2][v] * (v-beta*iw)*iw/s2;
    Gv[3][v] = Pv_[3][v] * gmixed((float)v,ic,ig,sigma,beta);
    Gv[4][v] = Pv_[4][v] * gmixed((float)v,ig,iw,sigma,beta);
    Gv[5][v] = Pv_[5][v] * gmixed((float)v, 0,ic,sigma,beta);
  }
}

void BiasDensityModel::gradient(float gPa[], float h[], float ic, float ig, float iw,
                                float sigma, float thetaK[], float *w)
{
  float dPdB[256]; // dPdB = [gvC' gvG' gvW' gvCG' gvGW'] * pt';
  computePv(Pv,ic,ig,iw,sigma,thetaK[IBias]);
  for (int v = 0; v<256; v++) Pa[v] = 0;
  for (int t=0;t<NClasses;t++)
    for (int v = 0; v<256; v++)
      Pa[v] += thetaK[t]*Pv[t][v];
  float sum = computeSum(Pa,256);
  for (int v = 0; v<256; v++) Pa[v] /= sum;
  computeGPv(GPv,Pv,ic,ig,iw,sigma,thetaK[IBias]);
  for (int v=0;v<256;v++) dPdB[v] = 0;
  for (int t=0;t<NClasses;t++)
    for (int v=0;v<256;v++)
      dPdB[v] += GPv[t][v] * thetaK[t];
  for (int i=0;i<NParameters;i++) gPa[i] = 0;

  for (int v = 0; v<256; v++) R[v] = -2 * (h[v] - Pa[v]);
  if (w)
  {
    for (int t=0;t<NClasses;t++)
      for (int v=0;v<256;v++)
        gPa[t] += w[v] * R[v] * Pv[t][v];
    for (int v=0;v<256;v++)
      gPa[IBias] += w[v] * R[v] * dPdB[v];
  }
  else
  {
    for (int t=0;t<NClasses;t++)
      for (int v=0;v<256;v++)
        gPa[t] += R[v] * Pv[t][v];
    for (int v=0;v<256;v++)
      gPa[IBias] += R[v] * dPdB[v];
  }
}
