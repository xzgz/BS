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

#include <findodfmaximathread.h>
#include <findodfmaximamanager.h>
#include <harditool.h>

FindODFMaximaThread::FindODFMaximaThread(FindODFMaximaManager *parent, Vol3D<EigenSystem3x3f> &vEig, const Vol3D<uint8> &vMask, const HARDITool &hardiTool) :
  parent(parent), vEig(vEig), hardiTool(hardiTool), vMask(vMask), threadNumber(threadCount++)
{
  useMask = vMask.isCompatible(vEig);
}

int FindODFMaximaThread::threadCount=0;

bool FindODFMaximaThread::runSlice(size_t z)
{
  if (z>=vEig.cz) return false;
  const auto cx=vEig.cx;
  const auto cy=vEig.cy;
  if (vMask.isCompatible(vEig)==false) useMask=false;
  auto eig=vEig.slice(z);
  if (useMask)
  {
    auto mask=vMask.slice(z);
    size_t i=0;
    for (size_t y=0;y<cy;y++)
      for (size_t x=0;x<cx;x++,i++)
        eig[i]=(mask[i]) ? hardiTool.findODFMaxima(x,y,z) : EigenSystem3x3f();
  }
  else
  {
    size_t i=0;
    for (size_t y=0;y<cy;y++)
      for (size_t x=0;x<cx;x++,i++)
        eig[i]= hardiTool.findODFMaxima(x,y,z);
  }
  return true;
}

void FindODFMaximaThread::run()
{
  int z=parent->nextSlice();
  while (z>=0)
  {
    runSlice(z);
    z=parent->nextSlice();
  }
}

