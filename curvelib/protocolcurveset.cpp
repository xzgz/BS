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

#include <fstream>
#include <sstream>
#include <strutil.h>
#include <protocolcurveset.h>
#include <dfcheader.h>
#include <rapidxml.hpp>
#include <DS/timer.h>
#include <xmlbuffer.h>
#include <rapidxml_utils.hpp>
#include <DS/getfilesize.h>
#include <cstring>

inline DSPoint str2rgb(std::string colorString)
{
  int h=0;
  std::istringstream(colorString)>>std::hex>>h;
  float r=((h>>16)&0xFF)/255.0f;
  float g=((h>>8)&0xFF)/255.0f;
  float b=((h)&0xFF)/255.0f;
  return DSPoint(r,g,b);
}

std::string readAttributeNode(rapidxml::xml_node<char>* headNode, std::string attribute)
{
  if (headNode)
  {
    rapidxml::xml_attribute<char>* attributeNode=headNode->first_attribute(attribute.c_str());
    if (attributeNode)
    {
      return attributeNode->value();
    }
  }
  return "";
}

bool parseCurveProtocolXML(std::string &protocolName, std::vector<ProtocolCurve> &curves, rapidxml::xml_document<char> &doc)
{
  rapidxml::xml_node<>* cur_node = doc.first_node("CurveSet");
  curves.resize(0);
  if (cur_node)
  {
    cur_node = cur_node->first_node("Curve");
    if (cur_node)
    {
      while (cur_node)
      {
        ProtocolCurve pc;
        rapidxml::xml_attribute<char> *nameNode = cur_node->first_attribute("name");
        pc.attributes.name = nameNode->value();
        nameNode = cur_node->first_attribute("color");
        std::istringstream istr(nameNode->value());
        istr>>pc.attributes.color;
        curves.push_back(pc);
        cur_node = cur_node->next_sibling("Curve");
      }
    }
  }
  else
  {
    rapidxml::xml_node<>* cur_node = doc.first_node("BrainSuiteCurveProtocol");
    if (!cur_node)
    {
      cur_node = doc.first_node("LONICurveProtocol");
      if (!cur_node) return false;
    }
    protocolName=::readAttributeNode(cur_node,"name");
    cur_node = cur_node->first_node("Curve",0,false);
    while (cur_node)
    {
      ProtocolCurve pc;
      pc.attributes.name=::readAttributeNode(cur_node,"name");
      pc.attributes.color=::str2rgb(readAttributeNode(cur_node,"defaultcolor"));
      pc.protocol.name=::readAttributeNode(cur_node,"name");
      pc.protocol.tag=::readAttributeNode(cur_node,"tag");
      pc.protocol.description=readAttributeNode(cur_node,"description");
      pc.protocol.start=readAttributeNode(cur_node,"start");
      pc.protocol.direction=readAttributeNode(cur_node,"direction");
      pc.protocol.stop=readAttributeNode(cur_node,"stop");
      pc.protocol.notes=readAttributeNode(cur_node,"notes");
      pc.protocol.href=readAttributeNode(cur_node,"href");
      pc.protocol.required=StrUtil::eqnocase(readAttributeNode(cur_node,"required"),"true");
      curves.push_back(pc);
      cur_node = cur_node->next_sibling("Curve",0,false);
    }
  }
  return true;
}

bool ProtocolCurveSet::readCurveProtocol(std::string ifname)
{
  int filesize=getFileSize(ifname);
  if (!filesize) return false;
  std::ifstream ifile(ifname.c_str());
  if (!ifile) return false;
  std::vector<char> buffer(filesize+1);
  ifile.read(&buffer[0],filesize);
  buffer[filesize]=0;
  rapidxml::xml_document<char> doc;
  doc.parse<0>(&buffer[0]);
  parseCurveProtocolXML(name,curves,doc);
  return true;
}

bool ProtocolCurveSet::parseXMLCurveAttributes(std::string &name, std::vector<ProtocolCurve> &curves, std::vector<char> &buffer)
{
  using namespace rapidxml;
  xml_document<> doc;    // character type defaults to char
  doc.parse<parse_declaration_node | parse_no_data_nodes>(&buffer[0]);    // 0 means default parse flags
  return parseCurveProtocolXML(name, curves,doc);
}

bool ProtocolCurveSet::readXML(std::string ifname)
{
  return readCurveProtocol(ifname);
}

bool ProtocolCurveSet::read(std::string ifname)
{
  if (StrUtil::hasExtension(ifname,".ucf"))
  {
    return readUCF(ifname);
  }
  std::ifstream ifile(ifname.c_str(),std::ios::binary);
  if (!ifile)
  {
    std::cerr<<"unable to open "<<ifname<<std::endl;
    return false;
  }
  DFCHeader hdr;
  ifile.read((char *)&hdr,sizeof(hdr));
  if (hdr.dataStart<=0) return false;
  int metadatasize = hdr.dataStart - hdr.mdoffset;
  std::vector<char> buffer(metadatasize+1);
  ifile.seekg(hdr.mdoffset,std::ios_base::beg);
  ifile.read(&buffer[0],metadatasize);
  buffer[metadatasize] = 0;
  parseXMLCurveAttributes(name,curves,buffer);
  ifile.seekg(hdr.dataStart,std::ios_base::beg);
  if (hdr.nContours != curves.size())
  {
    std::cerr<<"curve sizes do not match: "<<hdr.nContours<<" != "<<curves.size()<<std::endl;
    return false;
  }

  for (size_t i=0;i<hdr.nContours;i++)
  {
    int nPoints = 0;
    ifile.read((char *)&nPoints,sizeof(nPoints));
    curves[i].points.clear();
    curves[i].points.resize(nPoints);
    ifile.read((char *)&curves[i].points[0],sizeof(DSPoint)*nPoints);
  }
  return true;
}

inline std::string safestring(std::string istr)
{
  size_t pos = istr.find('"');
  const char *quotestring = "&quot;";
  while (pos != istr.npos)
  {
    istr.replace(pos,1,quotestring);
    pos = istr.find('"',pos);
  }
  return istr;
}

inline std::string rgb2hex(unsigned int ref)
{
  static char hexcode[] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' };
  char buf[10];
  buf[0]=hexcode[(ref>>20) & 15];
  buf[1]=hexcode[(ref>>16) & 15];
  buf[2]=hexcode[(ref>>12) & 15];
  buf[3]=hexcode[(ref>>8) & 15];
  buf[4]=hexcode[(ref>>4) & 15];
  buf[5]=hexcode[(ref   ) & 15];
  buf[6]=0;
  return std::string(buf);
}

inline std::string rgbf2hex(DSPoint color)
{
  if (color.x<0) color.x=0;
  if (color.x>1) color.x=1;
  if (color.y<0) color.y=0;
  if (color.y>1) color.y=1;
  if (color.z<0) color.z=0;
  if (color.z>1) color.z=1;
  uint32 r=uint32(color.x*255)&0xFF;
  uint32 g=uint32(color.y*255)&0xFF;
  uint32 b=uint32(color.z*255)&0xFF;
  return rgb2hex(r<<16|g<<8|b);
}

XMLBuffer &operator<<(XMLBuffer &stream, const ProtocolCurveSet &curveProtocol)
{
  stream<<"<?xml version='1.0' encoding='utf-8'?>\n";
  stream<<"<BrainSuiteCurveProtocol name=\""<<curveProtocol.name<<"\">\n";
  const std::vector<ProtocolCurve> &curves = curveProtocol.curves;
  for (auto curve = curves.begin();curve!=curves.end();curve++)
  {
    stream<<"<curve";
    stream<<" name=\""<<curve->protocol.name<<"\"";
    stream<<" tag=\""<<curve->protocol.tag<<"\"";
    if (!curve->protocol.description.empty()) stream<<" description=\""<<safestring(curve->protocol.description)<<"\"";
    if (!curve->protocol.start.empty()) stream<<" start=\""<<safestring(curve->protocol.start)<<"\"";
    if (!curve->protocol.direction.empty()) stream<<" direction=\""<<curve->protocol.direction<<"\"";
    if (!curve->protocol.stop.empty()) stream<<" stop=\""<<safestring(curve->protocol.stop)<<"\"";
    if (!curve->protocol.notes.empty()) stream<<" notes=\""<<safestring(curve->protocol.notes)<<"\"";
    if (!curve->protocol.href.empty()) stream<<" href=\""<<safestring(curve->protocol.href)<<"\"";
    stream<<" defaultcolor=\""<<rgbf2hex(curve->attributes.color)<<"\"";
    stream<<" required=\""<<(curve->protocol.required ? "true" : "false")<<"\"";
    stream<<"/>\n";
  }
  stream<<"</BrainSuiteCurveProtocol>";
  return stream;
}

bool ProtocolCurveSet::writeDFC(std::string ofname)
{
  std::ofstream ofile(ofname.c_str(),std::ios::binary);
  if (!ofile) return false;
  XMLBuffer metadata;
  metadata << *this;
  metadata.close();
  DFCHeader hdr;
  hdr.dataStart = sizeof(hdr) + (uint32)metadata.size();
  hdr.nContours = (uint32)curves.size();
  hdr.mdoffset = sizeof(hdr);
  ofile.write((char *)&hdr,sizeof(hdr));
  ofile.write(metadata.buffer.str().c_str(),metadata.size());
  std::vector<ProtocolCurve>::iterator curve = curves.begin();
  if ((int)ofile.tellp()!=(int)hdr.dataStart)
  {
    std::cerr<<"write error: "<<std::endl;
    std::cerr<<"File pos: "<<ofile.tellp()<<std::endl;
    std::cerr<<"Expected: "<<hdr.dataStart<<std::endl;
  }
  for (size_t i=0;i<hdr.nContours;i++,curve++)
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
  return true;
}

class UCFParser {
public:
  static const char *tokens[];
  static const int nTokens;
  static int parse(const char *str);
};

const char *UCFParser::tokens[] =
{
  "width",
  "height",
  "xrange",
  "yrange",
  "zrange",
  "levels",
  "level number",
  "point_num",
  "contour_data",
  "end of level"
};

const int UCFParser::nTokens = sizeof(UCFParser::tokens)/sizeof(UCFParser::tokens[0]);

int UCFParser::parse(const char *str)
{
  for (int i=0;i<nTokens;i++)
    if (strncmp(str,tokens[i],strlen(tokens[i]))==0) return i;
  return -1;
}

typedef std::pair<float,float> Pair;

inline std::istream &operator>>(std::istream &s, Pair &p)
{
  return s>>p.first>>p.second;
}

inline std::ostream &operator<<(std::ostream &s, Pair &p)
{
  return s<<p.first<<' '<<p.second;
}

void ProtocolCurveSet::readUCFLevel(std::istream &ifile)
{
  char buf[512];
  int levelNum = -1;
  int nPoints = -1;
  ifile>>levelNum;
  for (;;)
  {
    ifile>>std::ws;
    ifile.getline(buf,512);
    if ((ifile.gcount()==0)&&ifile.eof()) { std::cout<<"eof."<<std::endl; break; }
    if (buf[0]=='<')
    {
      switch (UCFParser::parse(buf+1))
      {
        case 0 :
        case 1 :
        case 2 :
        case 3 :
        case 4 :
        case 5 : std::cerr<<"Error reading UCF: Bad form."<<std::endl; break;
        case 6 : ifile>>levelNum; break;
        case 7 : ifile>>nPoints; break;
        case 8 :
        {          // need to set attributes
          if ((curves.size()==0)||(curves.back().points.size()>0)) curves.push_back(ProtocolCurve());
          for (int i=0;i<nPoints;i++)
          {
            DSPoint point;
            ifile>>point;
            curves.back().points.push_back(point);
          }
        }
        case 9 : return;
        default:
          std::cout<<"can't interpret "<<buf<<std::endl;
          break;
      }
    }
  }
}

bool ProtocolCurveSet::readUCF(std::string ifname)
{
  std::ifstream ifile(ifname.c_str());
  if (!ifile) return false;
  int width = 0;
  int height = 0;
  Pair xrange;
  Pair yrange;
  Pair zrange;
  int levels = 0;
  char buf[512];
  for (;;)
  {
    ifile>>std::ws;
    ifile.getline(buf,512);
    if (ifile.gcount()==0) if (ifile.eof()) break;
    if (buf[0]=='<')
    {
      switch (UCFParser::parse(buf+1))
      {
        case 0 : ifile>>width; break;
        case 1 : ifile>>height; break;
        case 2 : ifile>>xrange; break;
        case 3 : ifile>>yrange; break;
        case 4 : ifile>>zrange; break;
        case 5 : ifile>>levels; break;
        case 6 : readUCFLevel(ifile); break;
        default:
          break;
      }
    }
  }
  return true;
}
