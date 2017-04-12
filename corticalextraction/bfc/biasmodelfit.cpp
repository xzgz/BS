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

#include <BFC/biasdensitymodel.h>
#include <BFC/bfcmessages.h>
#include <math.h>
#include <fstream>
#include <iomanip>

template<class T>
inline T computeSum(const T *a, const int n)
{
  T sum = (T)0;
  for (int i=0;i<n;i++) sum += a[i];
  return sum;
}

inline float BiasDensityModel::werror(float *a, float *b, float *w, int n)
{
  float sum = 0;
  for (int i=0;i<n;i++) sum += w[i] * square(a[i] - b[i]);
  return sum;
}

void BiasDensityModel::fit(float H[256], const Means3& m, float *theta0, Weighting::Types weighting)
{
  float thetaK[NParameters];
  for (int i=0;i<NParameters;i++) thetaK[i] = theta0[i];
  float Px[256];
  float Pna[NParameters];
  float direx[NParameters];
  float grad[NParameters];
  const float aRate   = 0.5f;
  const float minMult = 0.1f;
  const int maxQ=1000;
  int stop=maxQ;
  float w[256];
  int hmax = 255;
  for (int i=0;i<256;i++) if (H[i]>0) hmax = i;
  for (int i=0;i<256;i++) w[i] = 1.0f;

  if ((weighting==Weighting::Ramp)||(weighting==Weighting::Both))
  {
    for (int i=0;i<=hmax;i++) w[i] = (float)(i);
  }
  if ((weighting==Weighting::Histogram)||(weighting==Weighting::Both))
  {
    for (int i=0;i<=hmax;i++) w[i] *= H[i];
    for (int i=hmax+1;i<256;i++) w[i] = 0;
  }
  makeHistogram(Px,m,thetaK[IBias],thetaK);
  double lasterror = werror(H,Px,w);
  float alpha0 = 1.0;
  for (int q=0;q<maxQ;q++)
  {
    gradient(grad,H,m.mean0,m.mean1,m.mean2,m.std,thetaK,w);
    for (int i=0;i<NParameters;i++) direx[i] = -grad[i];
    float gTd = 0.0f;
    for (int i=0;i<NParameters;i++) gTd += direx[i] * grad[i];
    float errorK=100;
    float alphaM = alpha0;
    float alpha  = -1.0;
    for (int r=0;r<30;r++)
    {
      for (int d=0;d<NClasses;d++)
      {
        Pna[d] = thetaK[d] + alphaM * direx[d];
        if (Pna[d]<0) Pna[d] = 0;
      }
      float betaN = thetaK[IBias] + alphaM * direx[IBias];
      makeHistogram(Px,m,betaN,Pna);
      errorK = werror(H,Px,w);
      if ((lasterror - errorK) >= (-minMult * alphaM * gTd)) // this is correct. it is checking if the change in error is larger than some minimum value
      {
        alpha = alphaM;
        break;
      }
      alphaM *= aRate;
    }
    if (alpha<0) // terminate condition: couldn't find a stepsize to improve value.
    {
      stop = q;
      break;
    }
    if (fabs(lasterror-errorK)<epsilon)
    {
      stop = q;
      break;
    }
    lasterror = errorK;
    for (int i=0;i<NClasses;i++)
    {
      thetaK[i] += alpha * direx[i];
      if (thetaK[i]<0) thetaK[i] = 0;
    }
    thetaK[IBias] += alpha * direx[IBias];
  }
  float sum = computeSum(thetaK,NClasses); // add up the first 5 elements.
  for (int i=0;i<NClasses;i++)
    theta[i] = thetaK[i]/sum;
  theta[IBias] = thetaK[IBias];
  lastStop = stop;
  lastError = (float)lasterror;
}
