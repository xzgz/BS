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

#include "edgemapper.h"

void EdgeMapper::insert(EdgeMap &map, int a, int b, int triangleID)
{
  if (a<b)
  {
    map[TriangleEdge(a,b)].addTriangle(triangleID);
  }
  else if (b<a)
  {
    map[TriangleEdge(b,a)].addTriangle(triangleID);
  }
  else std::cerr<<"degenerate edge: "<<a<<" - "<<b<<std::endl;
}

void EdgeMapper::addNeighbor(TriangleNeighborList &neighbors, int triangleID)
{
  if (neighbors.a<0) neighbors.a = triangleID;
  else if (neighbors.b<0) neighbors.b = triangleID;
  else if (neighbors.c<0) neighbors.c = triangleID;
  else { std::cerr<<"degenerate!"<<std::endl; }
}

void EdgeMapper::mapTriangleNeighbors(SILT::Surface &surface)
{
  const size_t nt = surface.nt();
  for (size_t i=0;i<nt;i++)
  {
    Triangle &t = surface.triangles[i];
    insert(edgeMap,t.a,t.b,(int)i);
    insert(edgeMap,t.b,t.c,(int)i);
    insert(edgeMap,t.c,t.a,(int)i);
  }
  int zc=0,sc=0,tc=0;
  triangleNeighbors.resize(nt,TriangleNeighborList(-1,-1,-1));
  for (EdgeMap::iterator e = edgeMap.begin(); e != edgeMap.end(); e++)
  {
    const int a = e->second.a;
    const int b = e->second.b;
    if (a<0) zc++;
    else if (b<0) sc++;
    else
    {
      tc++;
      addNeighbor(triangleNeighbors[a],b);
      addNeighbor(triangleNeighbors[b],a);
    }
  }
}
