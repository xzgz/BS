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

#include <TCA/tcafixbackground.h>
#include <TCA/slicesegmenter.h>
#include <TCA/slicetemplates.h>
#include <DS/timer.h>
#include <TCA/sliceops.h>
#include <TCA/maximalspanningtree.h>
#include <TCA/linkbg.h>
#include <iomanip>
#include <DS/runlengthsegmenter.h>

TCAFixBackground::TCAFixBackground() : nTotalRegions(0), verbosity(0)
{
}

sint16 TCAFixBackground::uniqueLabels(const Vol3D<sint16> &labels)
// unique labels converts a set of slice labels, where each one goes from 1 to Nz,
// into a set of unique labels for the whole volume, going from (1 to N0)(N0+1 to N0+N1) etc.
{
  const size_t slicesize = labels.cx * labels.cy;
  sint16 delta = 0;
  sint16 next = 0;
  for (size_t z=0;z<labels.cz;z++)
  {
    sint16 *label = labels.slice(z);
    for (size_t i=0;i<slicesize;i++)
    {
      if (label[i]>0)
      {
        label[i] += delta;
        if (label[i]>next) next = label[i];
      }
    }
    delta = next;
  }
  return next;
}

int TCAFixBackground::labelSliceSegments(const Vol3D<uint8> &vMaskIn)
{
  vSliceSegments.makeCompatible(vMaskIn);
  SliceSegmenter sliceSegmentor(vMaskIn.cx,vMaskIn.cy);
  sliceSegmentor.mode = SliceSegmenter::D8;
  vSliceSegments.makeCompatible(vMaskIn);
  nSliceSymbols.reserve(std::max(std::max(vMaskIn.cx,vMaskIn.cy),vMaskIn.cz));
  nSliceSymbols.resize(vMaskIn.cz);
  const int cz = vSliceSegments.cz;
  for (int z=0;z<cz;z++)
  {
    sliceSegmentor.segment(vMaskIn.slice(z),vSliceSegments.slice(z),0,0);
    int ns = sliceSegmentor.nSymbols()-1;
    nSliceSymbols[z] = ns;
  }
  nSliceSymbols[cz-1] = 0;
  vSliceLabels.copy(vSliceSegments);
  return uniqueLabels(vSliceLabels);
}

void TCAFixBackground::ensureCC(Vol3D<uint8> &vMask)
{
  static RunLengthSegmenter segmentor;
  static Vol3D<VBit> vBitMask;
  vBitMask.encode(vMask);
  segmentor.ensureCentered = false;
  segmentor.mode=RunLengthSegmenter::D6;
  segmentor.segmentFG(vBitMask);
  segmentor.mode=RunLengthSegmenter::D18;
  segmentor.segmentBG(vBitMask);
  vBitMask.decode(vMask);
}

void transposeXZ(const Vol3D<uint8> &va, Vol3D<uint8> &vb);
void transposeYZ(const Vol3D<uint8> &va, Vol3D<uint8> &vb);

int TCAFixBackground::checkTopology3Axes(const Vol3D<uint8> &vMaskIn)
{
  Vol3D<uint8> vTmp;
  Vol3D<VBit> vBit;
  if (verbosity>1) std::cout<<'\n';
  if (verbosity>0) std::cout<<"Z axis ";
  int nz = checkTopology(vMaskIn);
  if (verbosity>0) std::cout<<"Y axis ";
  ::transposeYZ(vMaskIn,vTmp);
  int ny = checkTopology(vTmp);
  ::transposeXZ(vMaskIn,vTmp);
  if (verbosity>0) std::cout<<"X axis ";
  int nx = checkTopology(vTmp);
  return nx + ny + nz;
}

int TCAFixBackground::correctTopology3Axes(Vol3D<uint8> &vMaskIn, const int maxCorrectionSize)
{
  Vol3D<uint8> vTmp;
  Vol3D<VBit> vBit;
  int mn = -1;
  if (verbosity>1) std::cout<<'\n';
  if (verbosity>0) std::cout<<"Z axis ";
  int n1 = correctTopology(vMaskIn,maxCorrectionSize);
  ::transposeYZ(vMaskIn,vTmp);
  ensureCC(vMaskIn);
  if (verbosity>0) std::cout<<"Y axis ";
  int n2 = correctTopology(vTmp,maxCorrectionSize);
  ::transposeYZ(vTmp,vMaskIn);
  ensureCC(vMaskIn);
  ::transposeXZ(vMaskIn,vTmp);
  if (verbosity>0) std::cout<<"X axis ";
  int n3 = correctTopology(vTmp,maxCorrectionSize);
  ::transposeXZ(vTmp,vMaskIn);
  ensureCC(vMaskIn);
  if ((n1>0)&&((mn<0)||(n1 < mn))) { mn = n1; }
  if ((n1>0)&&((mn<0)||(n2 < mn))) { mn = n2; }
  if ((n1>0)&&((mn<0)||(n3 < mn))) { mn = n3; }
  return mn;
}

int TCAFixBackground::correctTopology(Vol3D<uint8> &vMaskIn, const int maxCorrectionSize)
{
  const int cx = vMaskIn.cx;
  const int cy = vMaskIn.cy;
  const int cz = vMaskIn.cz;
  nTotalRegions = labelSliceSegments(vMaskIn);
  Vol3D<uint8> vMap;
  vMap.makeCompatible(vMaskIn);
  vMap.set(0);
  SliceT<uint8> sliceCurrent(cx,cy), sliceDilated(cx,cy);
  SliceT<uint8> regionNextSlice(cx,cy),regionNextSliceMaskedDilated(cx,cy);
  SliceT<sint16> sliceLabeled(cx,cy);
  Vol3D<sint16> vCorrectionMap;
  vCorrectionMap.setsize(cx,cy,nTotalRegions);
  vCorrectionMap.set(0);
  SliceSegmenter sliceSegmentor(cx,cy);
  sliceSegmentor.mode = SliceSegmenter::D8;
  const int zStop = cz - 1;
  int correctionSlice=0;
  EdgeTable<LinkBG> edgeTable;
  for (int z=0; z<zStop; z++)
  {
    for (int i=1;i<=nSliceSymbols[z];i++)
    {// sliceCurrent is slice z, but only one of several segments.
      SliceOps::select(sliceCurrent,vSliceSegments.slice(z),i);
      int firstpos = SliceOps::findFirst(sliceCurrent);
      if (firstpos<0) continue;
      const int labelA = vSliceLabels.slice(z)[firstpos];
      SliceOps::dilate4(sliceDilated,sliceCurrent);
      SliceOps::intersect(sliceDilated,vSliceSegments.slice(z+1));
      sliceSegmentor.segment(sliceDilated.data,sliceLabeled.data,0,255);
      SliceOps::copy<sint16>(vCorrectionMap.slice(correctionSlice),sliceLabeled); // these are the "B" corrections
      const int ns = sliceSegmentor.nSymbols();
      for (int j=1;j<ns;j++)
      {
        SliceOps::select(regionNextSlice,sliceLabeled.data,j); // this is the correction for B!
        const int firstpos = SliceOps::findFirst(regionNextSlice);
        if (firstpos<0) { std::cerr<<"warning: empty region"<<std::endl; continue; }
        const int labelB = vSliceLabels.slice(z+1)[firstpos];
        SliceOps::dilate4(regionNextSliceMaskedDilated,regionNextSlice);
        SliceOps::intersect(regionNextSliceMaskedDilated,sliceCurrent); // correction for A!
        int c = SliceOps::count(regionNextSliceMaskedDilated);
        LinkBG edge(labelA,labelB); // need a way of storing each correction!!!!
        edge.weightA = (float)c;
        edge.weightB = (float)sliceSegmentor.getInfo(j).count;
        edge.weight = std::min(edge.weightA,edge.weightB);
        edge.correctionIndex.slice = correctionSlice;
        edge.correctionIndex.label = j;
        edge.correctionIndex.sliceLabelA = i;
        edge.a.slice = z;
        edge.b.slice = z+1;
        edgeTable.add(edge);
      }
      correctionSlice++;
    }
  }
  MaximalSpanningTree<LinkBG> mst;
  mst.create(edgeTable);
  int g = edgeTable.genus();
  if (verbosity>0)
  {
    std::cout<<"("<<std::setw(4)<<g;
    if (g==1) std::cout<<" cycle ) ";
    else std::cout<<" cycles) ";
  }
  if (verbosity>1)
  {
    if (g>0)
    {
      const size_t ndisplay=(mst.dead.nedges>20) ? 20 : mst.dead.nedges;
      if (ndisplay>1)
        std::cout<<" weights :";
      else
        std::cout<<" weight  :";
      for (size_t i=0;i<ndisplay;i++)
        std::cout<<' '<<mst.dead.edge[i].weight;
      if (ndisplay<mst.dead.nedges)
        std::cout<<" ...";
    }
    std::cout<<std::endl;
  }
  const int n = (int)mst.dead.nedges;
  SliceT<uint8> sCorrection(cx,cy),sDM(cx,cy);
  int minCorr = -1;
  for (int i=n-1;i>=0;i--)
  {
    LinkBG link = mst.dead.edge[i];
    if (link.weight>maxCorrectionSize)
    {
      if (minCorr<0) minCorr = (int)link.weight;
      if (link.weight<minCorr) minCorr = (int)link.weight; continue;
    }
    SliceOps::select(sCorrection,vCorrectionMap.slice(link.correctionIndex.slice),link.correctionIndex.label);
    bool doA = (link.weightA<link.weightB);
    if (!doA)
    {
      SliceOps::incr<uint8>(vMaskIn.slice(link.b.slice),sCorrection,128);
    }
    else
    {
      SliceOps::select(sliceCurrent,vSliceSegments.slice(link.a.slice),link.correctionIndex.sliceLabelA);
      SliceOps::dilate4(sDM,sCorrection);
      SliceOps::intersect(sDM,sliceCurrent);
      SliceOps::incr<uint8>(vMaskIn.slice(link.a.slice),sDM,64);
    }
  }
  const int ds = vMaskIn.size();
  for (int i=0;i<ds;i++) { if (vMaskIn[i]>0) vMaskIn[i] = 255; }
  return minCorr;
}

int TCAFixBackground::checkTopology(const Vol3D<uint8> &vMaskIn)
{
  const int cx = vMaskIn.cx;
  const int cy = vMaskIn.cy;
  const int cz = vMaskIn.cz;
  nTotalRegions = labelSliceSegments(vMaskIn);
  Vol3D<uint8> vMap;
  vMap.makeCompatible(vMaskIn);
  vMap.set(0);
  SliceT<uint8> sliceCurrent(cx,cy), sliceDilated(cx,cy);
  SliceT<uint8> regionNextSlice(cx,cy),regionNextSliceMaskedDilated(cx,cy);
  SliceT<sint16> sliceLabeled(cx,cy);
  Vol3D<sint16> vCorrectionMap;
  vCorrectionMap.setsize(cx,cy,nTotalRegions);
  vCorrectionMap.set(0);
  SliceSegmenter sliceSegmentor(cx,cy);
  sliceSegmentor.mode = SliceSegmenter::D8;
  const int zStop = cz - 1;
  int correctionSlice=0;
  EdgeTable<LinkBG> edgeTable;
  for (int z=0; z<zStop; z++)
  {
    for (int i=1;i<=nSliceSymbols[z];i++)
    {
      SliceOps::select(sliceCurrent,vSliceSegments.slice(z),i);
      int firstpos = SliceOps::findFirst(sliceCurrent);
      if (firstpos<0) continue;
      const int labelA = vSliceLabels.slice(z)[firstpos];
      SliceOps::dilate4(sliceDilated,sliceCurrent);
      SliceOps::intersect(sliceDilated,vSliceSegments.slice(z+1));
      sliceSegmentor.segment(sliceDilated.data,sliceLabeled.data,0,255);
      SliceOps::copy<sint16>(vCorrectionMap.slice(correctionSlice),sliceLabeled); // these are the "B" corrections
      const int ns = sliceSegmentor.nSymbols();
      for (int j=1;j<ns;j++)
      {
        SliceOps::select(regionNextSlice,sliceLabeled.data,j); // this is the correction for B!
        const int firstpos = SliceOps::findFirst(regionNextSlice);
        if (firstpos<0) { std::cerr<<"warning: empty region"<<std::endl; continue; }
        const int labelB = vSliceLabels.slice(z+1)[firstpos];
        SliceOps::dilate4(regionNextSliceMaskedDilated,regionNextSlice);
        SliceOps::intersect(regionNextSliceMaskedDilated,sliceCurrent); // correction for A!
        int c = SliceOps::count(regionNextSliceMaskedDilated);
        LinkBG edge(labelA,labelB);
        edge.weightA = (float)c;
        edge.weightB = (float)sliceSegmentor.getInfo(j).count;
        edge.weight = std::min(edge.weightA,edge.weightB);
        edge.correctionIndex.slice = correctionSlice;
        edge.correctionIndex.label = j;
        edge.correctionIndex.sliceLabelA = i;
        edge.a.slice = z;
        edge.b.slice = z+1;
        edgeTable.add(edge);
      }
      correctionSlice++;
    }
  }
  int genus = edgeTable.genus();
  MaximalSpanningTree<LinkBG> mst;
  mst.create(edgeTable);
  if (verbosity>0)
  {
    std::cout<<"("<<std::setw(4)<<genus;
    if (genus==1) std::cout<<" cycle ) ";
    else std::cout<<" cycles) ";
  }
  if (verbosity>1)
  {
    if (genus>0)
    {
      const size_t ndisplay=(mst.dead.nedges>20) ? 20 : mst.dead.nedges;
      if (ndisplay>1)
        std::cout<<" weights :";
      else
        std::cout<<" weight  :";
      for (size_t i=0;i<ndisplay;i++)
        std::cout<<' '<<mst.dead.edge[i].weight;
      if (ndisplay<mst.dead.nedges)
        std::cout<<" ...";
    }
    std::cout<<std::endl;
  }
  return genus;
}
