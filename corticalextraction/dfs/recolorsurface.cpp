// Copyright (C) 2016 The Regents of the University of California
//
// Created by David W. Shattuck, Ph.D.
//
// This file is part of DFS.
//
// DFS is free software; you can redistribute it and/or
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

#include <surface.h>
#include <vol3d.h>
#include <vol3dcrop.h>
#include <volumeloader.h>

inline DSPoint operator*(const float a, const rgb8 &rgb)
{
  return DSPoint(a*rgb.r,a*rgb.g,a*rgb.b);
}

inline DSPoint samplePoint(DSPoint &p, const Vol3D<rgb8> &vMRI)
{
  const float px = (p.x/vMRI.rx);
  const float py = (p.y/vMRI.ry);
  const float pz = (p.z/vMRI.rz);
  int x = (int)px;
  float ax = px - x;
  int y = (int)py;
  float ay = py - y;
  int z = (int)pz;
  float az = pz - z;
  int offset = vMRI.index(x,y,z);
  rgb8 *src = vMRI.start();
  const int cx = vMRI.cx;
  const int ss = cx * vMRI.cy;
  DSPoint f =
     ((1-az)*(
              (1-ay) * ((1-ax)*src[offset]       + ax*src[offset+1])
             +(  ay) * ((1-ax)*src[offset+cx]    + ax*src[offset+cx+1]))
       +(az)*(
              (1-ay) * ((1-ax)*src[offset+ss]    + ax*src[offset+ss+1])
             +(  ay) * ((1-ax)*src[offset+ss+cx] + ax*src[offset+ss+cx+1])));
  return f/255.0f;
}

inline DSPoint imageScale(const float f)
{
  return DSPoint(f,f,f);
}

template <class T>
inline DSPoint samplePoint(DSPoint &p, const Vol3D<T> &vMRI)
{
  const float px = p.x/vMRI.rx;
  const float py = p.y/vMRI.ry;
  const float pz = p.z/vMRI.rz;
  const int x = (int)px;
  const float ax = px - x;
  const int y = (int)py;
  const float ay = py - y;
  const int z = (int)pz;
  const float az = pz - z;
  const int offset = vMRI.index(x,y,z);
  const T *src = vMRI.start();
  const int cx = vMRI.cx;
  const int cy = vMRI.cy;
  const int ss = cx * cy;
  const int sx = vMRI.cx-1;
  const int sy = vMRI.cy-1;
  const int sz = vMRI.cz-1;
  float f = 0;
  if ((x>=0)&&(y>=0)&&(z>0)&&(x<sx)&&(y<sy)&&(z<sz))
  {
    f = ((1-az)*(
           (1-ay) * ((1-ax)*src[offset]    + ax*src[offset+1])
          +(  ay) * ((1-ax)*src[offset+cx] + ax*src[offset+cx+1]))
        +(az)*(
          (1-ay) * ((1-ax)*src[offset+ss]    + ax*src[offset+ss+1])
        +(  ay) * ((1-ax)*src[offset+ss+cx] + ax*src[offset+ss+cx+1])));
  }
  return imageScale(f);
}

template <class T>
T findPercentT(Vol3D<T> &vol,const float f=0.999f)
{
  const int ds = vol.size();
  T mx = vol[0];
  for (int i=0;i<ds;i++)
    if (vol[i]>mx) mx = vol[i];
  return mx;
}

sint32 findPercentT(Vol3D<sint32> &vol,const float f=0.999f)
{
  const int ds = vol.size();
  sint32 mx = vol[0];
  for (int i=0;i<ds;i++)
    if (vol[i]>mx) mx = vol[i];
  return mx;
}

uint32 findPercentT(Vol3D<uint32> &vol,const float f=0.999f)
{
  const int ds = vol.size();
  uint32 mx = vol[0];
  for (int i=0;i<ds;i++)
    if (vol[i]>mx) mx = vol[i];
  return mx;
}

uint8 findPercentT(Vol3D<uint8> &vol,const float f=0.999f)
{
  std::vector<int> h(256,0);
  const int ds=vol.size();
  uint8 *d = vol.start();
  for (int i=0;i<ds;i++) h[d[i]]++;
  int sum=0;
  int stop=(int)(ds*f);
  for (size_t i=0;i<h.size();i++)
  {
    sum += h[i];
    if (sum>=stop) return (uint8)i;
  }
  return 255;
}

uint16 findPercentT(Vol3D<uint16> &vol,const float f=0.999f)
{
  std::vector<int> h(65536,0);
  const int ds=vol.size();
  uint16 *d = vol.start();
  for (int i=0;i<ds;i++) h[d[i]]++;
  int sum=0;
  int stop=(int)(ds*f);
  for (size_t i=0;i<h.size();i++)
  {
    sum += h[i];
    if (sum>=stop) return (uint16)i;
  }
  return 255;
}

sint16 findPercentT(Vol3D<sint16> &vol,const float f=0.999f)
{
  std::vector<int> h(65536,0);
  const int ds=vol.size();
  sint16 *d = vol.start();
  for (int i=0;i<ds;i++)
    h[d[i]+32768]++;
  int sum=0;
  int stop=(int)(ds*f);
  for (size_t i=0;i<h.size();i++)
  {
    sum += h[i];
    if (sum>=stop) return (sint16)(i-32768);
  }
  return 255;
}

float32 findPercentT(Vol3D<float32> &vol,const float f=0.999f)
{
  const int ds=vol.size();
  if (ds<=0) return 255;
  float32 *d = vol.start();
  float mn = vol[0];
  float mx = vol[0];
  for (int i=0;i<ds;i++)
  {
    if (d[i]<mn) mn = d[i];
    if (d[i]>mx) mx = d[i];
  }
  const int binsize = 1000;
  const float step = mx/binsize;
  if (step<=0) return 256;
  std::vector<int> h(binsize,0);
  for (int i=0;i<ds;i++)
  {
    int val = (int)floor(d[i]/step);
    if (val<0) val = 0;
    if (val>=binsize) val = binsize-1;
    h[val]++;
  }
  int sum=0;
  int stop=(int)(ds*f);
  for (size_t i=0;i<h.size();i++)
  {
    sum += h[i];
    if (sum>=stop)
    {
      float f = i*step;
      return f;
    }
  }
  return 255;
}

float64 findPercentT(Vol3D<float64> &vol,const float f=0.999f)
{
  const int ds=vol.size();
  if (ds<=0) return 255;
  float64 *d = vol.start();
  float64 mn = vol[0];
  float64 mx = vol[0];
  for (int i=0;i<ds;i++)
  {
    if (d[i]<mn) mn = d[i];
    if (d[i]>mx) mx = d[i];
  }
  const int binsize = 1000;
  const float64 step = mx/binsize;
  if (step<=0) return 256;
  std::vector<int> h(binsize,0);
  for (int i=0;i<ds;i++)
  {
    int val = (int)floor(d[i]/step);
    if (val<0) val = 0;
    if (val>=binsize) val = binsize-1;
    h[val]++;
  }
  int sum=0;
  int stop=(int)(ds*f);
  for (size_t i=0;i<h.size();i++)
  {
    sum += h[i];
    if (sum>=stop)
    {
      float64 f = i*step;
      return f;
    }
  }
  return 255;
}


bool colorSurfaceRGB(SILT::Surface &dfs, Vol3D<rgb8> &vColor, const float frac=0.99f)
{
  const size_t nv = dfs.vertices.size();
  dfs.vertexColor.resize(nv);
  DSPoint *v = &dfs.vertices[0];
  DSPoint *c = &dfs.vertexColor[0];
  for (size_t i=0;i<nv;i++)
  {
    c[i] = samplePoint(v[i],vColor);
  }
  return true;
}

template <class T> 
bool colorSurfaceT(SILT::Surface &dfs, Vol3D<T> &vColor, const float frac=0.999f)
{
  const size_t nv = dfs.vertices.size();
  dfs.vertexColor.resize(nv);
  DSPoint *v = &dfs.vertices[0];
  DSPoint *c = &dfs.vertexColor[0];
  for (size_t i=0;i<nv;i++)
  {
    c[i] = samplePoint(v[i],vColor);
  }
  T maxv = maxv = findPercentT(vColor,frac);
  if (maxv>0)
    for (size_t i=0;i<nv;i++) c[i] /= maxv;
  return true;
}

bool dispatch(SILT::Surface &dfs, Vol3DBase *base, const float frac=0.99f)
{
  switch (base->typeID())
  {
    case SILT::RGB8   : return colorSurfaceRGB(dfs, *(Vol3D<rgb8> * )base, frac); break;
    case SILT::Uint8   : return colorSurfaceT(dfs, *(Vol3D<uint8> * )base, frac); break;
    case SILT::Sint8   : return colorSurfaceT(dfs, *(Vol3D<sint8>*  )base, frac); break;
    case SILT::Sint16  : return colorSurfaceT(dfs, *(Vol3D<sint16>* )base, frac); break;
    case SILT::Uint16  : return colorSurfaceT(dfs, *(Vol3D<uint16>* )base, frac); break;
    case SILT::Sint32  : return colorSurfaceT(dfs, *(Vol3D<sint32>* )base, frac); break;
    case SILT::Uint32  : return colorSurfaceT(dfs, *(Vol3D<uint32>* )base, frac); break;
    case SILT::Float32 : return colorSurfaceT(dfs, *(Vol3D<float32>*)base, frac); break;
    default: std::cerr<<"Error: unsupported datatype! ("<<base->datatypeName()<<")"<<std::endl; return false;
  }
  return false;
}


template <class T> Vol3DBase *zeroPadT(Vol3D<T> &volume, const int x, const int y, const int z)
{
  Vol3D<T> *v = new Vol3D<T>;
  if (v)
  {
    zeropad(*v,volume,x,y,z);
  }
  return v;
}


Vol3DBase *zeroPad(Vol3DBase *base, const int x, const int y, const int z)
{
  switch (base->typeID())
  {
    case SILT::Uint8   : return zeroPadT(*(Vol3D<uint8> * )base,x,y,z); break;
    case SILT::Sint8   : return zeroPadT(*(Vol3D<sint8>*  )base,x,y,z); break;
    case SILT::Sint16  : return zeroPadT(*(Vol3D<sint16>* )base,x,y,z); break;
    case SILT::Uint16  : return zeroPadT(*(Vol3D<uint16>* )base,x,y,z); break;
    case SILT::Sint32  : return zeroPadT(*(Vol3D<sint32>* )base,x,y,z); break;
    case SILT::Uint32  : return zeroPadT(*(Vol3D<uint32>* )base,x,y,z); break;
    case SILT::Float32 : return zeroPadT(*(Vol3D<float32>*)base,x,y,z); break;
    case SILT::Float64 : return zeroPadT(*(Vol3D<float64>*)base,x,y,z); break;
    case SILT::RGB8    : return zeroPadT(*(Vol3D<rgb8>*)base,x,y,z); break;
    default: std::cerr<<"Unsupported datatype! ("<<base->datatypeName()<<")"<<std::endl; return 0;
  }
  return 0;
}

bool colorSurface(SILT::Surface &dfs, const char *ifname, bool zp, const float frac=0.99f)
{
  Vol3DBase *volume = VolumeLoader::load(ifname);
  if (!volume) return false;
  if (zp)
  {
    Vol3DBase *v2 = zeroPad(volume,2,2,2);
    if (!v2)
    {
      std::cerr<<"an error occurred while zeropadding the data."<<std::endl;
      return false;
    }
    dispatch(dfs,v2,frac);
    delete v2;
  }
  else
    dispatch(dfs,volume,frac);
  delete volume;
  return true;
}
