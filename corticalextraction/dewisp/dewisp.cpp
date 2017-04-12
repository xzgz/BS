// Copyright (C) 2016 The Regents of the University of California
//
// Created by David W. Shattuck, Ph.D.
//
// This file is part of Dewisp.
//
// Dewisp is free software; you can redistribute it and/or
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

#include <dewisp.h>
#include <TCA/slicesegmenter.h>
#include "TCA/slicetemplates.h"
#include "TCA/sliceops.h"
#include "TCA/seeker.h"
#include <DS/timer.h>
#include <DS/runlengthsegmenter.h>
#include <list>

void Dewisp::labelSlices(Vol3D<uint8> &vin)
{
  const int slicesize = vin.cx * vin.cy;
  SliceSegmenter sseg(vin.cx,vin.cy);
  foregroundLabels.makeCompatible(vin);
  sseg.mode = SliceSegmenter::D4;
  uint8  *d = vin.start() + slicesize; // start labeling on the second slice
  const int maxdim = std::max(std::max(vin.cx,vin.cy),vin.cz);
  nsym.resize(maxdim);
  nsym.resize(foregroundLabels.cz);
  offset.resize(maxdim);
  offset.resize(foregroundLabels.cz);
  nsym[0] = 0;
  short *slice = foregroundLabels.slice(0);
  for (int i=0;i<slicesize;i++) slice[i] = 0;
  const int lastSlice = vin.cz - 1;
  totalSym = 0;

  for (int z=1;z<lastSlice;z++)
  {
    slice = foregroundLabels.slice(z);
    sseg.segment(d,slice,0,255);
    nsym[z] = sseg.nSymbols()-1;
    totalSym += nsym[z];
    d += slicesize;
  }
  nsym[lastSlice] = 0;
  slice = foregroundLabels.slice(lastSlice);
  for (int i=0;i<slicesize;i++) slice[i] = 0;
  vUnique.makeCompatible(foregroundLabels);
  vUnique.set(0);
  const int ss=vUnique.cx*vUnique.cy;
  const int cz=vUnique.cz;
  labelCount.resize(totalSym+1);
  for (size_t i=0;i<labelCount.size();i++) labelCount[i] = 0;
  for (int z=0,id=0;z<cz;z++)
  {
    offset[z] = id;
    sint16 *s = foregroundLabels.slice(z);
    uint16 *d = vUnique.slice(z);
    for (int i=0;i<ss;i++)
      if (s[i])
      {
        d[i] = s[i] + id;
        labelCount[d[i]]++;
      }
    id += nsym[z];
  }
}

int Dewisp::analyze(Vol3D<uint8> &vin)
{
  edgeTable.clear();
  labelSlices(vin);
  SliceOps::andAdjacentSlices(intersect,vin); // note order reversed now
  labels.setsize(vin.cx,vin.cy,vin.cz);
  labels.rx = vin.rx;
  labels.ry = vin.ry;
  labels.rz = vin.rz;
  labels.set(0);
  const int slicesize = vin.cx * vin.cy;
  const int cz = vin.cz;
  const int nz = cz-1;
  SliceSegmenter sseg(vin.cx,vin.cy);
  sseg.mode = SliceSegmenter::D4;
  Seeker seeker;
  for (int i=0;i<nz;i++)
  {
    sint16 *top = foregroundLabels.slice(i);
    sint16 *bot = foregroundLabels.slice(i+1);
    uint8 *C = intersect.start() + i*slicesize;
    sseg.mode = SliceSegmenter::D4;
    sint16 *ilabel = labels.slice(i);
    sseg.segment(C,ilabel,0,255);
    seeker.findAndRemove(ilabel,top,bot,foregroundLabels.cx,foregroundLabels.cy);
    seeker.relabelChains(ilabel,slicesize);
    int ns = sseg.nSymbols();
    for (int n=0;n<ns;n++)
    {
      SliceRegionInfo info;
      SliceOps::checkInfo(ilabel,n+1,info,slicesize);
      if (info.count>0)
      {
        Link link;
        int a = vUnique.slice(i  )[info.idx];
        int b = vUnique.slice(i+1)[info.idx];
        link.a = SliceLabel(a,i);
        link.b = SliceLabel(b,i+1);
        link.a.trueID = vUnique.slice(i  )[info.idx];
        link.b.trueID = vUnique.slice(i+1)[info.idx];
        link.weight = (float)info.count;
        link.index = info.idx;
        link.id = ilabel[info.idx];
        link.tag = -1;
        edgeTable.add(link);
      }
    }
  }
  return edgeTable.genus();
}

bool Dewisp::process(Vol3D<uint8> &volume)
{
  analyze(volume);
  const int nSymbols = totalSym+1;
  Vol3D<short> vLabel;
  vLabel.copy(labels);
  const int cx = vLabel.cx;
  const int cy = vLabel.cy;
  const int cz = vLabel.cz;
  const int ss = cx * cy;
  for (int z=0;z<cz;z++)
  {
    short *slice = vLabel.slice(z);
    const int ofs = offset[z];
    for (int i=0;i<ss;i++) if (slice[i]>0) slice[i] += ofs;
  }
  typedef std::pair<int,Link> AdjEdge;
  std::vector<std::list<AdjEdge> > adjacencyList(nSymbols);
  for (int i=0;i<edgeTable.nedges;i++)
  {
    Link edge = edgeTable.edge[i];
    adjacencyList[edge.a.id].push_back(AdjEdge(edge.b.id,edge));
    adjacencyList[edge.b.id].push_back(AdjEdge(edge.a.id,edge));
  }
  std::vector<uint8> keepTable(nSymbols,255);
  for (int i=0;i<nSymbols;i++)
  {
    if (adjacencyList[i].size()==1)
    {
      int blobID = i;
      int prevID = blobID;
      if (labelCount[blobID]<sizeThreshold) keepTable[blobID]=0;
      blobID = adjacencyList[blobID].front().first;
      for (;;)
      {
        if (adjacencyList[blobID].size()!=2)
        {
          break;
        }
        if (labelCount[blobID]<sizeThreshold) keepTable[blobID]=0;
        int leftID = adjacencyList[blobID].front().first;
        int rightID = adjacencyList[blobID].back().first;
        int nextID=-1;
        if (leftID==prevID) nextID=rightID;
        else if (rightID==prevID) nextID=leftID;
        if (nextID<-1) {
          break;
        }
        prevID = blobID;
        blobID = nextID;
      }
    }
  }
  const int ds = volume.size();
  for (int i=0;i<ds;i++)
  {
    if (volume[i])
      volume[i] = keepTable[vUnique[i]];
  }
  return true;
}
