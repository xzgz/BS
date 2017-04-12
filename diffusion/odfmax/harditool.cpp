// Copyright (C) 2016 The Regents of the University of California
//
// Created by David W. Shattuck, Ph.D.
//
// This file is part of BrainSuite16a1.
//
// BrainSuite16a1 is free software; you can redistribute it and/or
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

#include <harditool.h>
#include <iostream>
#include <vol3d_t.h>
#ifdef QT_BUILD
#include <QFileInfo>
#endif

Vol3DInstance(Coefficients)

HARDITool::HARDITool() : isFRACT(false), samplingRes(20), filecount(0)
{
  odfFactory.makeBasis(samplingRes);
}

void HARDITool::updateBasis()
{
  odfFactory.makeBasis(samplingRes);
}

bool HARDITool::makeODFVolume(Vol3D<Coefficients> &vODF, Vol3D<float32> vODFC[], const int nCoeffs)
{
  vODF.makeCompatible(vODFC[0]);
  vODF.set(0.0f);
  const int ds = vODF.size();
  for (int i=0;i<nCoeffs;i++)
  {
    if (vODF.isCompatible(vODFC[i]))
    {
      for (int p=0;p<ds;p++)
        vODF[p].d[i] = vODFC[i][p];
    }
  }
  return true;
}

int HARDITool::setMaxCoefficient(const int n)
{
  odfFactory.nCoefficients = n;
  if (odfFactory.nCoefficients>filecount) odfFactory.nCoefficients = filecount;
  return odfFactory.nCoefficients;
}

inline float HARDITool::l2norm(const Coefficients &coeff, const size_t n)
{
  const size_t stop=ODFFactory::NBasisFunctions<n ? ODFFactory::NBasisFunctions : n;
  float ip=0.0f;
  for (size_t i=0;i<stop;i++)
    ip += coeff[i]*coeff[i];
  return sqrtf(ip);
}

inline float HARDITool::generalizedFractionalAnisotropy(const Coefficients &coeff, const size_t n)
{
  const size_t stop=ODFFactory::NBasisFunctions<n ? ODFFactory::NBasisFunctions : n;
  float ip=0.0f;
  for (size_t i=0;i<stop;i++)
    ip += coeff[i]*coeff[i];
  return (ip>0) ? sqrtf(1-coeff[0]*coeff[0]/ip) : 0;
}

inline bool HARDITool::nonzero(const Coefficients &coeff, const size_t n)
{
  const size_t stop=ODFFactory::NBasisFunctions<n ? ODFFactory::NBasisFunctions : n;
  for (size_t i=1;i<stop;i++) if (coeff[i]) return true;
  return false;
}

void HARDITool::computeGFA(Vol3D<float32> &vGFA)
{
  vGFA.makeCompatible(vODF);
  const int ds=vGFA.size();
  bool isNonzero=false;
  for (int i=0;i<ds;i++)
  {
    if (vODF[i][0]) { isNonzero=true; break; }
  }
  isFRACT = (isNonzero==false);
  if (isFRACT)
  {
    std::cerr<<"Warning: loading FRACT data -- unable to compute GFA. Showing l2 norm in image window."<<std::endl;
    for (int i=0;i<ds;i++)
    {
      vGFA[i]=l2norm(vODF[i],odfFactory.nCoefficients);
    }
  }
  else
    for (int i=0;i<ds;i++)
    {
      vGFA[i]=generalizedFractionalAnisotropy(vODF[i],odfFactory.nCoefficients);
    }
}

void HARDITool::computeGFA()
{
  computeGFA(vGFA);
}

typedef std::pair<float,size_t> MagKey;

struct sortPred {
  bool operator()(const std::pair<float,size_t> &left, const std::pair<float,size_t> &right) {
    return left.first > right.first;
  }
};

EigenSystem3x3f HARDITool::findODFMaxima(const int x, const int y, const int z) const
{
  const size_t nv = odfFactory.sphereBase.vertices.size();
  const size_t jEnd = odfFactory.nCoefficients;
  std::vector<float> radii;
  radii.resize(nv);
  const Coefficients &shc = vODF(x,y,z);
  float maxR = 0;
  size_t maxRid=0;
  std::vector<MagKey> magnitudes(nv);
  for (size_t v=0;v<nv;v++)
  {
    float R=0;
    for (size_t j=0;j<jEnd;j++)
    {
      R += shc[j]*odfFactory.sphereValues[j][v];
    }
    magnitudes[v].first = (R>0) ? R : 0;
    magnitudes[v].second = v;
    radii[v] = (R>0) ? R : 0;
    if (R>maxR) { maxRid = v; maxR = R; }
  }
  std::sort(magnitudes.begin(),magnitudes.end(),sortPred( ));
  EigenSystem3x3f eigenVoxel;
  eigenVoxel.l0 = magnitudes[0].first;
  eigenVoxel.v0 = odfFactory.sphereBase.vertices[magnitudes[0].second];
  eigenVoxel.l1 = magnitudes[2].first;
  eigenVoxel.v1 = odfFactory.sphereBase.vertices[magnitudes[2].second];
  eigenVoxel.l2 = magnitudes[4].first;
  eigenVoxel.v2 = odfFactory.sphereBase.vertices[magnitudes[4].second];
  return eigenVoxel;
}

bool HARDITool::loadSHClist(std::string hardiFilename)
{
//  std::cout<<"reading hardi file description: "<<hardiFilename<<std::endl;
  std::ifstream ifile(hardiFilename.c_str());
  if (!ifile) { std::cerr<<"couldn't open "<<hardiFilename<<std::endl; return false; }
  std::string path;
#ifdef QT_BUILD
  QFileInfo fileInfo(hardiFilename.c_str());
  path = fileInfo.canonicalPath().toStdString();
  path += "/";
#endif
  char buffer[1024];
  filecount=0;
  ifile>>std::ws;
  while (!ifile.eof())
  {
    ifile.getline(buffer,1024);
    int ns = ifile.gcount();
    if (ns==0) break;
    if (buffer[ns-2]==13) buffer[ns-2]=0;
    std::cout<<"loading "<<buffer<<std::endl;
    std::string filename = path + buffer;
    if (vODFC[filecount++].read(filename)==false)
    {
      std::cerr<<"error reading "<<filename<<std::endl;
      return false;
    }
    if (filecount>MAX_ODFs)
    {
      std::cerr<<"error: filecount exceeds maximum number of spherical harmonic coefficients allowed ("<<MAX_ODFs<<")"<<std::endl;
      break;
    }
    ifile>>std::ws;
  }
  makeODFVolume(vODF,vODFC,filecount);
#ifdef QT_BUILD
  computeGFA();
#endif
  odfFactory.nCoefficients = filecount;
  return true;
}

