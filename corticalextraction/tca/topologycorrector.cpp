// Copyright (C) 2016 The Regents of the University of California and
// the University of Southern California
//
// Created by David W. Shattuck, Ph.D.
//
// This file is part of TCA.
//
// TCA is free software; you can redistribute it and/or
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

#include <TCA/topologycorrector.h>
#include <DS/runlengthsegmenter.h>
#include <iostream>
#include <iomanip>

void transposeXZ(const Vol3D<uint8> &va, Vol3D<uint8> &vb)
{
  const int cx = va.cx;
  const int cy = va.cy;
  const int cz = va.cz;
  vb.setsize(cz,cy,cx);
  for (int z=0; z<cz; z++)
    for (int y=0; y<cy; y++)
    {
      for (int x=0; x<cx; x++)
      {
        vb(z,y,x) = va(x,y,z);
      }
    }
}

void transposeYZ(const Vol3D<uint8> &va, Vol3D<uint8> &vb)
{
  const int cx = va.cx;
  const int cy = va.cy;
  const int cz = va.cz;
  vb.setsize(cx,cz,cy);
  for (int z=0; z<cz; z++)
    for (int y=0; y<cy; y++)
      for (int x=0; x<cx; x++)
        vb(x,z,y) = va(x,y,z);
}

TopologyCorrector::TopologyCorrector() : list(false), bgOnly(false), topomode(0), verbosity(0)
{
}

void TopologyCorrector::selectCC(Vol3D<uint8> &vMask)
{
  RunLengthSegmenter segmentor;
  Vol3D<VBit> vBitMask;
  vBitMask.encode(vMask);
  segmentor.ensureCentered = false;
  segmentor.mode=RunLengthSegmenter::D6;
  segmentor.segmentFG(vBitMask);
  segmentor.mode=RunLengthSegmenter::D18;
  segmentor.segmentBG(vBitMask);
  vBitMask.decode(vMask);
}

int TopologyCorrector::analyzeAndFix(Vol3D<uint8> &mask, int thresh)
{
  int count = 0;
  int nfg = fg.analyze(mask);
  if (verbosity>0)
  {
    std::cout<<"("<<std::setw(4)<<nfg;
    if (nfg==1) std::cout<<" cycle ) ";
    else std::cout<<" cycles) ";
    if ((nfg>0)&&(verbosity>1))
    {
      MaximalSpanningTree<Link> mst;
      mst.create(fg.edgeTable);
      const int ndisplay=(mst.dead.nedges>20) ? 20 : mst.dead.nedges;
      if (ndisplay>1)
        std::cout<<" weights :";
      else
        std::cout<<" weight  :";
      for (int i=0;i<ndisplay;i++)
        std::cout<<' '<<mst.dead.edge[i].weight;
      if (ndisplay<mst.dead.nedges)
        std::cout<<" ...";
    }
  }
  if (nfg!=0)
  {
    if (!bgOnly)
    {
      while (fg.findNextFix(thresh)>=0)
      {
        int n = fg.fixNext(mask);
        count += n;
      }
      nfg = fg.analyze(mask);
    }
    fg.fixNextG(mask,thresh);
    selectCC(mask);
    fg.tryFixes(mask,thresh);
    selectCC(mask);
  }
  return count;
}

inline int TopologyCorrector::nextSmallest(EdgeTable<Link> &et, int thresh)
{
  const int n = et.nedges;
  int min=-1;
  for (int i=0;i<n;i++)
  {
    Link l = et.edge[i];
    if (l.weight>thresh)
    {
      if (min<0)
        min = (int)l.weight;
      else
        if (l.weight<min) min = (int)l.weight;
    }
  }
  return min;
}

int TopologyCorrector::checkTopology(Vol3D<uint8> &vMask)
{
  int nfg = fg.analyze(vMask);
  if (verbosity>0)
  {
    std::cout<<"("<<std::setw(4)<<nfg;
    if (nfg==1) std::cout<<" cycle ) ";
    else std::cout<<" cycles) ";
    if ((nfg>0)&&(verbosity>1))
    {
      MaximalSpanningTree<Link> mst;
      mst.create(fg.edgeTable);
      const int ndisplay=(mst.dead.nedges>20) ? 20 : mst.dead.nedges;
      if (ndisplay>1)
        std::cout<<" weights :";
      else
        std::cout<<" weight  :";
      for (int i=0;i<ndisplay;i++)
        std::cout<<' '<<mst.dead.edge[i].weight;
      if (ndisplay<mst.dead.nedges)
        std::cout<<" ...";
    }
  }
  return nfg;
}

int TopologyCorrector::checkTopology3Axes(Vol3D<uint8> &vMask)
{
  if (verbosity>1) std::cout<<'\n';
  if (verbosity>0) std::cout<<"Z axis ";
  int nZ = checkTopology(vMask);
  transposeYZ(vMask, tr);
  if (verbosity>1) std::cout<<'\n';
  if (verbosity>0) std::cout<<"Y axis ";
  int nY = checkTopology(tr);
  transposeXZ(vMask, tr);
  if (verbosity>1) std::cout<<'\n';
  if (verbosity>0) std::cout<<"X axis ";
  int nX = checkTopology(tr);
  return nX + nY + nZ;
}

int TopologyCorrector::loopFG(Vol3D<uint8> &vm, const int fixthreshold, const int fgDelta)
{
  selectCC(vm);
  int nextSize = 0;
  fg.largestFGChange = 0;
  int total=1;
  int largestFGChange = 0;
  if (topomode!=2)
  {
    int fgFixthreshold = fixthreshold - fgDelta;
    if (fgFixthreshold>0)
    {
      int nsm=0;
      fg.smallest = 0;
      fg.largestFGChange = 0;
      if (verbosity>1) std::cout<<'\n';
      if (verbosity>0) std::cout<<"Z axis ";
      total  = analyzeAndFix(vm,fgFixthreshold);
      largestFGChange = fg.largestFGChange; // may not be necessary
      nsm = nextSmallest(fg.mst.dead,fgFixthreshold);
      if ((nsm>fgFixthreshold)&&(nsm<nextSize)) nextSize = nsm;
      transposeYZ(vm, tr);
      if (verbosity>1) std::cout<<'\n';
      if (verbosity>0) std::cout<<"Y axis ";
      total += analyzeAndFix(tr,fgFixthreshold);
      if (largestFGChange < fg.largestFGChange) largestFGChange = fg.largestFGChange;
      nsm = nextSmallest(fg.mst.dead,fgFixthreshold);
      if ((nsm>fgFixthreshold)&&(nsm<nextSize)) nextSize = nsm;
      transposeYZ(tr, vm);
      transposeXZ(vm, tr);
      if (verbosity>1) std::cout<<'\n';
      if (verbosity>0) std::cout<<"X axis ";
      total += analyzeAndFix(tr,fgFixthreshold);
      if (largestFGChange < fg.largestFGChange) largestFGChange = fg.largestFGChange;
      nsm = nextSmallest(fg.mst.dead,fgFixthreshold);
      if (nsm>fgFixthreshold)
      {
        if ((nextSize<=0)||(nsm<nextSize))
          nextSize = nsm;
      }
      transposeXZ(tr, vm);
      if (verbosity>1) std::cout<<'\n';
    }
  }
  return nextSize;
}
