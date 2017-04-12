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

#include <connectivitymap.h>
#include <sstream>
#include <fstream>

void tokenize(std::vector<std::string> &tokens, std::istream &istr)
{
  std::string line;
  std::getline(istr,line);
  std::stringstream linestream(line);
  std::string cell;
  while(std::getline(linestream,cell,'\t'))
  {
    tokens.push_back(cell);
  }
}

ConnectivityMap::ConnectivityMap() : maxValue(1)
{
}

void ConnectivityMap::computeMaxValue()
{
  maxValue=1;
  const int nn = size();
  for (int i=1;i<nn;i++)
  {
    for (int j=i+1;j<nn;j++)
      if (adjacencyMatrix(i,j)>maxValue)
        maxValue=adjacencyMatrix(i,j);
  }
}

bool ConnectivityMap::readTSV(std::string ifname)
{
  std::ifstream ifile(ifname.c_str());
  if (!ifile) return false;
  std::vector<std::string> tokens;
  tokenize(tokens,ifile);
  nodeNames.resize(tokens.size()-1);
  for (size_t i=1;i<tokens.size();i++)
  {
    nodeNames[i-1] = tokens[i];
  }
  const int nn = size();
  adjacencyMatrix.setsize(nn,nn);
  for (int i=0;i<nn;i++)
  {
    std::vector<std::string> tokens;
    tokenize(tokens,ifile);
    for (int j=0;j<nn;j++)
      adjacencyMatrix(i,j) = (float)atof(tokens[j+1].c_str());
  }
  computeMaxValue();
  return true;
}
