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

#ifndef IndexedCurveSet_H
#define IndexedCurveSet_H

#include <list>
#include <dspoint.h>
#include <uvpoint.h>
#include <algorithm>
#include <iostream>

class IndexedCurvePoint {
public:
  IndexedCurvePoint(const DSPoint &p, const int vid=-1) : p(p), vid(vid) {}
  DSPoint p;
  int vid;
};

class SurfaceAlpha;

class IndexedCurve {
public:
  class ClickInfo {
  public:
    int vid;
    std::list<IndexedCurvePoint>::iterator clickPoint;
  };
  class Attributes {
  public:
    Attributes() : color(1,0,0), lineWidth(1), pointWidth(1) {}
    std::string name;
    DSPoint color;
    float lineWidth;
    float pointWidth;
  };
  int lastPointIndex() const
  {
    return (clickPoints.size()>0) ? clickPoints.back().vid : -1;
  }
  Attributes attributes;
  std::list<IndexedCurvePoint> points;
  std::list<UVPoint> uvPoints;
  std::list<ClickInfo> clickPoints;
};

inline std::ostream &operator<<(std::ostream &s, const IndexedCurve::ClickInfo &rewindInfo)
{
  return s<<rewindInfo.vid;
}

class IndexedCurveSet {
public:
  IndexedCurveSet();
  IndexedCurve & current() { return *currentCurve; }
  bool select(IndexedCurve *curve);
  bool remove(std::list<IndexedCurve>::iterator curve);
  std::list<IndexedCurve>::iterator currentIterator() { return currentCurve; }
  bool write(std::string ofname);
  void writeDFC(std::string ofname);
  bool writeUCF(std::string ofname);
  bool writeMultipleUCF(std::string prefix);
  void writeSVG(std::string ofname);

  bool read(std::string ifname);
  bool readDFC(std::string ifname);
  bool readOBJ(std::string ifname);
  bool readUCF(std::string ifname);
  void readUCFLevel(std::istream &ifile);
  bool readTRK(std::string ifname);
  void addPoint(IndexedCurvePoint &p)
  {
    if (curves.size()<=0) newCurve();
    currentCurve->points.push_back(p);
  }
  void addPoint(IndexedCurvePoint &p, UVPoint &uv)
  {
    if (curves.size()<=0) newCurve();
    currentCurve->points.push_back(p);
    currentCurve->uvPoints.push_back(uv);
  }
  void clearAll()
  {
    curves.clear();
    newCurve();
  }
  void clearLast()
  {
    if (curves.size()>0)
    {
      if (curves.back().points.size()==0)
      {
        curves.pop_back();
      }
      curves.pop_back();
    }
    newCurve();
  }
  void newCurve();
  void draw();
  void draw(SurfaceAlpha *surface);
  void drawFlat();
  size_t nCurves() { return curves.size(); }
  std::list<IndexedCurve> curves;
  bool useDefaultLineWidth;
  float defaultLineWidth;
protected:
  std::list<IndexedCurve>::iterator currentCurve;
};

#endif
