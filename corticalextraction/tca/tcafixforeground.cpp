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

#include <iostream>
#include <TCA/tcafixforeground.h>
#include <TCA/tcafixchecker.h>
#include <TCA/slicesegmenter.h>
#include <TCA/sliceops.h>
#include <DS/timer.h>

TCAFixForeground::TCAFixForeground() :
  smallest(0), largestFGChange(0),
  verbose(false),
  currentEdge(-1), thresh_(2)
{
}

void TCAFixForeground::findAndRemove(sint16 *s, sint16 *slice1, sint16 *slice2, const int cx, const int cy)
{
  seeker.findAndRemove(s,slice1,slice2,cx,cy);
  return;
}

Vol3D<uint8> TCAFixForeground::vtest;

TCAFixForeground::~TCAFixForeground()
{
}

void TCAFixForeground::labelSlices(Vol3D<uint8> &vin)
{
  const int slicesize = vin.cx * vin.cy;
  SliceSegmenter sseg(vin.cx,vin.cy);
  foregroundLabels.setsize(vin.cx,vin.cy,vin.cz);
  foregroundLabels.rx = vin.rx;
  foregroundLabels.ry = vin.ry;
  foregroundLabels.rz = vin.rz;
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
  int totalSym = 0;
  for (int z=1;z<lastSlice;z++)
  {
    slice = foregroundLabels.slice(z);
    sseg.segment(d,slice,0,255);
    nsym[z] = sseg.nSymbols()-1;
    totalSym += nsym[z];
    d += slicesize;
  }
  nsym[lastSlice] = 0;
  slice = foregroundLabels.slice(lastSlice); // last slice
  for (int i=0;i<slicesize;i++) slice[i] = 0;
}

int TCAFixForeground::analyze(Vol3D<uint8> &vin)
{
  currentEdge=-1;
  edgeTable.clear();
  labelSlices(vin);
  SliceOps::andAdjacentSlices(intersect,vin);
  labels.setsize(vin.cx,vin.cy,vin.cz);
  labels.rx = vin.rx;
  labels.ry = vin.ry;
  labels.rz = vin.rz;
  labels.set(0);
  const int slicesize = vin.cx * vin.cy;
  const int cz = vin.cz;
  const int nz = cz-1;
  SliceSegmenter sliceSegmenter(vin.cx,vin.cy);
  sliceSegmenter.mode = SliceSegmenter::D4;
  int id = 0;
  for (int r=0;r<cz;r++)
  {
    offset[r] = id;
    id += nsym[r];
  }
  for (int i=0;i<nz;i++)
  {
    sint16 *top = foregroundLabels.slice(i);
    sint16 *bot = foregroundLabels.slice(i+1);
    uint8 *C = intersect.start() + i*slicesize;
    sliceSegmenter.mode = SliceSegmenter::D4;
    sint16 *ilabel = labels.slice(i);
    sliceSegmenter.segment(C,ilabel,0,255);
    findAndRemove(ilabel,top,bot,foregroundLabels.cx,foregroundLabels.cy);
    seeker.relabelChains(ilabel,slicesize);
    int ns = sliceSegmenter.nSymbols();
    for (int n=0;n<ns;n++)
    {
      SliceRegionInfo info;
      SliceOps::checkInfo(ilabel,n+1,info,slicesize);
      if (info.count>0)
      {
        Link link;
        link.a = SliceLabel(top[info.idx]+offset[i],i);
        link.b = SliceLabel(bot[info.idx]+offset[i+1],i+1);
        link.a.trueID = top[info.idx];
        link.b.trueID = bot[info.idx];
        link.weight = (float)info.count;
        link.index = info.idx;
        link.id = ilabel[info.idx];
        link.tag = -1;
        edgeTable.add(link);
      }
    }
  }
  Timer t1;
  t1.start();
  return edgeTable.genus();
}

int TCAFixForeground::findNextFix(const int threshold)
{
  thresh_ = threshold;
  if (currentEdge<0)
  {
    mst.create(edgeTable);
    const int ne = mst.dead.nedges;
    if (verbose) std::cout<<" removing "<<ne<<" edges.\n";
    currentEdge=0;
    if (ne>0) smallest = (int)mst.dead.edge[0].weight;
    else smallest=0;
  }
  const int ne = mst.dead.nedges;
  while (currentEdge<ne)
  {
    Link l = mst.dead.edge[currentEdge];
    if (l.weight>thresh_)
    {
      if (l.weight<smallest) smallest = (int)l.weight;
      currentEdge++;
      continue;
    }
    break;
  }
  if (currentEdge>=ne) currentEdge = -1;
  return currentEdge;
}

int TCAFixForeground::fixNext(Vol3D<uint8> &vin)
{
  sliceBuffer.create(vin.cx,vin.cy);
  Link l = mst.dead.edge[currentEdge];
  int na = mst.tree.count(l.a);
  int nb = mst.tree.count(l.b);
  if ((na>=2)&&(nb>=2)) { currentEdge++; return 0; }
  bool up = (na==1);
  if (verbose) { if ((na==0)&&(nb==0)) std::cout<<"dead link!\n"; }
  const int z = (up) ? l.a.slice : l.b.slice;
  if (verbose) { std::cout<<"Fixing slice "<<z<<"\t"<<((up) ? "up" : "down")<<((up) ? na : nb)<<"\n"; }
  SliceOps::select(labels.slice(l.a.slice),sliceBuffer.data,l.id,sliceBuffer.slicesize);
  if (l.weight>largestFGChange) largestFGChange=(int)l.weight;
  if (verbose)
  {
    int c1 = SliceOps::count(sliceBuffer);
    std::cout<<"mask contains "<<c1<<" voxels\n";
  }
  SliceOps::subSliceX(vin.slice(z),sliceBuffer.data,sliceBuffer.slicesize);
  currentEdge++;
  return 1;
}

void instrument(Vol3D<uint8> &mask);

int TCAFixForeground::fixNextG(Vol3D<uint8> &vIn, const int thresh)
{	
  thresh_ = thresh;
  s1.create(vIn.cx,vIn.cy);
  s2.create(vIn.cx,vIn.cy);
  s3.create(vIn.cx,vIn.cy);
  const int slicesize = vIn.cx * vIn.cy;
  int fixcount = 0;
  mst.create(edgeTable);
  if (verbose) std::cout<<"There are "<<mst.dead.nedges<<" dead edges.\n";
  for (int i=mst.dead.nedges-1;i>=0;i--)
  {
    Link l = mst.dead.edge[i];
    if (l.weight>thresh_) continue;
    int na = mst.tree.count(l.a);
    int nb = mst.tree.count(l.b);
    int z = l.a.slice;
    SliceOps::select(labels.slice(z),s1.data,l.id,slicesize);
    if (SliceOps::isSubset(s1.data,vIn.slice(z-1),slicesize))
    {
      SliceOps::subSlice(vIn.slice(z),s1.data,slicesize);
      if (verbose) std::cout<<"Cutting A."<<l.weight<<"\t"<<na<<"\t"<<nb<<"\n";
      fixcount++;
      if (l.weight>largestFGChange) largestFGChange=(int)l.weight;
    }
    else // check down
    {
      z = l.b.slice;
      if (SliceOps::isSubset(s1.data,vIn.slice(z+1),slicesize))
      {
        SliceOps::subSlice(vIn.slice(z),s1.data,slicesize);
        if (verbose) std::cout<<"Cutting B."<<l.weight<<"\t"<<na<<"\t"<<nb<<"\n";
        fixcount++;
        if (l.weight>largestFGChange) largestFGChange=(int)l.weight;
      }
    }
  }
  return fixcount;
}


bool TCAFixForeground::tryFixes(Vol3D<uint8> &vin, const int thresh) // try to fix it, test to see if fix introduces problems
{
  vtest.copy(vin);
  while (findNextFix(thresh)>=0)
  {
    sliceBuffer.create(vin.cx,vin.cy);
    Link l = mst.dead.edge[currentEdge];
    int na = mst.tree.count(l.a);
    int nb = mst.tree.count(l.b);
    bool up = (na<nb); // try the smaller of the two
    bool okflag = false;
    {
      if (na==0) up = false;
      if (nb==0) up = true;
      if (verbose) { if ((na==0)&&(nb==0)) std::cerr<<"dead link!\n"; }
      const int z = (up) ? l.a.slice : l.b.slice;
      if (verbose) { std::cout<<"Fixing slice "<<z<<"\t"<<((up) ? "up" : "down")<<((up) ? na : nb)<<"\n"; }
      SliceOps::select(labels.slice(l.a.slice),sliceBuffer.data,l.id,sliceBuffer.slicesize);
      if (l.weight>largestFGChange) largestFGChange=(int)l.weight;
      if (verbose)
      {
        int c1 = SliceOps::count(sliceBuffer);
        std::cout<<"mask contains "<<c1<<" voxels\n";
      }
      SliceOps::subSliceX(vtest.slice(z),sliceBuffer.data,sliceBuffer.slicesize);	// clear it in the slice
      if (TCAFixChecker::checkMaskFG(vtest,z)==false)
      {
        if ((na>0)&&(nb>0))
        {
          if (verbose) { if ((na==0)&&(nb==0)) std::cout<<"dead link!\n"; }
          const int z = (up) ? l.a.slice : l.b.slice;
          if (verbose) { std::cout<<"Fixing slice "<<z<<"\t"<<((up) ? "up" : "down")<<((up) ? na : nb)<<"\n"; }
          SliceOps::select(labels.slice(l.a.slice),sliceBuffer.data,l.id,sliceBuffer.slicesize);
          if (l.weight>largestFGChange) largestFGChange=(int)l.weight;
          if (verbose)
          {
            int c1 = SliceOps::count(sliceBuffer);
            std::cout<<"mask contains "<<c1<<" voxels\n";
          }
          SliceOps::subSliceX(vtest.slice(z),sliceBuffer.data,sliceBuffer.slicesize);	// clear it in the slice
          if (TCAFixChecker::checkMaskFG(vtest,z)==true) okflag = true;
        }
      }
      else
      {
        okflag = true;
      }
    }
    if (okflag)
    {
      vin.copy(vtest);
    }
    else
      vtest.copy(vin);
    currentEdge++;
  }
  return true;
}
