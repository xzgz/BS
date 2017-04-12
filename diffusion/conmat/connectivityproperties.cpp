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

#include "connectivityproperties.h"
#include <connectivitymap.h>
#include <DS/timer.h>

void ConnectivityProperties::computeLabelCentroids(Vol3D<uint16> &vLabel)
{
  Timer t;
  t.start();
  count.resize(65536); std::fill(count.begin(),count.end(),0);
  const int cx=vLabel.cx;
  const int cy=vLabel.cy;
  const int cz=vLabel.cz;

  std::vector<IPoint3D> roiVoxels(65536);
  for (int z=0;z<cz;z++)
  {
    for (int y=0;y<cy;y++)
    {
      for (int x=0;x<cx;x++)
      {
        int label=vLabel(x,y,z);
        roiVoxels[label].x+=x;
        roiVoxels[label].y+=y;
        roiVoxels[label].z+=z;
        count[label]++;
      }
    }
  }
  labelCentroids.resize(65536); std::fill(labelCentroids.begin(),labelCentroids.end(),DSPoint(0,0,0));
  for (int i=0;i<65536;i++)
  {
    if (count[i])
      labelCentroids[i] = DSPoint(roiVoxels[i].x * vLabel.rx/count[i],
                                  roiVoxels[i].y * vLabel.ry/count[i],
                                  roiVoxels[i].z * vLabel.rz/count[i]);
  }
  t.stop();
}


void ConnectivityProperties::sumWeights(ConnectivityMap &connectivityMap, Vol3D<uint16> &/*vLabel*/)
{
  int cx = connectivityMap.adjacencyMatrix.cx;
  int cy = connectivityMap.adjacencyMatrix.cy;
  nodeWeights.resize(65536);
  std::fill(nodeWeights.begin(),nodeWeights.end(),0.0f);
  maxWeight=0;
  for (int x=1;x<cx;x++)
  {
    int labelID=((int)connectivityMap.labelIDs.size()>=x) ? connectivityMap.labelIDs[x] : 0;
    float nTracks=0;
    for (int y=1;y<cy;y++)
    {
      if (x==y) continue;
      nTracks += connectivityMap.adjacencyMatrix(x,y);
    }
    nodeWeights[labelID] = nTracks;
    maxWeight += nTracks;
  }
  maxWeight/=2.0f;
}
