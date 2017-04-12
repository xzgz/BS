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

#include <string>
#include "svregmanifest.h"
#include <fstream>
#include <DS/timer.h>
#include <sstream>
#include <strutil.h>
#include <DS/getfilesize.h>
#include <vector>
#include <iostream>

SVRegManifest::SVRegManifest() : buildNumber(0), valid(false)
{
}

bool SVRegManifest::read(std::string ifname)
{
  atlases.clear();
  int filesize = getFileSize(ifname);
  if (filesize<=0) return false;
  std::ifstream ifile(ifname.c_str(),std::ios::binary);
  if (!ifile) return false;
  std::vector<char> buffer(filesize+1);
  ifile.read(&buffer[0],filesize);
  buffer[filesize] = 0;
  try
  {
    rapidxml::xml_document<> doc;    // character type defaults to char
    doc.parse<rapidxml::parse_declaration_node | rapidxml::parse_no_data_nodes>(&buffer[0]);    // 0 means default parse flags
    rapidxml::xml_node<>* cur_node = doc.first_node("svregmanifest");
    if (cur_node)
    {
      valid=true;
      filepath=ifname;
      versionstr=readNode(cur_node,"version");
      buildstr=readNode(cur_node,"build");
      std::istringstream(buildstr)>>buildNumber;
      date=readNode(cur_node,"date");
      mcrversion=readNode(cur_node,"mcrversion");
      platform=readNode(cur_node,"platform");
      minimumBrainSuiteVersion=readNode(cur_node,"minimumbrainsuiteversion");
      rapidxml::xml_node<>* atlasNode = cur_node->first_node("atlas");
      if (atlasNode)
      {
        while (atlasNode)
        {
          Atlas atlas;
          atlas.name = atlasNode->value();
          rapidxml::xml_attribute<char> *nameNode = atlasNode->first_attribute("basename");
          if (nameNode)
            atlas.basename= nameNode->value();
          else
            atlas.basename = "mri";
          atlasNode = atlasNode->next_sibling("atlas");
          atlases.push_back(atlas);
        }
      }
    }
    else
      valid=false;
  }
  catch(rapidxml::parse_error &error)
  {
    std::cerr<<"RapidXML exception when parsing "<<ifname<<" : "<<error.what()<<std::endl;
    return false;
  }
  return valid;
}
