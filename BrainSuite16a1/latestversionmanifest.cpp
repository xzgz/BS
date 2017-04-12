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

#include "latestversionmanifest.h"
#include <sstream>
#include <iostream>

LatestVersionManifest::LatestVersionManifest() : valid(false)
{
}

bool LatestVersionManifest::scanVersionInfo(VersionInfo &versionInfo, rapidxml::xml_node<char> *node, std::string key)
{
  rapidxml::xml_node<>* sw_node = node->first_node(key.c_str());
  if (sw_node)
  {
    rapidxml::xml_attribute<char> *build = sw_node->first_attribute("build");
    if (build)
    {
      std::string s=build->value();
      std::istringstream(s)>>versionInfo.build;
    }
    rapidxml::xml_attribute<char> *vers = sw_node->first_attribute("version");
    if (vers)
    {
      versionInfo.version=vers->value();
    }
    rapidxml::xml_attribute<char> *date = sw_node->first_attribute("releasedate");
    if (date)
    {
      std::istringstream istr(date->value());
      int year,month,day; char ch;
      istr>>year>>ch>>month>>ch>>day;
      versionInfo.releaseDate.setDate(year,month,day);
    }
    return true;
  }
  return false;
}

bool LatestVersionManifest::parse(char *data)
{
  using namespace rapidxml;
  try {
    xml_document<char> doc;    // character type defaults to char
    doc.parse<parse_declaration_node | parse_no_data_nodes>(data);    // 0 means default parse flags
    xml_node<>* cur_node = doc.first_node("releaseinfo");
    if (cur_node)
    {
      scanVersionInfo(brainSuiteVersion,cur_node,"brainsuite");
      scanVersionInfo(svregVersion,cur_node,"svreg");
      scanVersionInfo(bdpVersion,cur_node,"bdp");
      valid=true;
    }
    else
    {
      valid=false;
    }
  }
  catch(rapidxml::parse_error &error)
  {
    std::cerr<<"RapidXML exception reading manifest : "<<error.what()<<std::endl;
    return false;
  }
  return valid;
}
