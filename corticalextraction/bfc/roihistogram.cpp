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

#include <BFC/roihistogram.h>
#include <vol3dbase.h>

ROIHistogram::ROIHistogram() : rx(0), ry(0), rz(0)
{
  for (int i=0;i<256;i++) count[i] = 0;
}

size_t ROIHistogram::nVoxels() const
{
  return deltas.size();
}

void ROIHistogram::setup(Vol3DBase &v, const float radius_, const ROIType roiType_)
{
// assumes the radius is in x-voxels.
  roiType = roiType_;
  radius = radius_ * v.rx;
  rx = v.rx;
  ry = v.ry;
  rz = v.rz;
  if (roiType==Box)
  {
    int nx = (int)(radius / v.rx);
    int ny = (int)(radius / v.ry);
    int nz = (int)(radius / v.rz);
    deltas.reserve((2*nx+1)*(2*ny+1)*(2*nz+1));
    for (int z=-nz;z<=nz;z++)
      for (int y=-ny;y<=ny;y++)
        for (int x=-nx;x<=nx;x++)
        {
          deltas.push_back(IPoint3D(x,y,z));
        }
  }
  else
  {
    int nx = (int)::ceilf(radius / v.rx);
    int ny = (int)::ceilf(radius / v.ry);
    int nz = (int)::ceilf(radius / v.rz);
    deltas.reserve((2*nx+1)*(2*ny+1)*(2*nz+1));
    const float r2 = radius*radius;
    const float rx2= v.rx*v.rx;
    const float ry2= v.ry*v.ry;
    const float rz2= v.rz*v.rz;
    for (int z=-nz;z<=nz;z++)
      for (int y=-ny;y<=ny;y++)
        for (int x=-nx;x<=nx;x++)
        {
          if ((rx2*x*x+ry2*y*y+rz2*z*z)<r2)
          {
            deltas.push_back(IPoint3D(x,y,z));
          }
        }
  }
}

void ROIHistogram::compute(Vol3D<uint8> &vImage, int x, int y, int z)
{
  const int cx = vImage.cx;
  const int cy = vImage.cy;
  const int cz = vImage.cz;
  for (int i=0;i<256;i++) count[i] = 0;
  const size_t np = deltas.size();
  const IPoint3D center = IPoint3D(x,y,z);
  centroid = DSPoint((float)x,(float)y,(float)z);
  DSPoint localCentroid;
  int localTotal=0;
  for (size_t i=0;i<np;i++)
  {
    IPoint3D p = center + deltas[i];
    if ((p.x<0)||(p.x>=cx)||(p.y<0)||(p.y>=cy)||(p.z<0)||(p.z>=cz)) continue;
    const uint8 value=vImage(p);
    count[value]++;
    if (value) localCentroid+=deltas[i];
    localTotal++;
  }
  total = localTotal - count[0];
  if (total>0) centroid += localCentroid/total;
  count[0] = 0;
}

void ROIHistogram::compute(Vol3D<uint8> &vImage, Vol3D<uint8> &vMask, int x, int y, int z)
{
  const int cx = vImage.cx;
  const int cy = vImage.cy;
  const int cz = vImage.cz;
  for (int i=0;i<256;i++) count[i] = 0;
  const size_t np = deltas.size();
  const IPoint3D center = IPoint3D(x,y,z);
  centroid = DSPoint((float)x,(float)y,(float)z);
  DSPoint localCentroid;
  int localTotal=0;
  for (size_t i=0;i<np;i++)
  {
    IPoint3D p = center + deltas[i];
    if ((p.x<0)||(p.x>=cx)||(p.y<0)||(p.y>=cy)||(p.z<0)||(p.z>=cz)) continue;
    if (vMask(p))
    {
      const uint8 value=vImage(p);
      count[value]++;
      if (value) localCentroid+=deltas[i];
      localTotal++;
    }
  }
  total = localTotal - count[0];
  if (total>0) centroid += localCentroid/total;
  count[0] = 0;
}
