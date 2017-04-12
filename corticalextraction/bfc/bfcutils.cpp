// Copyright (C) 2016 The Regents of the University of California
//
// Created by David W. Shattuck, Ph.D.
//
// This file is part of BFC.
//
// BFC is free software; you can redistribute it and/or
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

#include <BFC/biasfieldcorrector.h>
#include <volumescaler.h>

BiasFieldCorrector::IterationParameters BiasFieldCorrector::defaultSchedule[] =
{
  BiasFieldCorrector::IterationParameters(24,16,64,0),
  BiasFieldCorrector::IterationParameters(24,16,64,0),
  BiasFieldCorrector::IterationParameters(16,16,48,0),
  BiasFieldCorrector::IterationParameters(16,16,48,0),
  BiasFieldCorrector::IterationParameters(12,16,32,0),
  BiasFieldCorrector::IterationParameters(12,16,32,0),
  BiasFieldCorrector::IterationParameters(12,16,24,0),
  BiasFieldCorrector::IterationParameters(12,16,24,0),
  BiasFieldCorrector::IterationParameters(12,16,16,0),
  BiasFieldCorrector::IterationParameters(12,16,16,0),
};

bool BiasFieldCorrector::loadDefaultSchedule()
{
  const int nDefaultIterations = sizeof(BiasFieldCorrector::defaultSchedule)/sizeof(BiasFieldCorrector::defaultSchedule[0]);
  for (int i=0;i<nDefaultIterations;i++)
    schedule.push_back(BiasFieldCorrector::defaultSchedule[i]);
  return true;
}

bool BiasFieldCorrector::readSchedule(std::string ifname)
{
  std::ifstream ifile(ifname.c_str());
  if (!ifile)
  {
    return false;
  }
  while (!ifile.eof())
  {
    ifile>>std::ws;
    std::string str;
    getline(ifile, str);
    if (str.length()==0) continue;
    std::istringstream istr(str);
    int r,s,c;
    float w;
    istr>>r>>s>>c>>w;
    schedule.push_back(IterationParameters(r,s,c,w));
    if (debug) std::cout<<"\tSchedule has "<<schedule.size()<<" elements."<<std::endl;
  }
  return true;
}

template <class T>
bool BiasFieldCorrector::initialize(Vol3D<T> &vIn, Vol3D<uint8> &vMask)
{
  if (vMask.size()>0)
  {
    if (!vMask.isCompatible(vIn))
    {
      std::cerr<<"Error: mask volume ("<<vMask.filename<<" and image volume "<<vIn.filename<<" have different dimensions."<<std::endl;
      return false;
    }
    VolumeScaler::scaleToUint8Masked(vol,vIn,vMask);
    return true;
  }
  else
  {
    VolumeScaler::scaleToUint8(vol,vIn);
    return true;
  }
}

bool BiasFieldCorrector::initialize(Vol3DBase *vIn, Vol3D<uint8> &vMask)
{
  switch (vIn->typeID())
  {
    case SILT::Sint8:   return initialize(*(Vol3D<uint8> *)vIn,vMask); break;
    case SILT::Uint8:   return initialize(*(Vol3D<uint8> *)vIn,vMask); break;
    case SILT::Sint16:  return initialize(*(Vol3D<sint16> *)vIn,vMask); break;
    case SILT::Uint16:  return initialize(*(Vol3D<uint16> *)vIn,vMask); break;
    case SILT::Sint32:  return initialize(*(Vol3D<sint16> *)vIn,vMask); break;
    case SILT::Uint32:  return initialize(*(Vol3D<uint16> *)vIn,vMask); break;
    case SILT::Float32: return initialize(*(Vol3D<float32> *)vIn,vMask); break;
    case SILT::Float64: return initialize(*(Vol3D<float64> *)vIn,vMask); break;
    default:
      std::cerr<<"BFC cannot process data of type "<<vIn->datatypeName();
      return false;
  }
}
