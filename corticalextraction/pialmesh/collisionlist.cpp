// Copyright (C) 2016 The Regents of the University of California
//
// Created by David W. Shattuck, Ph.D.
//
// This file is part of Pialmesh.
//
// Pialmesh is free software; you can redistribute it and/or
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

#include "collisionlist.h"
#include <surface.h>
#include <DS/timer.h>

typedef std::pair<DSPoint,DSPoint> Extent;

Extent computeExtent(SILT::Surface &s)
// TODO: move to surflib
{
  const size_t nv = s.nv();
  if (nv>0)
  {
    DSPoint *v = &s.vertices[0];
    DSPoint minpt = v[0];
    DSPoint maxpt = minpt;
    for (size_t i=1;i<nv;i++)
    {
      if (v[i].x < minpt.x) minpt.x = v[i].x; else if (v[i].x>maxpt.x) maxpt.x = v[i].x;
      if (v[i].y < minpt.y) minpt.y = v[i].y; else if (v[i].y>maxpt.y) maxpt.y = v[i].y;
      if (v[i].z < minpt.z) minpt.z = v[i].z; else if (v[i].z>maxpt.z) maxpt.z = v[i].z;
    }
    return Extent(minpt,maxpt);
  }
  else
    return Extent();
}


void CollisionVolume::add(const int triangleID, const int /*vertexID*/, const DSPoint p)
{
  const int idx = ptIndex(p);
  triangleIndex[triangleID] = idx;
  CollisionData *s = start() + idx;
  s->triangleList.insert(triangleID);
}

void CollisionVolume::groupD26(CollisionData &collisionData, const DSPoint p) const // find all neighbors in the local nbhd
{
  IPoint3D pos = position(p);
  collisionData.triangleList.clear();
  for (int dz=-1;dz<=1;dz++)
  {
    int z = pos.z+dz;
    if ((z<0)||(z>=(int)cz)) continue;
    for (int dy=-1;dy<=1;dy++)
    {
      int y = pos.y+dy;
      if ((y<0)||(y>=(int)cy)) continue;
      for (int dx=-1;dx<=1;dx++)
      {
        int x = pos.x+dx;
        if ((x<0)||(x>=(int)cx)) continue;
        CollisionData &cd = data[index(x,y,z)];
        collisionData.triangleList.insert(cd.triangleList.begin(),cd.triangleList.end());
      }
    }
  }
}

void CollisionVolume::build(SILT::Surface &s, const float radius)
{
  if (verbosity>2)
    std::cout<<"building collision system."<<std::endl;
  Timer t; t.start();
  const size_t nt = s.nt();
  triangleIndex.resize(nt);
  for (size_t i=0;i<nt;i++) triangleIndex[i]=0;
  DSPoint *v = &s.vertices[0];
  Extent e = computeExtent(s);
  if (verbosity>2)
    std::cout<<"range is "<<e.first<<" : "<<e.second<<std::endl;
  IPoint3D vsize;
  vsize.x = int(e.second.x / radius) + 1;
  vsize.y = int(e.second.y / radius) + 1;
  vsize.z = int(e.second.z / radius) + 1;
  if (verbosity>2)
  {
    std::cout<<"creating cube of vsize  "<<vsize.x<<'\t'<<vsize.y<<'\t'<<vsize.z<<std::endl;
    std::cout<<"current size of cube is "<<cx<<'\t'<<cy<<'\t'<<cz<<std::endl;
  }
  if (cx==0)
  {
    vsize.x += int(ceil(20.0/radius));
    vsize.y += int(ceil(20.0/radius));
    vsize.z += int(ceil(20.0/radius));
    if (verbosity>2) std::cout<<"enlarging to "<<vsize.x<<'\t'<<vsize.y<<'\t'<<vsize.z<<std::endl;
  }
  if (verbosity>2)
  {
    std::cout<<"setting size from " <<cx*cy*cz<<'('<<allocated()<<")\tto\t"<<vsize.x*vsize.y*vsize.z<<std::endl;
  }
  clear();
  setsize(vsize.x,vsize.y,vsize.z);
  if (verbosity>2)
  {
    std::cout<<"set size to: "<<cx*cy*cz<<'('<<allocated()<<")\tor\t"<<vsize.x*vsize.y*vsize.z<<std::endl;
    std::cout<<"allocated space is "<<sizeof(CollisionData) * allocated()<<std::endl;
  }
  rx = radius;
  ry = radius;
  rz = radius;

  float maxR2=0;
  for (size_t i=0;i<nt;i++)
  {
    const int a = s.triangles[i].a;
    const int b = s.triangles[i].b;
    const int c = s.triangles[i].c;
    DSPoint center((v[a]+v[b]+v[c])/3.0f);
    float da = (v[a]-center).pwr();
    float db = (v[b]-center).pwr();
    float dc = (v[c]-center).pwr();
    if (da>maxR2) maxR2=da;
    if (db>maxR2) maxR2=db;
    if (dc>maxR2) maxR2=dc;
    add(int(i),a,center);
  }
  t.stop();
  if (verbosity>2) std::cout<<"building collision volume took "<<t.elapsed()<<std::endl;
}

void CollisionVolume::clear()
{
  const int ds = size();
  for (int i=0;i<ds;i++)
  {
    data[i].triangleList.clear();
  }
}

