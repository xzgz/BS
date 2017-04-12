// Copyright (C) 2016 The Regents of the University of California
//
// Created by David W. Shattuck, Ph.D.
//
// This file is part of Hemisplit.
//
// Hemisplit is free software; you can redistribute it and/or
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

#include <DS/timer.h>
#include <graph.h>
#include <surface.h>
#include <brainsplitter.h>
#include <map>
#include <list>
#include <algorithm>
#include "edgemapper.h"
#include <surflib.h>

void BrainSplitter::labelTriangles(std::vector<uint8> &triangleID, const SILT::Surface &brain)
{
  const size_t nt = brain.nt();
  triangleID.resize(nt,255);
  for (size_t i=0;i<nt;i++)
  {
    int a = brain.vertexLabels[brain.triangles[i].a];
    int b = brain.vertexLabels[brain.triangles[i].b];
    int c = brain.vertexLabels[brain.triangles[i].c];
    if ((a==b)&&(a==c))
    {
      triangleID[i] = a;
    }
    else
      triangleID[i] = 1; // double check this!
  }
}

int BrainSplitter::segmentSurface(std::vector<int> &surfaceLabel, const SILT::Surface &brain)
{
  const size_t nt = brain.nt();
  Graph graph((int)nt);
  graph.reset((int)nt);
  int nLinks = 0;
  int nBroken = 0;
  for (size_t i=0;i<nt;i++)
  {
    Triangle t = brain.triangles[i];
    const int a = t.a;
    const int b = t.b;
    const int c = t.c;
    if (brain.vertexLabels[a]==brain.vertexLabels[b])
    {
      graph.link(a,b);
      nLinks++;
    }
    else nBroken++;
    if (brain.vertexLabels[b]==brain.vertexLabels[c])
    {
      graph.link(b,c);
      nLinks++;
    }
    else nBroken++;
    if (brain.vertexLabels[c]==brain.vertexLabels[a])
    {
      graph.link(c,a);
      nLinks++;
    }
    else nBroken++;
  }
  return graph.makemap(&surfaceLabel[0]);
}

void BrainSplitter::initLabels(SILT::Surface &brain, Vol3D<uint8> &vLabel)
{
  static const Point3D<int> offsets[] = {
    Point3D<int>(0,0,0),
    Point3D<int>(0,0,1),
    Point3D<int>(0,1,0),
    Point3D<int>(0,1,1),
    Point3D<int>(1,0,0),
    Point3D<int>(1,0,1),
    Point3D<int>(1,1,0),
    Point3D<int>(1,1,1),
  };
  const int cx = vLabel.cx;
  const int cy = vLabel.cy;
  const int cz = vLabel.cz;
  const float rx = vLabel.rx;
  const float ry = vLabel.ry;
  const float rz = vLabel.rz;
  const Point3D<int> dims(cx,cy,cz);
  const size_t nv = brain.nv();
  const DSPoint *p = &(brain.vertices[0]);
  brain.vertexLabels.resize(nv);
  for (size_t i=0;i<nv;i++)
  {
    Point3D<int> ip(clamp(p[i].x/rx,cx),clamp(p[i].y/ry,cy),clamp(p[i].z/rz,cz));
    for (int j=0;j<8;j++)
    {
      Point3D<int> ip2 = ip + offsets[j];
      if (under(ip2,dims))
      {
        int idx = vLabel.index(ip2.x,ip2.y,ip2.z);
        if (vLabel[idx])
        {
          brain.vertexLabels[i] = vLabel[idx];
          break;
        }
      }
    }
  }
}

size_t makeMap(std::vector<int> &map, const Surface &surface)
{
  const size_t nv = surface.nv();
  std::vector<bool> flag(nv,false);
  map.resize(nv);
  std::fill(map.begin(),map.end(),-1);

  const size_t nt = surface.nt();
  for (size_t i=0;i<nt;i++)
  {
    flag[surface.triangles[i].a] = true;
    flag[surface.triangles[i].b] = true;
    flag[surface.triangles[i].c] = true;
  }
  size_t vcount = 0;
  for (size_t i=0;i<nv;i++)
  {
    if (flag[i])
    {
      map[i] = (int)vcount++;
    }
  }
  return vcount;
}

void removeUnusedVertices(std::vector<int> &map, SILT::Surface &surface)
{
  const size_t nv = surface.nv();
  const size_t nt = surface.nt();
  Triangle *t = &surface.triangles[0];
  DSPoint *v = &surface.vertices[0];
  size_t vcount = makeMap(map,surface);
  for (size_t i=0;i<nt;i++)
  {
    t[i].a = map[t[i].a];
    t[i].b = map[t[i].b];
    t[i].c = map[t[i].c];
  }
  std::vector<DSPoint> newlist(vcount);
  for (size_t i=0;i<nv;i++)
  {
    if (map[i]>=0)
    {
      newlist[map[i]] = v[i];
    }
  }
  surface.vertices.resize(vcount);
  for (size_t i=0;i<vcount;i++)
    surface.vertices[i] = newlist[i];

  if (surface.vertexColor.size()==nv)
  {
    for (size_t i=0;i<nv;i++)
    {
      if (map[i]>=0)
        newlist[map[i]] = surface.vertexColor[i];
    }
    surface.vertexColor.resize(vcount);
    for (size_t i=0;i<vcount;i++)
      surface.vertexColor[i] = newlist[i];
  }
  if (surface.vertexNormals.size()==nv)
  {
    for (size_t i=0;i<nv;i++)
    {
      if (map[i]>=0)
        newlist[map[i]] = surface.vertexNormals[i];
    }
    surface.vertexNormals.resize(vcount);
    for (size_t i=0;i<vcount;i++)
      surface.vertexNormals[i] = newlist[i];
  }
  if (surface.vertexUV.size()==nv)
  {
    std::vector<UVPoint> newlist(vcount);
    for (size_t i=0;i<nv;i++)
    {
      if (map[i]>=0)
        newlist[map[i]] = surface.vertexUV[i];
    }
    surface.vertexUV.resize(vcount);
    for (size_t i=0;i<vcount;i++)
      surface.vertexUV[i] = newlist[i];
  }
}

bool BrainSplitter::mapVertices(SILT::Surface &dst, SILT::Surface &src, std::vector<int> &map)
{
  const size_t np = map.size();
  int n =0;
  for (size_t i=0;i<np;i++)
  {
    if (map[i]>=0)
    {
      dst.vertices[map[i]] = src.vertices[i];
      n++;
    }
  }
  if (src.vertexColor.size()==src.nv())
  {
    dst.vertexColor.resize(dst.nv());
    for (size_t i=0;i<np;i++)
    {
      if (map[i]>=0)
      {
        dst.vertexColor[map[i]] = src.vertexColor[i];
      }
    }
  }
  if (src.vertexAttributes.size()==src.nv())
  {
    dst.vertexAttributes.resize(dst.nv());
    for (size_t i=0;i<np;i++)
    {
      if (map[i]>=0)
      {
        dst.vertexAttributes[map[i]] = src.vertexAttributes[i];
      }
    }
  }
  return true;
}


bool BrainSplitter::split(SILT::Surface &brain, Vol3D<uint8> &vLabel,
                          SILT::Surface &left, SILT::Surface &right,
                          std::vector<int>  &leftMap,
                          std::vector<int>  &rightMap)
{
  Timer t;
  t.start();
  const size_t nv = brain.nv();
  brain.vertexLabels.resize(nv);
  brain.computeConnectivity();
  initLabels(brain,vLabel);
  EdgeMapper edgeMapper;
  edgeMapper.mapTriangleNeighbors(brain);

  size_t zeroCount=0;
  for (size_t i=0;i<nv;i++)
  {
    if (brain.vertexLabels[i])
    {
      brain.vertexLabels[i] = 1 + brain.vertexLabels[i]%2;
    }
    else
    {
      brain.vertexLabels[i] = 0;
      zeroCount++;
    }
  }
  std::vector<int> map(brain.nt());
  const int nSymbols = segmentSurface(map, brain);
  int mx = map[0];
  int mn = map[0];
  for (int i=0;i<nSymbols;i++)
  {
    if (map[i]>mx) mx = map[i];
    if (map[i]<mn) mn = map[i];
  }
  std::vector<int> hist(mx+1,0);
  for (size_t i=0;i<nv;i++)
  {
    hist[map[i]]++;
  }
  std::vector<std::pair<int,int> > v(mx+1);
  for (int i=0;i<=mx;i++)
  {
    v[i] = std::pair<int,int>(i,hist[i]);
  }
  std::sort(v.begin(),v.end(),biggest);
  const int firstID = v[0].first;
  const int secondID = v[1].first;
  for (size_t i=0;i<nv;i++)
  {
    if ((map[i]!=firstID)&&(map[i]!=secondID))
    {
      switch (brain.vertexLabels[i])
      {
        case 0: break;
        case 1: brain.vertexLabels[i] = 2; break;
        case 2: brain.vertexLabels[i] = 1; break;
      }
    }
  }
  SurfLib::fastFill(brain);
  brain.vertexColor.resize(nv);
  std::fill(brain.vertexColor.begin(),brain.vertexColor.end(),DSPoint(0.7f,0.7f,0.7f));
  {
    std::vector<uint8> triangleID;
    labelTriangles(triangleID,brain);
    const size_t nt=brain.nt();
    for (int nIter=0;nIter<10;nIter++)
    {
      int degen=0;
      for (size_t i=0;i<nt;i++)
      {
        EdgeMapper::TriangleNeighborList tn = edgeMapper.triangleNeighbors[i];
        if (tn.a<0||tn.b<0||tn.c<0) continue;
        const int id=triangleID[i];
        if (id==triangleID[tn.a]) continue;
        if (id==triangleID[tn.b]) continue;
        if (id==triangleID[tn.c]) continue;

        triangleID[i]=triangleID[tn.a];
        Triangle t = brain.triangles[i];
        brain.vertexColor[t.a] = DSPoint(1,0,0);
        brain.vertexColor[t.b] = DSPoint(1,0,0);
        brain.vertexColor[t.c] = DSPoint(1,0,0);
        degen++;
      }
      if (degen>0) std::cout<<"fixed "<<degen<<" isolated triangles."<<std::endl;
      if (degen==0) break;
    }
    grabSurface(left,  brain, triangleID, 1);
    grabSurface(right, brain, triangleID, 2);
    removeUnusedVertices(leftMap,left);
    removeUnusedVertices(rightMap,right);
  }
  t.stop();
  return true;
}

void BrainSplitter::grabSurface(SILT::Surface &left, const SILT::Surface &brain, const std::vector<uint8> &triangleID, const int CODE)
{
  const size_t nt = brain.nt();
  int tcount = 0;
  for (size_t i=0;i<nt;i++)
    if (triangleID[i]==CODE) tcount++;
  left.triangles.resize(tcount);
  tcount = 0;
  for (size_t i=0;i<nt;i++)
  {
    if (triangleID[i]==CODE) left.triangles[tcount++] = brain.triangles[i];
  }
  left.vertices=brain.vertices;
  left.vertexColor=brain.vertexColor;
  left.vertexUV=brain.vertexUV;
  left.vertexLabels=brain.vertexLabels;
}
