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

#ifndef CollisionList_H
#define CollisionList_H

#include <set>
#include <vol3d.h>
#include <vector>

class CollisionData {
public:
  std::set<int> triangleList;
};

namespace SILT { class Surface; }

class CollisionVolume : public Vol3D<CollisionData> {
public:
  CollisionVolume() : verbosity(1) {}
  void build(SILT::Surface &s, const float radius);
  void clear();
  inline int clamp(const float f, const int /*minv*/, const int maxv) const
  {
    int iv = (int)f;
    return (iv>=0) ? ((iv<=maxv) ? iv : maxv) : 0;
  }
  IPoint3D position(const DSPoint p) const
  {
    const int x = clamp(p.x / rx, 0, cx-1);
    const int y = clamp(p.y / ry, 0, cy-1);
    const int z = clamp(p.z / rz, 0, cz-1);
    return  IPoint3D(x,y,z);
  }
  int ptIndex(const DSPoint p) const
  {
    const int x = clamp(p.x / rx, 0, cx-1);
    const int y = clamp(p.y / ry, 0, cy-1);
    const int z = clamp(p.z / rz, 0, cz-1);
    return index(x,y,z);
  }
  CollisionData* list(const DSPoint p) const
  {
    return Vol3D<CollisionData>::start() + ptIndex(p);
  }
  void groupD26(CollisionData &collisionData, const DSPoint p) const; // find all neighbors in the local nbhd
  CollisionData& value(const DSPoint p) const
  {
    return Vol3D<CollisionData>::start()[ptIndex(p)];
  }
  int t(const int n) { return triangleIndex[n]; }
  size_t nt() const { return triangleIndex.size(); }
  SILT::Surface *builtWith() const { return surface; }
  int verbosity;
protected:
  void add(const int triangleID, const int vertexID, const DSPoint p);
  std::vector<int> triangleIndex;
  SILT::Surface *surface;
};

#endif
