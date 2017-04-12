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

#include <BFC/regularizedsplinef.h>
#include <BFC/bfcmessages.h>
#include <stdio.h>
#include <stdlib.h>
#include <DS/timer.h>
#include <fstream>

RegularizedSplineF::RegularizedSplineF() :
  splineLambda(0.0001f),	verbosity(1), stopError(0.001f), stopDelta(0.0001f), stopCount(500),
  imagecount(0), imagesize(0),
  cx(0), cy(0), cz(0),
  dx(0), dy(0), dz(0),
  nx(0), ny(0), nz(0),
  cpsize(0), czstride(0),
  CP(0), measured(0),
  splineImg(0), mask(0),
  direx(0)
{
  for (int i=0;i<4;i++)
  {
    bx[i] = 0;
    by[i] = 0;
    bz[i] = 0;
  }
}

// integrals of the spline energy terms; derived externally.
// each is from a convolution of polynomials, integrated over the range [0,1], with maximum order of 7,
// (u^3 ** u^3, integrated), hence the numerators of up to 7!.
const float RegularizedSplineF::Ib2_[7] = {	1.0f/5040.0f,	 1.0f/42.0f,	 397.0f/1680.0f,	151.0f/315.0f,	397.0f/1680.0f,	 1.0f/42.0f, 1.0f/5040.0f };  
const float RegularizedSplineF::Idb2_[7] = {-1.0f/120.0f,	-1.0f/5.0f,		-1.0f/8.0f,				2.0f/3.0f,			-1.0f/8,				-1.0f/5.0f,	-1.0f/120.0f   };
const float RegularizedSplineF::Id2b2_[7] = {     1.0f/6.0f,       0.0f,  -3.0f/2.0f,				8.0f/3.0f,		  -3.0f/2.0f,           0,           1.0f/6.0f     };
const float *RegularizedSplineF::Ib2= &RegularizedSplineF::Ib2_[3];
const float *RegularizedSplineF::Idb2= &RegularizedSplineF::Idb2_[3];
const float *RegularizedSplineF::Id2b2= &RegularizedSplineF::Id2b2_[3];

void RegularizedSplineF::initializeSplineEnergy()
{
  for (int z=0;z<4;z++)
    for (int y=0;y<4;y++)
      for (int x=0;x<4;x++)
      {
        E[z][y][x] = 0;
        for (int q=0;q<3;q++)
          for (int r=0;q<3;q++)
          {
            if (q==r)
            {
              switch (q)
              {
                case 0 : E[z][y][x] += Ib2[z] * Ib2[y] * Id2b2[x] ; break;
                case 1 : E[z][y][x] += Ib2[x] * Ib2[z] * Id2b2[y] ; break;
                case 2 : E[z][y][x] += Ib2[x] * Ib2[y] * Id2b2[z] ; break;
              }
            }
            else
            {
              int s = 0;
              if (r==0||q==0) s = 1;
              if (r==1||q==1) s = 2;
              float f = 0;
              switch (q)
              {
                case 0 : f = Idb2[x] ; break;
                case 1 : f = Idb2[y] ; break;
                case 2 : f = Idb2[z] ; break;
              }
              switch (r)
              {
                case 0 : f *= Idb2[x] ; break;
                case 1 : f *= Idb2[y] ; break;
                case 2 : f *= Idb2[z] ; break;
              }
              switch (s)
              {
                case 0 : f *= Ib2[x] ; break;
                case 1 : f *= Ib2[y] ; break;
                case 2 : f *= Ib2[z] ; break;
              }
              E[z][y][x] += f;
            }
          }
      }
}

void RegularizedSplineF::computeSplineEnergyMatrix()
{
  initializeSplineEnergy();
  const int ncp = nx*ny*nz;
  energyMatrix.resize(ncp*ncp);
  for (int i=0;i<ncp*ncp;i++) energyMatrix[i] = 0;
  float e = 0;
  for (int iz=0;iz<nz;iz++)
  {
    for (int iy=0;iy<ny;iy++)
    {
      for (int ix=0;ix<nx;ix++)
      {
        int idxI = iz * nx*ny + iy * nx + ix;
        for (int jz=iz-3;jz<=iz+3;jz++)
        {
          if (jz<0) continue;
          if (jz>=nz) break;
          for (int jy=iy-3;jy<=iy+3;jy++)
          {
            if (jy<0) continue;
            if (jy>=ny) break;
            for (int jx=ix-3;jx<=ix+3;jx++)
            {
              if (jx<0) continue;
              if (jx>=nx) break;
              int idxJ = jz * nx*ny + jy * nx + jx;
              e += CP[idxJ] * CP[idxI] * E[abs(iz-jz)][abs(iy-jy)][abs(ix-jx)];
              energyMatrix[idxI * ncp + idxJ] = E[abs(iz-jz)][abs(iy-jy)][abs(ix-jx)];
            }
          }
        }
      }
    }
  }
}

float RegularizedSplineF::computeBeta()
// compute the beta value for the conjugate gradient method,
// as per Section 1.6, equation 6.36 of _Nonlinear Programming_
// by Dimitri P. Bertsekas (1993) p. 127.
// \beta = \frac{ g^{(k)}'(g^{(k)}-g^{(k-1)})}{g^{(k-1)}'g^{(k-1)}}
{
  const int nCP = cpsize;
  float den=0;
  float num=0;
  for (int i=0; i<nCP;i++)
  {
    num += grad[i]*(grad[i]-gradPrev[i]);
    den += gradPrev[i] * gradPrev[i];
  }
  return num/den;
}

RegularizedSplineF::~RegularizedSplineF()
// delete all allocated memory
{
  for (int i=0;i<4;i++)
  {
    delete[] bx[i];
    delete[] by[i];
    delete[] bz[i];
  }
  delete[] CP;
  delete[] splineImg;
}

void RegularizedSplineF::create(int x, int y, int z, int x2, int y2, int z2)
// creates a spline for an image of size (x,y,z), with
// control points spaced (x2,y2,z2) points apart.
// Computes the sampled spline functions.
// Allocates and initializes all memory, except for the 
// memory requiring knowledge of the mask.
{
  cx = x;
  dx = x2;
  // NOTE:  Number of points are adjusted for non-multiples!
  nx = x/dx+3+1;
  cy = y;
  dy = y2;
  ny = y/dy+3+1;
  cz = z;
  dz = z2;
  nz = z/dz+3+1;
  cpsize = nx*ny*nz;
  imagesize = cx*cy*cz;
  for (int i=0;i<4;i++)
  {
    delete[] bx[i];		bx[i] = new float[dx];
    delete[] by[i];		by[i] = new float[dy];
    delete[] bz[i];		bz[i] = new float[dz];
  }
  float u = 0.0f;
  float uStep = 1.0f / dx;
  for (int i = 0;i<dx;i++)
  {
    float u2 = u*u;
    float u3 = u*u*u;
    bx[0][i] = u3/6.0f;
    bx[1][i] = (1 + 3*u + 3*u2 - 3*u3)/6.0f;
    bx[2][i] = (4 -       6*u2 + 3*u3)/6.0f;
    bx[3][i] = (1 - 3*u + 3*u2 -   u3)/6.0f;
    u += uStep;
  }
  u = 0.0f;
  uStep = 1.0f / dy;
  for (int i = 0;i<dy;i++)
  {
    float u2 = u*u;
    float u3 = u*u*u;
    by[0][i] = u3/6.0f;
    by[1][i] = (1 + 3*u + 3*u2 - 3*u3)/6.0f;
    by[2][i] = (4 -       6*u2 + 3*u3)/6.0f;
    by[3][i] = (1 - 3*u + 3*u2 -   u3)/6.0f;
    u += uStep;
  }
  u = 0.0f;
  uStep = 1.0f / dz;
  for (int i = 0;i<dz;i++)
  {
    float u2 = u*u;
    float u3 = u*u*u;
    bz[0][i] = u3/6.0f;
    bz[1][i] = (1 + 3*u + 3*u2 - 3*u3)/6.0f;
    bz[2][i] = (4 -       6*u2 + 3*u3)/6.0f;
    bz[3][i] = (1 - 3*u + 3*u2 -   u3)/6.0f;
    u += uStep;
  }
  delete[] CP;
  delete[] direx;
  CP = new float[cpsize];
  grad.resize(cpsize);
  gradR.resize(cpsize);
  gradS.resize(cpsize);
  gradPrev.resize(cpsize);
  direx = new float[cpsize];
  for (int i=0;i<cpsize;i++)
  {
    CP[i] = 1.0f;
  }
}

void RegularizedSplineF::updateCP(float alpha)
// adjust the direction according to the update rule,
// d^{k} = - g^{k} + B^{k}d^{k-1}
{
  const int nCP = cpsize;
  for (int i=0; i<nCP;i++)
    CP[i] += alpha * direx[i];
}

float RegularizedSplineF::cgm()
{
  Timer timer;
  timer.start();
  int vcount = 0;
  for (int i=0;i<imagesize;i++)
    if (mask[i]!=0) vcount++;
  imagecount = vcount;
  const int nCP = cpsize;
  init();
  if (verbosity>1)
  {
    std::cout<<"Fitting spline to bias estimate samples (at most "<<stopCount<<" iterations)"<<std::endl;
  }
  if (verbosity>2)
  {
    std::cout<<"Conjugate Gradient Method iteration 1/"<<stopCount<<std::endl;
  }
  float e = computeResidual();
  float en0= splineCrossEnergy(CP,CP);
  e += splineLambda * en0;

  computeGradient();
  for (int i=0; i<nCP;i++) direx[i] = -grad[i];
  float alpha = computeAlpha();
  updateCP(alpha);
  compute(CP);
  const int N=stopCount;
  int count = 0;
  for (int i=1;i<N;i++)
  {
    if (verbosity>2)
    {
      std::cout<<"Conjugate Gradient Method iteration "<<i+1<<"/"<<N<<". Error: "<<e<<std::endl;
    }
    float lastE = e;
    e = computeResidual();
    e += splineLambda * splineCrossEnergy(CP,CP);
    if ((e<stopError)||((lastE - e)<stopDelta))
    {
      if (++count>20) break;
    }
    computeGradient();
    float beta  = computeBeta();
    for (int j=0; j<nCP;j++)
      direx[j] = -grad[j] + beta * direx[j];
    float alpha = computeAlpha();
    updateCP(alpha);
    compute(CP);
  }
  if (verbosity>1)
  {
    std::ostringstream ostr;
    std::cout<<"Conjugate Gradient Method finished. Final error: "<<e<<std::endl;
  }
  return 0;
}

void RegularizedSplineF::resetCP()
{
  for (int i=0;i<cpsize;i++)
  {
    CP[i] = 1.0f;
  }
}

float RegularizedSplineF::computeAlpha()
// Optimal computation of alpha for the conjugate gradient step.
// Result appears in Bertsekas.
{
  compute(direx,"Computing Alpha");
  float den = 0;
  float num = 0;
  for (int i=0;i<imagecount;i++)
  {
    den += square(splineImg[i]);
    num += splineImg[i] * residual[i];
  }
  num -= splineLambda * splineCrossEnergy(direx,CP);
  den += splineLambda * splineCrossEnergy(direx,direx);
  return num/den;
}

void RegularizedSplineF::setMask(uint8 *m)
// Set the mask pointer.
// Compute the number of brain voxels.
// Allocate memory for the residual and spline images.
{
  mask = m;
  int vcount=0;
  for (int i=0;i<imagesize;i++)
    if (mask[i]!=0) vcount++;
  if (verbosity>1) std::cout<<"spline mask has "<<vcount<<" non-zero points."<<std::endl;
  delete[] splineImg;
  residual.resize(vcount);
  splineImg = new float[vcount];
  imagecount = vcount;
}

void RegularizedSplineF::init()
// The initial reconstruction results from a FLAT bias,
// so set the spline equal to the reconstructed image.
{
  computeSplineEnergyMatrix();
  const int datasize = cx*cy*cz;
  int sIndex = 0;
  int i;
  for (i=0;i<datasize;i++)
    if (mask[i]!=0)
      splineImg[sIndex++] = 1.0;
  for (i=0;i<cpsize;i++)
  {
    CP[i] = 1.0f;
  }
}

float RegularizedSplineF::computeResidual()
// Computes the error vector of the spline to
// the measured data.  Returns the squared error.
{
  float e2=0.0f;
  const int slicesize = cx*cy;
  int rIndex = 0;
  int index = 0;
  for (int z = 0;z<cz;z++)
  {
    for (int i = 0;i<slicesize;i++)
    {
      if (mask[index]!=0)
      {
        float e = measured[rIndex] - splineImg[rIndex];
        residual[rIndex] = e;
        rIndex++;
        e2 += square(e);
      }
      index++;
    }
  }
  return e2;
}

void RegularizedSplineF::computeGradient()
// computes the gradient of the function.
// f = |x-Bp|^2 + \splineLambda p' E p
// g = -2(B)'(x-Bp)
//   = -2(B)'R
{
  const int ngp = nx*ny*nz;
  for (int i=0;i<ngp;i++)
  {
    gradPrev[i] = grad[i];
    grad [i] = 0.0f;
    gradR[i] = 0.0f;
    gradS[i] = 0.0f;
  }
  int uz=0;
  int cpz=3;
  int index = 0;
  int rIndex = 0;
  const int nx_=nx;
  const int ny_=ny;
  for (int z=0;z<cz;z++,uz++)
  {
    if (uz>=dz) { uz -= dz; cpz++;}
    int uy=0;
    int cpy=3;
    for (int y=0;y<cy;y++,uy++)
    {
      if (uy>=dy) { uy -= dy; cpy++;}
      int ux=0;
      int cpx=3;
      for (int x=0;x<cx;x++,ux++)
      {
        if (ux>=dx) { ux -= dx; cpx++;}
        if (mask[index]!=0)
        {
          float R = 2*residual[rIndex++];
          for (int q=0;q<4;q++)
            for (int r=0;r<4;r++)
            {
              float f = R * bz[q][uz] * by[r][uy];
              for (int s=0;s<4;s++)
              {
                gradR[(cpz-q)*nx_*ny_ + (cpy-r)*nx_ + (cpx-s)] -= f * bx[s][ux];
              }
            }
        }
        index++;
      }
    }
  }
  dEdp(gradS);
  for (int i=0;i<ngp;i++)
  {
    grad[i] = gradR[i]/imagecount + splineLambda * gradS[i];
  }
}

void RegularizedSplineF::compute(float *controlPoints, std::string /*desc*/)
// computes the spline, given a set of control points.
{
  int uz=0;
  int cpz=3;
  int index = 0;
  int sIndex = 0;
  for (int z=0;z<cz;z++,uz++)
  {
    if (uz>=dz) { uz -= dz; cpz++;}
    int uy=0;
    int cpy=3;
    for (int y=0;y<cy;y++,uy++)
    {
      if (uy>=dy) { uy -= dy; cpy++;}
      int ux=0;
      int cpx=3;
      for (int x=0;x<cx;x++,ux++)
      {
        if (ux>=dx) { ux -= dx; cpx++;}
        float val = 0.0f;
        if (mask[index]!=0)
        {
          for (int q=0;q<4;q++)
            for (int r=0;r<4;r++)
              for (int s=0;s<4;s++)
              {
                val += controlPoints[(cpz-q)*nx*ny + (cpy-r)*nx + (cpx-s)]
                    * bz[q][uz] * by[r][uy] * bx[s][ux];
              }
          splineImg[sIndex++] = val;
        }
        else
          val = 1.0f;
        index++;
      }
    }
  }
}

// energy computations.
float32 RegularizedSplineF::splineCrossEnergy(float32 *p1, float32 *p2)
{
  const int ncp = nx*ny*nz;
  float32 e = 0;
  for (int i=0;i<ncp;i++)
    for (int j=0;j<ncp;j++)
    {
      e += p1[i]*energyMatrix[i*ncp+j]*p2[j];
    }
  return e/ncp;
}


void RegularizedSplineF::dEdp(std::vector<float> &vec)
// first derivative of energy with respect to control points...
{
  const int ncp = nx*ny*nz;
  const float scale = 2.0f/ncp;
  for (int i=0;i<ncp;i++)
  {
    for (int j=0;j<ncp;j++)
    {
      vec[i] = scale * energyMatrix[i*ncp+j] * CP[j];
    }
  }
}
