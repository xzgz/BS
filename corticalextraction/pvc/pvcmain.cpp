// Copyright (C) 2016 The Regents of the University of California
//
// Created by David W. Shattuck, Ph.D.
//
// This file is part of PVC.
//
// PVC is free software; you can redistribute it and/or
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

#include <PVC/pvctool.h>
#include <vol3dbase.h>
#include <volumeloader.h>
#include <DS/timer.h>
#include <vol3dlib.h>
#include "pvcparser.h"

template <class T> bool nonzero(Vol3D<uint8> &vm, const Vol3DBase *vIn)
{
  const Vol3D<T> &v = *(Vol3D<T> *)vIn;
  if (vm.makeCompatible(v)==false) return false;
  const int ds = vm.size();
  for (int i=0;i<ds;i++)
    vm[i] = (v[i] != 0) ? 255 : 0;
  return true;
}

bool makeMask(Vol3D<uint8> &vm, const Vol3DBase *v)
{
  switch (v->typeID())
  {
    case SILT::Uint8 : return nonzero<uint8>(vm,v); break;
    case SILT::Sint8 : return nonzero<sint8>(vm,v); break;
    case SILT::Uint16 : return nonzero<uint16>(vm,v); break;
    case SILT::Sint16 : return nonzero<sint16>(vm,v); break;
    case SILT::Float32: return nonzero<float32>(vm,v); break;
    default:
      return false;
  }
  return false;
}

int main(int argc, char *argv[])
{
  PVCParser parser;
  if (!parser.parse(argc,argv)) return parser.usage();
  if (!parser.validate()) return parser.usage();
  Timer timer;
  if (parser.timer) timer.start();
  if (parser.verbosity>1) std::cout<<"using lambda = "<<parser.lambda<<std::endl;
  Vol3DBase *volume = VolumeLoader::load(parser.ifname);
  if (!volume) { std::cerr<<"unable to load "<<parser.ifname<<std::endl; return 1; }
  if (parser.verbosity>1)
    std::cout<<"read input file "<<parser.ifname<<std::endl;
  PVCTool pvcTool(parser.verbosity);
  Vol3D<uint8> vMask;
  if (parser.mfname.empty())
  {
    makeMask(vMask,volume);
  }
  else
  {
    if (vMask.read(parser.mfname)==false)
    {
      std::cerr<<"Error: failed to read "<<parser.mfname<<std::endl;
      return 1;
    }
  }
  if (pvcTool.initializeML(pvcTool.vl,volume,vMask)==false)
  {
    return 1;
  }
  if (parser.timer) { timer.stop(); if (parser.verbosity>1) std::cout<<"Init took "<<timer.elapsed()<<std::endl; }
  pvcTool.icm(pvcTool.vl,parser.lambda,100,parser.threeClass);
  if (parser.timer) { timer.stop(); if (parser.verbosity>1) std::cout<<"ICM took "<<timer.elapsed()<<std::endl; }
  if (parser.ofname.empty()==false)
  {
    if (pvcTool.vl.write(parser.ofname))
    {
      if (parser.verbosity>0)
        std::cout<<"Wrote tissue label file "<<parser.ofname<<std::endl;
    }
    else
    {
      std::cerr<<"Error: unable to write "<<parser.ofname<<std::endl;
    }
  }

  if (parser.fracfname.empty()==false)
  {
    Vol3D<float> vFractions;
    pvcTool.computeFractions(vFractions, pvcTool.vb, pvcTool.vl, pvcTool.theta);
    if (vFractions.write(parser.fracfname))
    {
      if (parser.verbosity>0)
        std::cout<<"Wrote tissue fraction file "<<parser.fracfname<<std::endl;
    }
    else
    {
      std::cerr<<"Error: unable to write fraction file "<<parser.fracfname<<std::endl;
    }
  }
  if (parser.timer) { timer.stop(); std::cout<<"PVC took "<<timer.elapsed()<<std::endl; }
  return 0;
}
