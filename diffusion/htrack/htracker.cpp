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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vol3d.h>
#include <htracker.h>

template <class DataType>
inline DataType nearestNeighbor(DSPoint &p, const Vol3D<DataType> &vol)
{
  const float px = p.x/vol.rx + 0.5f;
  const float py = p.y/vol.ry + 0.5f;
  const float pz = p.z/vol.rz + 0.5f;
  const int x = (int)px;
  const int y = (int)py;
  const int z = (int)pz;
  if ((x>=0)&&(y>=0)&&(z>0)&&(x<(int)vol.cx)&&(y<(int)vol.cy)&&(z<(int)vol.cz))
  {
    return vol(x,y,z);
  }
  DataType v;
  return v;
}

inline DSPoint samplePoint(DSPoint &p, const Vol3D<DSPoint> &vol)
{
  const float px = p.x/vol.rx + 0.5f;
  const float py = p.y/vol.ry + 0.5f;
  const float pz = p.z/vol.rz + 0.5f;
  const int x = (int)px;
  const float ax = px - x;
  const int y = (int)py;
  const float ay = py - y;
  const int z = (int)pz;
  const float az = pz - z;
  const int offset = vol.index(x,y,z);
  const DSPoint *src = vol.start();
  const int cx = vol.cx;
  const int cy = vol.cy;
  const int ss = cx * cy;
  const int sx = vol.cx-1;
  const int sy = vol.cy-1;
  const int sz = vol.cz-1;
  DSPoint vec;
  if ((x>=0)&&(y>=0)&&(z>0)&&(x<sx)&&(y<sy)&&(z<sz))
  {
    vec = ((1-az)*(
             (1-ay) * ((1-ax)*src[offset]    + ax*src[offset+1])
           +(  ay) * ((1-ax)*src[offset+cx] + ax*src[offset+cx+1]))
        +(az)*(
          (1-ay) * ((1-ax)*src[offset+ss]    + ax*src[offset+ss+1])
        +(  ay) * ((1-ax)*src[offset+ss+cx] + ax*src[offset+ss+cx+1])));
  }
  return vec;
}


inline DSPoint align(const DSPoint &newVec, const DSPoint &targetVec)
{
  return (newVec.dot(targetVec)>0) ? newVec : -newVec;
}

inline DSPoint samplePointAlign(DSPoint &p, const DSPoint &direx, const Vol3D<DSPoint> &vol)
{
  const float px = p.x/vol.rx + 0.5f;
  const float py = p.y/vol.ry + 0.5f;
  const float pz = p.z/vol.rz + 0.5f;
  const int x = (int)px;
  const float ax = px - x;
  const int y = (int)py;
  const float ay = py - y;
  const int z = (int)pz;
  const float az = pz - z;
  const int offset = vol.index(x,y,z);
  const DSPoint *src = vol.start();
  const int cx = vol.cx;
  const int cy = vol.cy;
  const int ss = cx * cy;
  const int sx = vol.cx-1;
  const int sy = vol.cy-1;
  const int sz = vol.cz-1;
  DSPoint vec;
  if ((x>=0)&&(y>=0)&&(z>0)&&(x<sx)&&(y<sy)&&(z<sz))
  {
    vec = ((1-az)*(
                   (1-ay) * ((1-ax)*align(src[offset],direx)    + ax*align(src[offset+1],direx))
                  +(  ay) * ((1-ax)*align(src[offset+cx],direx) + ax*align(src[offset+cx+1],direx)))
            +(az)*(
                   (1-ay) * ((1-ax)*align(src[offset+ss],direx)    + ax*align(src[offset+ss+1],direx))
                  +(  ay) * ((1-ax)*align(src[offset+ss+cx],direx) + ax*align(src[offset+ss+cx+1],direx))));
  }
  return vec;
}


inline DSPoint voxel2WC(const Vol3DBase &v, DSPoint pV)
{
  return DSPoint (v.rx * pV.x, v.ry * pV.y, v.rz * pV.z);
}

template <class Interpolator, class Integrator>
bool HTracker<Interpolator, Integrator>::track(std::list<DSPoint> &fiberTrack, int &seedIndex, const Vol3D<EigenSystem3x3f> &vMax, DSPoint seedPoint, const float fibertrackDotProductThresh) const
{
  if (voxels) seedPoint = voxel2WC(vMax,seedPoint);
  DSPoint v0=nearestNeighbor(seedPoint,vMax).v0;
  if ((v0.x==0)&&(v0.y==0)&&(v0.z==0)) return false;
  fiberTrack.push_back(seedPoint);
  DSPoint p = seedPoint;
  DSPoint prev=v0;
  int nf=0;
  for (int i=0;i<nSteps;i++)
  {
    DSPoint direx=Integrator::template eval<Interpolator>(p,prev,vMax,stepsize);
    if (direx.x==0&&direx.y==0&&direx.z==0) break;
    DSPoint d=direx.unit();
    if (i>0)
    {
      if (d.dot(prev)<fibertrackDotProductThresh) break;
    }
    nf++;
    p += stepsize * direx;
    prev = direx.unit();
    fiberTrack.push_back(p);
  }
  p = seedPoint;
  prev = -v0;
  int nb=0;
  for (int i=0;i<nSteps;i++)
  {
    DSPoint direx=Integrator:: template eval<Interpolator>(p,prev,vMax,stepsize);
    if (direx.x==0&&direx.y==0&&direx.z==0) break;
    DSPoint d=direx.unit();
    if (i>0)
    {
      if (d.dot(prev)<fibertrackDotProductThresh) break;
    }
    nb++;
    p += stepsize * direx;
    prev = direx;
    fiberTrack.push_front(p);
  }
  seedIndex=nb; // nb points were added in front of the seed, so seed index is nb
  return true;
}

#include <vectorinterpolator.h>
#include <alignfull.h>
#include <rungekutta4.h>
// default tracker
template bool HTracker<VectorInterpolator<AlignFull> , RungeKutta4>::track(std::list<DSPoint> &tract, int &seedIndex,const Vol3D<EigenSystem3x3f> &vMax, DSPoint seedPoint, const float fibertrackDotProductThresh) const;
