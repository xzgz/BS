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

#include <set>
#include <vol3d.h>
#include <surflib.h>
#include <colorops.h>
#include <surfacerecolorchoices.h>
#include <surfacerecolor.h>
#include <surfacealpha.h>
#include <brainsuitewindow.h>
#include <cerebrumlabelerqt.h>
#include <cerebroguitool.h>

extern CerebroGUITool cerebroGUITool;

inline DSPoint operator*(const float a, const rgb8 &rgb)
{
  return DSPoint(a*rgb.r,a*rgb.g,a*rgb.b);
}

inline DSPoint rgb2pt(const uint32 a)
{
  unsigned char *p = (unsigned char *)&a;
  return DSPoint(p[2]/255.0f,p[1]/255.0f,p[0]/255.0f);
}

inline DSPoint worldToVoxel(DSPoint &p, const Vol3DBase &vMRI)
{
  const float px = p.x/vMRI.rx;
  const float py = p.y/vMRI.ry;
  const float pz = p.z/vMRI.rz;
  return DSPoint(px,py,pz);
}

inline DSPoint samplePoint(DSPoint &p, const Vol3D<rgb8> &vMRI)
{
  DSPoint pVC = worldToVoxel(p,vMRI);
  int x = (int)pVC.x;
  float ax = pVC.x - x;
  int y = (int)pVC.y;
  float ay = pVC.y - y;
  int z = (int)pVC.z;
  float az = pVC.z - z;
  int offset = vMRI.index(x,y,z);
  rgb8 *src = vMRI.start();
  const int cx = vMRI.cx;
  const int ss = cx * vMRI.cy;
  DSPoint f =
      ((1-az)*(
          (1-ay) * ((1-ax)*src[offset]    + ax*src[offset+1])
        + (  ay) * ((1-ax)*src[offset+cx] + ax*src[offset+cx+1]))
         +(az)*(
          (1-ay) * ((1-ax)*src[offset+ss]    + ax*src[offset+ss+1])
        +(  ay) * ((1-ax)*src[offset+ss+cx] + ax*src[offset+ss+cx+1])));
  return f/255.0f;
}

template <class T>
inline float samplePoint(DSPoint &p, const Vol3D<T> &vMRI)
{
  DSPoint pVC = worldToVoxel(p,vMRI);
  int x = (int)pVC.x;
  float ax = pVC.x - x;
  int y = (int)pVC.y;
  float ay = pVC.y - y;
  int z = (int)pVC.z;
  float az = pVC.z - z;
  const int offset = vMRI.index(x,y,z);
  const T *src = vMRI.start();
  const int cx = vMRI.cx;
  const int cy = vMRI.cy;
  const int ss = cx * cy;
  const int sx = vMRI.cx-1;
  const int sy = vMRI.cy-1;
  const int sz = vMRI.cz-1;

  double f = 0; // could be single precision
  if ((x>=0)&&(y>=0)&&(z>0)&&(x<sx)&&(y<sy)&&(z<sz))
  {
    f = ((1-az)*(
           (1-ay) * ((1-ax)*src[offset]    + ax*src[offset+1])
         +(  ay) * ((1-ax)*src[offset+cx] + ax*src[offset+cx+1]))
        +(az)*(
          (1-ay) * ((1-ax)*src[offset+ss]    + ax*src[offset+ss+1])
        +(  ay) * ((1-ax)*src[offset+ss+cx] + ax*src[offset+ss+cx+1])));
  }
  return (float)f;
}

SurfaceRecolor::SurfaceRecolor()
{
}

template <class VecT>
float SurfaceRecolor::quantify(VecT &curvature, const float percent)
{
  const int bins = 1000;
  std::vector<int> histogram(bins,0);
  const size_t nv = curvature.size();
  const float *c = &curvature[0];
  for (size_t i=0;i<nv;i++)
  {
    const int v = abs((int)(c[i]*bins));
    histogram[(v<bins) ? v : (bins-1) ]++;
  }
  int hittest = (int)(percent * nv);
  int count = 0;
  for (size_t i=0;i<nv;i++)
  {
    count+=histogram[i];
    if (count>=hittest) return float(i)/bins;
  }
  return 0;
}

bool SurfaceRecolor::recolorSurfaceWithVertexLabels(SILT::Surface *surface)
{
  const size_t nv = surface->nv();
  if (surface->vertexLabels.size()!=nv) return false;
  surface->vertexColor.resize(nv);
  for (size_t i=0;i<nv;i++)
  {
    surface->vertexColor[i] = ::rgb2pt(LabelOp::colorTable[surface->vertexLabels[i]]);
  }
  return true;
}

template <class T>
bool SurfaceRecolor::recolorSurfaceWithVolumeLabels(SILT::Surface &surface, Vol3D<T> &vLabel)
{
  const size_t nv = surface.nv();
  surface.vertexColor.resize(nv);
  DSPoint *color = &surface.vertexColor[0];
  DSPoint *point = &surface.vertices[0];
  const int cx = vLabel.cx;
  const int cy = vLabel.cy;
  const int cz = vLabel.cz;
  const float rx = vLabel.rx;
  const float ry = vLabel.ry;
  const float rz = vLabel.rz;
  DSPoint origin=vLabel.origin;
  origin=DSPoint(0,0,0);
  for (size_t i=0;i<nv;i++)
  {
    int x = int(point[i].x/rx+0.5f);
    int y = int(point[i].y/ry+0.5f);
    int z = int(point[i].z/rz+0.5f);
    if ((x<0)||(y<0)||(z<0)||(x>=cx)||(y>=cy)||(z>=cz))
    {
      color[i] = DSPoint(0,0,0);
    }
    else
    {
      color[i] = rgb2pt(LabelOp::colorTable[vLabel(x,y,z)]);

    }
  }
  return true;
}

bool SurfaceRecolor::recolorSurfaceWithVolumeLabels(SILT::Surface *surface, Vol3DBase *vLabel)
{
  bool state = false;
  switch (vLabel->typeID())
  {
    case SILT::Sint8 :
    case SILT::Uint8 : state = recolorSurfaceWithVolumeLabels(*surface, *(Vol3D<uint8> *)vLabel); break;
    case SILT::Sint16: // avoid negative labels
    case SILT::Uint16: state = recolorSurfaceWithVolumeLabels(*surface, *(Vol3D<uint16>*)vLabel); break;
    case SILT::Sint32: // avoid negative labels
    case SILT::Uint32: state = recolorSurfaceWithVolumeLabels(*surface, *(Vol3D<uint32>*)vLabel); break;
    default: break;
  }
  return state;
}

template <class T>
bool SurfaceRecolor::colorSurfaceWithScaledImage_T(SILT::Surface &dfs, Vol3D<T> &vColor, const RangeLUTOp &op)
{
  const size_t nv = dfs.vertices.size();
  dfs.vertexColor.resize(nv);
  DSPoint *vertices = &dfs.vertices[0];
  DSPoint *vertexColor = &dfs.vertexColor[0];
  for (size_t i=0;i<nv;i++)
  {
    uint32 colorref;
    op(colorref,samplePoint(vertices[i],vColor));
    vertexColor[i] = rgb2pt(colorref);
  }
  return true;
}

template <>
bool SurfaceRecolor::colorSurfaceWithScaledImage_T(SILT::Surface &dfs, Vol3D<rgb8> &vColor, const RangeLUTOp &/*op*/)
{
  const size_t nv = dfs.vertices.size();
  dfs.vertexColor.resize(nv);
  DSPoint *vertices = &dfs.vertices[0];
  DSPoint *vertexColor = &dfs.vertexColor[0];
  for (size_t i=0;i<nv;i++)
  {
    vertexColor[i] = samplePoint(vertices[i],vColor);
  }
  return true;
}

bool SurfaceRecolor::colorSurfaceWithScaledImage(SILT::Surface &dfs, Vol3DBase *base, const RangeLUTOp &op)
{
  switch (base->typeID())
  {
    case SILT::RGB8   : return SurfaceRecolor::colorSurfaceWithScaledImage_T(dfs, *(Vol3D<rgb8> * )base, op); break;
    case SILT::Uint8   : return SurfaceRecolor::colorSurfaceWithScaledImage_T(dfs, *(Vol3D<uint8> * )base, op); break;
    case SILT::Sint8   : return SurfaceRecolor::colorSurfaceWithScaledImage_T(dfs, *(Vol3D<sint8>*  )base, op); break;
    case SILT::Sint16  : return SurfaceRecolor::colorSurfaceWithScaledImage_T(dfs, *(Vol3D<sint16>* )base, op); break;
    case SILT::Uint16  : return SurfaceRecolor::colorSurfaceWithScaledImage_T(dfs, *(Vol3D<uint16>* )base, op); break;
    case SILT::Sint32  : return SurfaceRecolor::colorSurfaceWithScaledImage_T(dfs, *(Vol3D<sint32>* )base, op); break;
    case SILT::Uint32  : return SurfaceRecolor::colorSurfaceWithScaledImage_T(dfs, *(Vol3D<uint32>* )base, op); break;
    case SILT::Float32 : return SurfaceRecolor::colorSurfaceWithScaledImage_T(dfs, *(Vol3D<float32>*)base, op); break;
    case SILT::Float64 : return SurfaceRecolor::colorSurfaceWithScaledImage_T(dfs, *(Vol3D<float64>*)base, op); break;
    default: std::cerr<<"Unsupported datatype! ("<<base->datatypeName()<<")"<<std::endl; return false;
  }
  return false;
}

template <class T>
bool recolorSurfaceTBlend(SILT::Surface &surface, Vol3D<T> &vLabel, const float alpha)
{
  const size_t nv = surface.nv();
  surface.vertexColor.resize(nv);
  DSPoint *color = &surface.vertexColor[0];
  DSPoint *point = &surface.vertices[0];
  const int cx = vLabel.cx;
  const int cy = vLabel.cy;
  const int cz = vLabel.cz;
  const float rx = vLabel.rx;
  const float ry = vLabel.ry;
  const float rz = vLabel.rz;
  const float beta = 1.0f - alpha;
  for (size_t i=0;i<nv;i++)
  {
    int x = int(point[i].x/rx+0.5f);
    int y = int(point[i].y/ry+0.5f);
    int z = int(point[i].z/rz+0.5f);
    if ((x<0)||(y<0)||(z<0)||(x>=cx)||(y>=cy)||(z>=cz))
    {
      color[i] = beta * color[i];
    }
    else
    {
      color[i] = beta * color[i] + alpha * rgb2pt(LabelOp::colorTable[vLabel(x,y,z)]);
    }
  }
  return true;
}

bool recolorSurfaceBlend(SILT::Surface *surface, Vol3DBase *vLabel, const float alpha)
{
  bool state = false;
  switch (vLabel->typeID())
  {
    case SILT::Uint8 : state = recolorSurfaceTBlend(*surface, *(Vol3D<uint8> *)vLabel,alpha); break;
    case SILT::Sint16: // avoid negative labels
    case SILT::Uint16: state = recolorSurfaceTBlend(*surface, *(Vol3D<uint16>*)vLabel,alpha); break;
    default: break;
  }
  return state;
}

bool SurfaceRecolor::recolorSurfaceWithCurvature(SILT::Surface *surface)
{
  SurfLib::computeCurvature(*surface);
  const size_t nv = surface->vertices.size();
  surface->vertexColor.resize(nv);
  DSPoint *vertexColor = &surface->vertexColor[0];
  const float *meanCurvature = &surface->meanCurvature[0];
  const float upperLevel = quantify(surface->meanCurvature);
  const float scale = (upperLevel>0) ? 0.5f/upperLevel : 0.5f;
  for (size_t i=0;i<nv;i++)
  {
    const float v = meanCurvature[i]*scale + 0.5f;
    vertexColor[i] = DSPoint(v,v,v);
  }
  return true;
}

bool colorSurfaceRBlend(SILT::Surface &dfs, Vol3D<rgb8> &vColor, const RangeLUTOp & /* op */, const float alpha)
{
  const size_t nv = dfs.vertices.size();
  dfs.vertexColor.resize(nv);
  DSPoint *v = &dfs.vertices[0];
  DSPoint *c = &dfs.vertexColor[0];
  const float b = (1-alpha);
  for (size_t i=0;i<nv;i++)
  {
    c[i] = b * c[i] + alpha * samplePoint(v[i],vColor);
  }
  return true;
}


template <class T>
bool colorSurfaceRBlend(SILT::Surface &dfs, Vol3D<T> &vColor, const RangeLUTOp &op, const float alpha)
{
  const size_t nv = dfs.vertices.size();
  dfs.vertexColor.resize(nv);
  DSPoint *v = &dfs.vertices[0];
  DSPoint *c = &dfs.vertexColor[0];
  const float b = 1 - alpha;
  for (size_t i=0;i<nv;i++)
  {
    uint32 colorref;
    op(colorref,samplePoint(v[i],vColor));
    c[i] = b * c[i] + alpha * rgb2pt(colorref);
  }
  return true;
}

bool dispatchRBlend(SILT::Surface &dfs, Vol3DBase *base, const RangeLUTOp &op, const float alpha)
{
  switch (base->typeID())
  {
    case SILT::RGB8   : return colorSurfaceRBlend(dfs, *(Vol3D<rgb8> * )base, op, alpha); break;
    case SILT::Uint8   : return colorSurfaceRBlend(dfs, *(Vol3D<uint8> * )base, op, alpha); break;
    case SILT::Sint8   : return colorSurfaceRBlend(dfs, *(Vol3D<sint8>*  )base, op, alpha); break;
    case SILT::Sint16  : return colorSurfaceRBlend(dfs, *(Vol3D<sint16>* )base, op, alpha); break;
    case SILT::Uint16  : return colorSurfaceRBlend(dfs, *(Vol3D<uint16>* )base, op, alpha); break;
    case SILT::Sint32  : return colorSurfaceRBlend(dfs, *(Vol3D<sint32>* )base, op, alpha); break;
    case SILT::Uint32  : return colorSurfaceRBlend(dfs, *(Vol3D<uint32>* )base, op, alpha); break;
    case SILT::Float32 : return colorSurfaceRBlend(dfs, *(Vol3D<float32>*)base, op, alpha); break;
    case SILT::Float64 : return colorSurfaceRBlend(dfs, *(Vol3D<float64>*)base, op, alpha); break;
    default: std::cerr<<"unsupported datatype! ("<<base->datatypeName()<<")"<<std::endl; return false;
  }
  return false;
}

template <class VecT>
std::pair<float,float> getRange(VecT &v)
{
  float mn=v[0],mx=v[0];
  const size_t nv=v.size();
  for (size_t i=0;i<nv;i++) { if (v[i]<mn) mn=v[i]; if (v[i]>mx) mx=v[i]; }
  return std::pair<float,float>(mn,mx);
}

inline int clamp8(float f) { return (f<0) ? 0 : ((f>255) ? 255 : (int)f); }

bool surfaceBlend(SurfaceAlpha *surface, BrainSuiteWindow *brainSuiteWindow)
{
  if (!brainSuiteWindow) return false;
  if (!surface) return false;
  bool started = false;
  if (brainSuiteWindow->brainSuiteDataManager.volume && brainSuiteWindow->imageState.viewVolume)
  {
    RangeLUTOp op(brainSuiteWindow->imageLUT,brainSuiteWindow->imageState.brightness.minv,brainSuiteWindow->imageState.brightness.maxv);
    SurfaceRecolor::colorSurfaceWithScaledImage(*surface, brainSuiteWindow->brainSuiteDataManager.volume,op);
    started = true;
  }
  if (brainSuiteWindow->brainSuiteDataManager.imageOverlay1Volume && brainSuiteWindow->imageState.viewOverlay)
  {
    RangeLUTOp op(brainSuiteWindow->imageOverlay1LUT,brainSuiteWindow->imageState.overlay1Brightness.minv,brainSuiteWindow->imageState.overlay1Brightness.maxv);
    const float alpha = (started) ? brainSuiteWindow->imageState.overlayAlpha/255.0f : 1.0f;
    dispatchRBlend(*surface, brainSuiteWindow->brainSuiteDataManager.imageOverlay1Volume,op,alpha);
    started = true;
  }
  if (brainSuiteWindow->brainSuiteDataManager.imageOverlay2Volume && brainSuiteWindow->imageState.viewOverlay2)
  {
    RangeLUTOp op(brainSuiteWindow->imageOverlay2LUT,brainSuiteWindow->imageState.overlay2Brightness.minv,brainSuiteWindow->imageState.overlay2Brightness.maxv);
    const float alpha = (started) ? brainSuiteWindow->imageState.overlay2Alpha/255.0f : 1.0f;
    dispatchRBlend(*surface, brainSuiteWindow->brainSuiteDataManager.imageOverlay2Volume,op,alpha);
    started = true;
  }
  if (brainSuiteWindow->brainSuiteDataManager.vLabel && brainSuiteWindow->imageState.viewLabels)
  {
    const float alpha = (started) ? brainSuiteWindow->imageState.labelAlpha/255.0f : 1.0f;
    recolorSurfaceBlend(surface, brainSuiteWindow->brainSuiteDataManager.vLabel, alpha);
  }
  surface->useSolidColor = false;
  surface->useVertexColor = true;
  return true;
}


bool surfaceRecolorRequirementsOK(const int recolorChoice, BrainSuiteWindow *brainSuiteWindow)
{
  switch (recolorChoice)
  {
    case SurfaceRecolorChoices::NoColor: return true; break;
    case SurfaceRecolorChoices::Curvature : return true; break;
    case SurfaceRecolorChoices::Labels :
      if (brainSuiteWindow && brainSuiteWindow->brainSuiteDataManager.vLabel)
      {
        switch (brainSuiteWindow->brainSuiteDataManager.vLabel->typeID())
        {
          case SILT::Sint8 :
          case SILT::Uint8 :
          case SILT::Sint16:
          case SILT::Uint16:
          case SILT::Sint32:
          case SILT::Uint32: return true;
          default: break;
        }
      }
      break;
    case SurfaceRecolorChoices::Hemispheres :
      if (brainSuiteWindow && cerebroGUITool.cerebrumLabeler.vLabeledLobes.size()>0)
      {
        switch (cerebroGUITool.cerebrumLabeler.vLabeledLobes.typeID())
        {
          case SILT::Sint8 :
          case SILT::Uint8 :
          case SILT::Sint16:
          case SILT::Uint16:
          case SILT::Sint32:
          case SILT::Uint32:
            return true;
            break;
          default:
            break;
        }
      }
      break;
    case SurfaceRecolorChoices::Intensity :
      if (brainSuiteWindow && brainSuiteWindow->brainSuiteDataManager.volume) return true;
      break;
    case SurfaceRecolorChoices::Blend : return true; break;
    case SurfaceRecolorChoices::VertexLabels : return true; break;
    case SurfaceRecolorChoices::VertexAttributes : return true; break;
    default: break;
  }
  return false;
}

bool surfaceRecolor(SurfaceAlpha *surface, const int recolorChoice, BrainSuiteWindow *brainSuiteWindow)
{
  if (!surface) return false;
  switch (recolorChoice)
  {
    case SurfaceRecolorChoices::NoColor:
      {
        surface->vertexColor.resize(0);
        surface->useSolidColor = true;
        surface->useVertexColor = false;
      }
      return true;
      break;
    case SurfaceRecolorChoices::Curvature :
      if (SurfaceRecolor::recolorSurfaceWithCurvature(surface))
      {
        surface->useSolidColor = false;
        surface->useVertexColor = true;
        return true;
      }
      break;
    case SurfaceRecolorChoices::Labels :
      if (brainSuiteWindow && brainSuiteWindow->brainSuiteDataManager.vLabel)
      {
        if (SurfaceRecolor::recolorSurfaceWithVolumeLabels(surface,brainSuiteWindow->brainSuiteDataManager.vLabel))
        {
          surface->useSolidColor = false;
          surface->useVertexColor = true;
          return true;
        }
      }
      break;
    case SurfaceRecolorChoices::Hemispheres :
      if (brainSuiteWindow && cerebroGUITool.cerebrumLabeler.vLabeledLobes.size()>0)
      {
        if (SurfaceRecolor::recolorSurfaceWithVolumeLabels(surface, &cerebroGUITool.cerebrumLabeler.vLabeledLobes))
        {
          surface->useSolidColor = false;
          surface->useVertexColor = true;
          return true;
        }
      }
      break;
    case SurfaceRecolorChoices::Intensity :
      {
        if (brainSuiteWindow && brainSuiteWindow->brainSuiteDataManager.volume)
        {
          RangeLUTOp op(brainSuiteWindow->imageLUT,brainSuiteWindow->imageState.brightness.minv,brainSuiteWindow->imageState.brightness.maxv);
          if (SurfaceRecolor::colorSurfaceWithScaledImage(*surface, brainSuiteWindow->brainSuiteDataManager.volume,op))
          {
            surface->useSolidColor = false;
            surface->useVertexColor = true;
            return true;
          }
        }
      }
      break;
    case SurfaceRecolorChoices::Blend :
      if (surfaceBlend(surface,brainSuiteWindow))
      {
        surface->useSolidColor = false;
        surface->useVertexColor = true;
        return true;
      }
      break;
    case SurfaceRecolorChoices::VertexLabels :
      if (SurfaceRecolor::recolorSurfaceWithVertexLabels(surface))
      {
        surface->useSolidColor = false;
        surface->useVertexColor = true;
        return true;
      }
      break;
    case SurfaceRecolorChoices::VertexAttributes :
      if (surface->vertexAttributes.size()==surface->nv())
      {
        if (!brainSuiteWindow) break;
        uint32 *imageLUT=brainSuiteWindow->imageLUT;
        if (!imageLUT) break;
        std::pair<float,float> r=getRange(surface->vertexAttributes);
        if (r.second<=r.first) { r.first=0; r.second=1; }
        std::cout<<"using "<<r.first<<'\t'<<r.second<<std::endl;
        const size_t nv = surface->vertices.size();
        surface->vertexColor.resize(nv);
        DSPoint *c = &surface->vertexColor[0];
        float *va = &surface->vertexAttributes[0];
        for (size_t i=0;i<nv;i++)
        {
          c[i] = rgb2pt(imageLUT[clamp8(255*va[i])]);
        }
        surface->useSolidColor = false;
        surface->useVertexColor = true;
      }
      return true;
      break;
    default: return false;
  }
  return false;
}
