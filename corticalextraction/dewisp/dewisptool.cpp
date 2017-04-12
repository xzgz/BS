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

#include <dewisptool.h>
#include <dewisp.h>
#include <DS/runlengthsegmenter.h>

void DewispTool::ensureCC(Vol3D<uint8> &vMask)
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

void DewispTool::process(Vol3D<uint8> &volume, int /*verbosity*/)
{
  Dewisp wisper(sizeThreshold);
  wisper.process(volume);
  ensureCC(volume);
}

void DewispTool::processXZ(Vol3D<uint8> &volume, int /*verbosity*/)
{
  Dewisp wisper(sizeThreshold);
  Vol3D<uint8> vTranspose;
  transposeXZ(vTranspose,volume);
  wisper.process(vTranspose);
  transposeXZ(volume,vTranspose);
  ensureCC(volume);
}

void DewispTool::processYZ(Vol3D<uint8> &volume, int /*verbosity*/)
{
  Dewisp wisper(sizeThreshold);
  Vol3D<uint8> vTranspose;
  transposeYZ(vTranspose,volume);
  wisper.process(vTranspose);
  transposeYZ(volume,vTranspose);
  ensureCC(volume);
}

void DewispTool::transposeXZ(Vol3D<uint8> &vDst, const Vol3D<uint8> &vSrc)
{
  const int cx = vSrc.cx;
  const int cy = vSrc.cy;
  const int cz = vSrc.cz;
  vDst.setsize(cz,cy,cx);
  for (int z=0; z<cz; z++)
    for (int y=0; y<cy; y++)
      for (int x=0; x<cx; x++)
        vDst(z,y,x) = vSrc(x,y,z);
}

void DewispTool::transposeYZ(Vol3D<uint8> &vDst, const Vol3D<uint8> &vSrc)
{
  const int cx = vSrc.cx;
  const int cy = vSrc.cy;
  const int cz = vSrc.cz;
  vDst.setsize(cx,cz,cy);
  for (int z=0; z<cz; z++)
    for (int y=0; y<cy; y++)
      for (int x=0; x<cx; x++)
        vDst(x,z,y) = vSrc(x,y,z);
}

int DewispTool::delta(const Vol3D<uint8> &vA, const Vol3D<uint8> &vB)
{
  const int ds = vA.size();
  int count=0;
  for (int i=0;i<ds;i++)
    count += (vA[i]!=vB[i]);
  return count;
}

int DewispTool::loop(Vol3D<uint8> &vOut, const Vol3D<uint8> &vIn, int verbosity)
{
  if (verbosity>1) std::cout<<"Z-axis. ";
  process(vOut,verbosity);
  if (verbosity>1)
  {
    const int nX=delta(vOut,vIn);
    std::cout<<"cumulative voxels changed: "<<nX<<std::endl;
  }
  if (verbosity>1) std::cout<<"Y-axis. ";
  processYZ(vOut,verbosity);
  if (verbosity>1)
  {
    const int nY=delta(vOut,vIn);
    std::cout<<"cumulative voxels changed: "<<nY<<std::endl;
  }
  if (verbosity>1) std::cout<<"X-axis. ";
  processXZ(vOut,verbosity);
  int nZ = delta(vOut,vIn);
  if (verbosity>1) std::cout<<"cumulative voxels changed: "<<nZ<<std::endl;
  return nZ;
}
