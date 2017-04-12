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

#include <iostream>
#include <fibertrackset.h>
#include <trkheader.h>
#include <mat3.h>
#include <DS/timer.h>
#include <rapidxml.hpp>
#include <strutil.h>
#include <xmlbuffer.h>
#include <dftheader.h>
#include <strutil.h>

FiberTrackSet::FiberTrackSet()
{
}

void FiberTrackSet::readNextTrack(std::ifstream &ifile, FiberTrackSet &set, const SILT::Mat3<float> &xform, const DSPoint &shift)
{
  int np=0;
  ifile.read((char *)&np,sizeof(np));
  DSPoint point;
  set.newCurve(np);
  DSPoint color = DSPoint(0,0,0);
  if (np>0)
  {
    ifile.read((char *)&point,sizeof(point));
    point = xform * point+shift;
    set.addPoint(point);
    DSPoint lastPoint = point;
    for (int i=1;i<np;i++)
    {
      ifile.read((char *)&point,sizeof(point));
      point = xform * point + shift;
      color += abs(lastPoint - point);
      set.addPoint(point);
    }
    set.curves.back().attributes.color = color.unit();
  }
}

DSPoint FiberTrackSet::computeTrackOrientation(char code)
{
  switch (code)
  {
    case 'R' : return DSPoint( 1,0,0);
    case 'L' : return DSPoint(-1,0,0);
    case 'A' : return DSPoint(0, 1,0);
    case 'P' : return DSPoint(0,-1,0);
    case 'S' : return DSPoint(0,0, 1);
    case 'I' : return DSPoint(0,0,-1);
  }
  return DSPoint(1,0,0);
}

SILT::Mat3<float> FiberTrackSet::getTrackMatrix(TrkHeader &hdr)
{
  DSPoint e1=computeTrackOrientation(hdr.voxel_order[0]);
  DSPoint e2=computeTrackOrientation(hdr.voxel_order[1]);
  DSPoint e3=computeTrackOrientation(hdr.voxel_order[2]);
  return SILT::Mat3<float> ( e1.x,e1.y,e1.z, e2.x,e2.y,e2.z, e3.x,e3.y,e3.z );
}

bool FiberTrackSet::read(std::string ifname)
{
  if (StrUtil::hasExtension(ifname,".trk")) return readTRK(ifname);
  if (StrUtil::hasExtension(ifname,".dft")) return readDFT(ifname);
  if (StrUtil::hasExtension(ifname,".dfc")) return readDFT(ifname);
  std::cerr<<ifname<<" has an unknown format."<<std::endl;
  return false;
}

bool FiberTrackSet::readTRK(std::string ifname)
{
  TrkHeader hdr;
  std::ifstream ifile(ifname.c_str(),std::ios::binary);
  if (!ifile) return false;
  ifile.read((char *)&hdr,sizeof(hdr));
  SILT::Mat3<float> m=getTrackMatrix(hdr);
  DSPoint volumeSizes = DSPoint(hdr.dim[0]*hdr.voxel_size[0],hdr.dim[1]*hdr.voxel_size[1],hdr.dim[2]*hdr.voxel_size[2]);
  DSPoint lastVoxel = m * (volumeSizes-DSPoint(1,1,1));
  float shiftx = (lastVoxel.x<0) ? -lastVoxel.x : 0;
  float shifty = (lastVoxel.y<0) ? -lastVoxel.y : 0;
  float shiftz = (lastVoxel.z<0) ? -lastVoxel.z : 0;
  DSPoint d = DSPoint(shiftx,shifty,shiftz);
  curves.clear();
  if (hdr.n_count==0)
  {
    int count=0;
    while (!ifile.eof())
    {
      readNextTrack(ifile,*this,m,d);
      count++;
    }
  }
  else
  {
    this->curves.reserve(hdr.n_count);
    this->curves.resize(0);
    for (int i=0;i<hdr.n_count;i++)
    {
      readNextTrack(ifile,*this,m,d);
    }
  }
  return true;
}

void FiberTrackSet::addCurve(std::vector<DSPoint> &points, int seedIndex, DSPoint color)
{
  curves.push_back(FiberTrack());
  curves.back().points=points;
  curves.back().attributes.color = color;
  curves.back().seedIndex=seedIndex;
}

void FiberTrackSet::newCurve(const size_t reserve)
{
  static DSPoint colorSet[] =
  {
    DSPoint(0,0,1),
    DSPoint(0,1,0),
    DSPoint(0,1,1),
    DSPoint(1,0,0),
    DSPoint(1,0,1),
    DSPoint(1,1,0),
  };
  static const int nColors = sizeof(colorSet)/sizeof(colorSet[0]);
  FiberTrack c;
  curves.push_back(c);
  if (reserve)
    curves.back().points.reserve(reserve);
  curves.back().attributes.color = colorSet[(nCurves()-1)%nColors];
}

XMLBuffer &operator<<(XMLBuffer &stream, FiberTrackSet &curveSet)
{
  stream<<"<?xml version='1.0' encoding='utf-8'?>\n";
  std::vector<FiberTrack> &curves = curveSet.curves;
  stream<<"<CurveSet>\n";
  for (std::vector<FiberTrack>::iterator curve = curves.begin();curve!=curves.end();curve++)
  {
    stream<<"<Curve"<<" name=\""<<curve->attributes.name<<"\""<<" color=\""<<curve->attributes.color<<"\""<<"/>\n";
  }
  stream<<"</CurveSet>";
  return stream;
}

bool FiberTrackSet::write(std::string ofname) // currently only supports DFT, but can add more formats
{
  return writeDFT(ofname);
}

bool FiberTrackSet::writeDFT(std::string ofname)
{
  std::ofstream ofile(ofname.c_str(),std::ios::binary);
  if (!ofile) return false;
  XMLBuffer metadata;
  metadata << *this;
  metadata.close();
  DFTHeader hdr;
  const auto nTracks=curves.size();
  std::vector<sint32> seedPoints(nTracks);
  for (size_t i=0;i<nTracks;i++) seedPoints[i]=curves[i].seedIndex;
  hdr.nTracks = nTracks;
  hdr.mdoffset = sizeof(hdr);
  hdr.dataStart = sizeof(hdr) + (sint32)metadata.size();
  hdr.seedPoints = nTracks;

  ofile.write((char *)&hdr,sizeof(hdr));
  ofile.write(metadata.buffer.str().c_str(),metadata.size());

  std::vector<FiberTrack>::iterator curve = curves.begin();
  if ((int)ofile.tellp()!=(int)hdr.dataStart)
  {
    std::cerr<<"error writing dft"<<std::endl;
    std::cerr<<"file pos = "<<ofile.tellp()<<std::endl;
    std::cerr<<"expected "<<hdr.dataStart<<std::endl;
  }
  for (size_t i=0;i<hdr.nTracks;i++,curve++)
  {
    if (curve==curves.end()) break;
    std::vector<DSPoint> &points = curve->points;
    unsigned int nPoints = (unsigned int)points.size();
    ofile.write((char *)&nPoints,sizeof(nPoints));
    std::vector<DSPoint>::const_iterator piter = curve->points.begin();
    const std::vector<DSPoint>::const_iterator pEnd = curve->points.end();
    for (;piter!=pEnd;piter++)
    {
      ofile.write((char *)&piter->x,sizeof(DSPoint));
    }
  }
  ofile.write((char *)&seedPoints[0],sizeof(seedPoints[0])*seedPoints.size());
  return true;
}

inline DSPoint FiberTrackSet::str2pt(std::string str)
{
  std::istringstream istr(str.c_str());
  DSPoint p;
  istr>>p.x>>p.y>>p.z;
  return p;
}

bool FiberTrackSet::parseXMLFiberAttributes(std::list<FiberTrack::Attributes> &fiberAttributes, std::vector<char> &buffer)
{
  using namespace rapidxml;
  Timer parseTimer;
  parseTimer.start();
  try {
    xml_document<> doc; // character type defaults to char
    doc.parse<parse_declaration_node | parse_no_data_nodes>(&buffer[0]); // 0 means default parse flags
    xml_node<>* cur_node = doc.first_node("CurveSet");
    if (cur_node)
    {
      cur_node = cur_node->first_node("Curve");
      if (cur_node)
      {
        while (cur_node)
        {
          FiberTrack::Attributes attributeIn;
          xml_attribute<char> *nameNode = cur_node->first_attribute("name");
          attributeIn.name = nameNode->value();
          nameNode = cur_node->first_attribute("color");
          attributeIn.color = str2pt(nameNode->value());
          fiberAttributes.push_back(attributeIn);
          cur_node = cur_node->next_sibling("Curve");
        }
      }
    }
  }
  catch(rapidxml::parse_error &error)
  {
    std::cerr<<"RapidXML exception when parsing metadata for fiber track file"<<error.what()<<std::endl;
    return false;
  }
  return true;
}

bool FiberTrackSet::readDFT(std::string ifname)
{
  XMLBuffer metadata;
  std::ifstream ifile(ifname.c_str(),std::ios::binary);
  if (!ifile) return false;
  DFTHeader hdr;
  const Version vMin(1,0,0,3);
  ifile.read((char *)&hdr,sizeof(hdr));
  if (hdr.dataStart<=0) return false;
  if (hdr.version.hex()<vMin.hex()) { hdr.seedPoints=0; }
  std::list<FiberTrack::Attributes> curveAttributes;
  if (hdr.mdoffset>0)
  {
    int metadatasize = hdr.dataStart - hdr.mdoffset;
    std::vector<char> buffer(metadatasize+1);
    ifile.seekg(hdr.mdoffset,std::ios_base::beg);
    ifile.read(&buffer[0],metadatasize);
    buffer[metadatasize] = 0;
    parseXMLFiberAttributes(curveAttributes,buffer);
    ifile.seekg(hdr.dataStart,std::ios_base::beg);
  }
  std::list<FiberTrack::Attributes>::iterator attribute = curveAttributes.begin();
  Timer curveTimer;
  curveTimer.start();
  curves.clear();
  curves.resize(hdr.nTracks);
  for (size_t i=0;i<hdr.nTracks;i++)
  {
    curves[i]=FiberTrack();
    if (attribute != curveAttributes.end())
      curves[i].attributes = *attribute++;
    int nPoints = 0;
    ifile.read((char *)&nPoints,sizeof(nPoints));
    curves[i].points.resize(nPoints);
    ifile.read((char *)&(curves[i].points[0]),sizeof(DSPoint)*nPoints);
  }
  if (hdr.seedPoints)
  {
    std::vector<uint32> seedPoints(hdr.nTracks);
    ifile.read((char *)&seedPoints[0],sizeof(int)*hdr.nTracks);
    for (uint32 i=0;i<hdr.nTracks;i++)
    {
      curves[i].seedIndex=seedPoints[i];
    }
  }
  return true;
}

typedef std::pair<DSPoint, size_t> IndexPair;

inline bool compare(const IndexPair& a, const IndexPair& b)
{
  return (a.first.z<b.first.z) ? true : (a.first.z>b.first.z) ? false : (a.first.y<b.first.y) ? true : (a.first.y>b.first.y) ? false : (a.first.x<b.first.x);
}

void FiberTrackSet::sortFiberTracks()
{
  size_t nc=curves.size();
  std::vector<IndexPair> pairs(nc);
  for (size_t i=0;i<nc;i++)
  {
    FiberTrack &x=curves[i];
    pairs[i].first = (x.seedIndex>=0 && x.seedIndex < (int)x.points.size()) ? x.points[x.seedIndex] : x.points[0];
    pairs[i].second = i;
  }
  std::sort(pairs.begin(),pairs.end(),compare);
  std::vector<FiberTrack> sortedCurves(nc);
  for (size_t i=0;i<nc;i++)
  {
    FiberTrack &s=curves[pairs[i].second];
    FiberTrack &d=sortedCurves[i];
    d.attributes=s.attributes;
    d.seedIndex=s.seedIndex;
    d.points=std::move(s.points);
  }
  curves=std::move(sortedCurves);
}
