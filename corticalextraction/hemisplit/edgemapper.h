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

#ifndef EdgeMapper_H
#define EdgeMapper_H

#include <triple.h>
#include <surface.h>
#include <map>

class EdgeMapper {
public:
  typedef Triple<int> TriangleNeighborList;
  typedef std::pair<int,int> TriangleEdge;
  class TrianglePair {
  public:
    TrianglePair() : a(-1), b(-1) { }
    int a,b;
    bool addTriangle(const int triangleID)
    {
      if (a<0) { a=triangleID; return true; }
      if (b<0) { b=triangleID; return true; }
      return false;
    }
  };
  typedef	std::map<TriangleEdge,TrianglePair > EdgeMap;
  void mapTriangleNeighbors(SILT::Surface &surface);
  void addNeighbor(TriangleNeighborList &neighbors, int triangleID);
  void insert(EdgeMap &map, int a, int b, int triangleID);
  EdgeMap edgeMap;
  std::vector<TriangleNeighborList> triangleNeighbors;
};

#endif
