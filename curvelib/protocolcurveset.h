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

#ifndef ProtocolCurveset_H
#define ProtocolCurveset_H

#include <string>
#include <vector>
#include <list>
#include <dspoint.h>
#include <uvpoint.h>

class ProtocolCurve {
public:
  class Protocol {
  public:
    Protocol() : required(false) {}
    std::string name;
    std::string tag;
    std::string start;
    std::string stop;
    std::string notes;
    std::string direction;
    std::string description;
    std::string href;		// hyperlink
    unsigned int color; // 32 bit color code
    bool required;
  };
  class Attributes {
  public:
    Attributes() : color(1,0,0), lineWidth(1), pointWidth(1) {}
    std::string name;
    DSPoint color;
    float lineWidth;
    float pointWidth;
  };
  class ClickInfo {
  public:
    ClickInfo(int vid=-1,size_t index=0) : vid(vid), index(index) {}
    int vid;
    size_t index;
  };
  int lastPointIndex() const
  {
    return (clickPoints.size()>0) ? clickPoints.back().vid : -1;
  }
  Protocol protocol;
  Attributes attributes;
  std::vector<int> vertexIDs;
  std::vector<DSPoint> points;
  std::vector<UVPoint> uvPoints;
  std::list<ClickInfo> clickPoints;
  void clear()
  {
    clickPoints.clear();
    points.clear();
    uvPoints.clear();
    vertexIDs.clear();
  }
};

class ProtocolCurveSet {
public:
  bool write(std::string ofname);
  bool writeXML(std::string ofname);
  bool writeDFC(std::string ofname);
  bool read(std::string ofname);
  void readUCFLevel(std::istream &ifile);
  bool readUCF(std::string ofname);
  bool readXML(std::string ofname);
  bool parseXMLCurveAttributes(std::string &name, std::vector<ProtocolCurve> &curves, std::vector<char> &buffer);
  bool readCurveProtocol(std::string ifname);
  std::string name;
  std::vector<ProtocolCurve> curves;
  void drawFlat();
  bool exportUCFs(std::string prefix);
};

#endif
