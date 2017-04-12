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

#include <computerange.h>
#include <vol3d.h>

sint32 ComputeRange::findMax(Vol3D<sint32> &vol)
{
  const size_t ds = vol.size();
  sint32 mx = vol[0];
  for (size_t i=0;i<ds;i++)
    if (vol[i]>mx) mx = vol[i];
  return mx;
}

uint32 ComputeRange::findMax(Vol3D<uint32> &vol)
{
  const size_t ds = vol.size();
  uint32 mx = vol[0];
  for (size_t i=0;i<ds;i++)
    if (vol[i]>mx) mx = vol[i];
  return mx;
}

uint8 ComputeRange::findPercentileT(Vol3D<uint8> &vol, const float f)
{
  std::vector<size_t> h(256,0);
  const size_t ds=vol.size();
  const uint8 *d = vol.start();
  for (size_t i=0;i<ds;i++) h[d[i]]++;
  size_t sum=0;
  size_t stop=(size_t)(ds*f);
  for (size_t i=0;i<h.size();i++)
  {
    sum += h[i];
    if (sum>=stop) return (uint8)i;
  }
  return 255;
}

uint16 ComputeRange::findPercentileT(Vol3D<uint16> &vol, const float f)
{
  std::vector<size_t> h(65536,0);
  const size_t ds=vol.size();
  uint16 *d = vol.start();
  for (size_t i=0;i<ds;i++) h[d[i]]++;
  size_t sum=0;
  size_t stop=(size_t)(ds*f);
  for (size_t i=0;i<h.size();i++)
  {
    sum += h[i];
    if (sum>=stop) return (uint16)i;
  }
  return 255;
}

sint16 ComputeRange::findPercentileT(Vol3D<sint16> &vol, const float f)
{
  std::vector<size_t> h(65536,0);
  const size_t ds=vol.size();
  sint16 *d = vol.start();
  for (size_t i=0;i<ds;i++)
    h[d[i]+32768]++;
  size_t sum=0;
  size_t stop=(size_t)(ds*f);
  for (size_t i=0;i<h.size();i++)
  {
    sum += h[i];
    if (sum>=stop) return (sint16)(i-32768);
  }
  return 255;
}

template <class FloatType>
FloatType ComputeRange::findPercentFloat(Vol3D<FloatType> &vol,const float f)
{
  const size_t ds=vol.size();
  if (ds<=0) return 255;
  auto *d = vol.start();
  float mn = vol[0];
  float mx = vol[0];
  for (size_t i=0;i<ds;i++)
  {
    if (d[i]<mn) mn = d[i];
    if (d[i]>mx) mx = d[i];
  }
  if (mx<=0) return 256;
  const int binsize = 1000000;
  float step=mx;
  for (int loop=0;loop<6;loop++) // will allow for a dynamic range of 1000000^6, e.g., 10^36!
  {
    step/=binsize;
    std::vector<size_t> h(binsize,0);
    for (size_t i=0;i<ds;i++)
    {
      int val = (int)floor(d[i]/step);
      if (val<0) val = 0;
      if (val>=binsize) val = binsize-1;
      h[val]++;
    }
    size_t sum=0;
    size_t stop=(size_t)(ds*f);
    for (size_t i=0;i<h.size();i++)
    {
      sum += h[i];
      if (sum>=stop)
      {
        if (i==0) break;
        float f = i*step;
        return f;
      }
    }
  }
  return (mx<255) ? mx : 255;
}

// for 32bit integers, we just compute the max instead of performing
// histograms on the large range of integers
// could adapt to use same approach as for floats
double ComputeRange::findRange(Vol3DBase *vol, const float percentile)
{
  double v=256;
  switch (vol->typeID())
  {
  case SILT::Uint8 : v=findPercentileT(*(Vol3D<uint8> *)vol,percentile); break;
  case SILT::Sint16: v=findPercentileT(*(Vol3D<sint16> *)vol,percentile); break;
  case SILT::Uint16: v=findPercentileT(*(Vol3D<uint16> *)vol,percentile); break;
  case SILT::Uint32: v=findMax(*(Vol3D<uint32> *)vol); break;
  case SILT::Sint32: v=findMax(*(Vol3D<sint32> *)vol); break;
  case SILT::Float32 : v=findPercentFloat(*(Vol3D<float32> *)vol,percentile); break;
  case SILT::Float64 : v=findPercentFloat(*(Vol3D<float64> *)vol,percentile); break;
  default: break; // there are unhandled types
  }
  return v;
}

template <class T>
float ComputeRange::findMinT(Vol3D<T> &v)
{
  const size_t ds = v.size();
  if (ds!=0)
  {
    T minVal = v[0];
    for (size_t i=1;i<ds;i++)
      if (v[i]<minVal) minVal = v[i];
    return (float)minVal;
  }
  return 0;
}

float ComputeRange::findMin(Vol3DBase *vol)
{
  float v=0;
  switch (vol->typeID())
  {
    case SILT::Sint16: v=findMinT(*(Vol3D<sint16> *)vol); break;
    case SILT::Sint32: v=findMinT(*(Vol3D<sint32> *)vol); break;
    case SILT::Float32 : v=findMinT(*(Vol3D<float32> *)vol); break;
    case SILT::Float64 : v=findMinT(*(Vol3D<float64> *)vol); break;
    default: break; // there are unhandled types
  }
  return v;
}
