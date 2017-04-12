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

#include <odffactory.h>
#include <modshc.h>
#include <numeric>
#include <algorithm>
#include <DS/timer.h>

const float M_PIf = (float)M_PI;

inline void validateRange(int &x, const int maxval)
{
  if (x>=maxval) x = maxval-1;
  if (x<0) x=0;
}

inline void validateRanges(IPoint3D &point, Vol3DBase &vIn)
{
  validateRange(point.x,vIn.cx);
  validateRange(point.y,vIn.cy);
  validateRange(point.z,vIn.cz);
}

ODFFactory::ODFFactory() : constrainRadius(false), nonNegative(true), flipX(false), flipY(false), flipZ(false), nCoefficients(NBasisFunctions), scale(50.0f), normalize(true)
{
}

int ODFFactory::nonzero(Vol3D<uint8> &vMask, IPoint3D &pMin, IPoint3D &pMax)
{
  int nvox = 0;
  for (int z=pMin.z;z<=pMax.z;z++)
  {
    for (int y=pMin.y;y<=pMax.y;y++)
    {
      for (int x=pMin.x;x<=pMax.x;x++)
      {
        if (vMask(x,y,z))
          nvox++;
      }
    }
  }
  return nvox;
}


std::string ODFFactory::colormodelName(ColorModel model) const
{
  switch (model)
  {
    case 0 : return "grey"; break;
    case 1 : return "max direction"; break;
    case 2 : return "FMI"; break;
    case 3 : return "RGB"; break;
    case 4 : return "DS2"; break;
    case 5 : return "R0"; break;
    case 6 : return "R2"; break;
    case 7 : return "RMulti"; break;
  }
  return "unspecified";
}

inline float ODFFactory::samplePhi(const int i, const int NX) const
{
  return (2*M_PIf*i)/(NX-1);
}

inline float ODFFactory::sampleTheta(const int i, const int NX) const
{
  return (  M_PIf*i)/(NX-1);
}

inline DSPoint ODFFactory::sph2cart(const float theta, const float phi, const float R)
{
  const float temp=R*sin(theta);
  return DSPoint(temp*cos(phi),temp*sin(phi),R*cos(theta));
}

void ODFFactory::makeBasis(Surface &odfSurface, const int level, const int NX)
{
  const int nt = (NX-1) * (NX-1) * 2;
  const int nv = NX * NX;
  odfSurface.vertices.resize(nv);
  odfSurface.vertexColor.resize(nv);
  odfSurface.triangles.resize(nt);
  for (int i=0;i<NX;i++)
  {
    for (int j=0;j<NX;j++)
    {
      float phi   = samplePhi(j,NX);
      float theta = sampleTheta(i,NX);
      int idx = i * NX + j;
      odfSurface.vertices[idx] = sph2cart((float)theta,(float)phi,(float)ModSHC<float>::eval(level,theta,phi));
      if (flipX) odfSurface.vertices[idx].x *= -1;
      if (flipY) odfSurface.vertices[idx].y *= -1;
      if (flipZ) odfSurface.vertices[idx].z *= -1;
      odfSurface.vertexColor[idx] = DSPoint((float)(theta/M_PIf),(float)(phi/(2*M_PIf)),1);
    }
  }
  bool parity = false;
  parity ^= flipX;
  parity ^= flipY;
  parity ^= flipZ;
  int tcount=0;
  for (int i=1;i<NX;i++)
    for (int j=1;j<NX;j++)
    {
      int a = NX*(i-1)+j-1;
      int b = NX*(i-1)+j;
      int c = NX*(i  )+j-1;
      int d = NX*(i  )+j;
      if (j==(NX-1)) { b -= (NX-1); d-=(NX-1); }
      if (i==1) { a = 0; b = 0; }
      if (i==(NX-1)) { c = d = NX*i; } // d doesn't matter now!
      if (!parity)
      {
        if (a!=b) odfSurface.triangles[tcount++] = Triangle(a,b,c);
        if (c!=d) odfSurface.triangles[tcount++] = Triangle(b,d,c);
      }
      else
      {
        odfSurface.triangles[tcount++] = Triangle(a,c,b);
        odfSurface.triangles[tcount++] = Triangle(b,c,d);
      }
    }
  odfSurface.triangles.resize(tcount);
  odfSurface.computeNormals();
  {
    sphereBase.create(NX,NX);
    const size_t nv = sphereBase.vertices.size();
    for (int level=0;level<NBasisFunctions;level++)
    {
      sphereValues[level].resize(nv);
      for (size_t i=0;i<nv;i++)
      {
        sphereValues[level][i] = (float)ModSHC<float>::eval(level+1,sphereBase.theta[i],sphereBase.phi[i]);
      }
    }
    {
      for (size_t idx=0;idx<nv;idx++)
      {
        if (flipX) sphereBase.vertices[idx].x *= -1;
        if (flipY) sphereBase.vertices[idx].y *= -1;
        if (flipZ) sphereBase.vertices[idx].z *= -1;
      }
      if (parity)
      {
        const size_t nt = sphereBase.triangles.size();
        for (size_t t=0;t<nt;t++)
          sphereBase.triangles[t] = Triangle(sphereBase.triangles[t].a,sphereBase.triangles[t].c,sphereBase.triangles[t].b);
      }
    }
  }
}

void ODFFactory::makeBasis(const int resX)
{
  for (int j=0;j<NBasisFunctions;j++)
    makeBasis(basisFunctions[j],j+1,resX);
}

void ODFFactory::minmaxODF(Surface &odfSurface, Coefficients &shc)
{
  const size_t nv = sphereBase.vertices.size();
  const size_t nt = sphereBase.triangles.size();
  odfSurface.vertices.resize(nv);
  odfSurface.triangles.resize(nt);
  odfSurface.triangles = sphereBase.triangles;
  size_t maxRid = 0;
  const int jEnd = nCoefficients-1;
  static std::vector<float> radii;
  radii.resize(nv);
  for (size_t v=0;v<nv;v++)
  {
    float R=0;
    for (int j=0;j<=jEnd;j++)
    {
      R += shc[j]*sphereValues[j][v];
    }
    radii[v] = (R>0) ? R : 0;
    if (R>radii[maxRid]) maxRid = v;
  }
  if (normalize)
  {
    std::vector<float>::iterator minRit = std::min_element(radii.begin(),radii.end());
    float maxR = radii[maxRid];
    float minR = *minRit;
    if (maxR<=0) maxR=1;
    if (minR<=0) minR=0;
    float scale = (maxR - minR);
    if (scale==0) { minR=0; scale=maxR; }
    for (size_t v=0;v<nv;v++)
    {
      odfSurface.vertices[v] = ((radii[v]-minR)/scale)*sphereBase.vertices[v];
    }
  }
  else
    for (size_t v=0;v<nv;v++)
      odfSurface.vertices[v] = radii[v]*sphereBase.vertices[v];
  odfSurface.vertexColor.resize(nv);
  odfSurface.computeNormals();
}

void ODFFactory::minmaxODF(Surface &odfSurface, const std::vector<float> &shc)
{
  const size_t nv = sphereBase.vertices.size();
  const size_t nt = sphereBase.triangles.size();
  odfSurface.vertices.resize(nv);
  odfSurface.triangles.resize(nt);
  odfSurface.triangles = sphereBase.triangles;
  size_t maxRid = 0;
  const int jEnd = nCoefficients-1;

  std::vector<float> radii(nv);
  for (size_t v=0;v<nv;v++)
  {
    float R=0;
    for (int j=0;j<=jEnd;j++)
    {
      R += shc[j]*sphereValues[j][v];
    }
    radii[v] = (R>0) ? R : 0;
    if (R>radii[maxRid]) maxRid = v;
  }

  std::vector<float>::iterator minRit = std::min_element(radii.begin(),radii.end());

  float maxR = radii[maxRid];
  float minR = *minRit;
  if (maxR<=0) maxR=1;
  if (minR<=0) minR=0;
  float scale = (maxR - minR);
  if (scale==0) { minR=0; scale=1; }
  for (size_t v=0;v<nv;v++)
  {
    odfSurface.vertices[v] = ((radii[v]-minR)/scale)*sphereBase.vertices[v];
  }
  odfSurface.vertexColor.resize(nv);
  odfSurface.computeNormals();
  switch (colorModel)
  {
    case DS :
    {
      for (size_t v=0;v<nv;v++) odfSurface.vertexColor[v] = abs((odfSurface.vertices[v])).unit();
      break;
    }
    case R0 :
    {
      const size_t jUpper1 = std::min(5,jEnd);
      const size_t jUpper2 = std::min(14,jEnd);
      float all = 0;
      for (size_t j=0;j<=jUpper2;j++) all += fabs(shc[j]);
      float s2 = 0;
      for (size_t j=1;j<=jUpper1; j++) s2 += fabs(shc[j]);
      float sMulti = 0;
      for (size_t j=6;j<=jUpper2;j++) sMulti += fabs(shc[j]);
      const float R0 = (all>0) ? fabs(shc[0])/all : 0;
      DSPoint color((float)R0,(float)R0,(float)R0);
      for (size_t v=0;v<nv;v++)
        odfSurface.vertexColor[v] = color;
      break;
    }
    case R2 :
    {
      const size_t jUpper1 = std::min(5,jEnd);
      const size_t jUpper2 = std::min(14,jEnd);
      float all = 0;
      for (size_t j=0;j<=jUpper2;j++) all += fabs(shc[j]);
      float s2 = 0;
      for (size_t j=1;j<=jUpper1; j++) s2 += fabs(shc[j]);
      const float R2 = (all>0) ? s2/all : 0;
      DSPoint color((float)R2,(float)R2,(float)R2);
      for (size_t v=0;v<nv;v++)
        odfSurface.vertexColor[v] = color;
      break;
    }
    case RMulti :
    {
      const size_t jUpper1 = std::min(5,jEnd);
      const size_t jUpper2 = std::min(14,jEnd);
      float all = 0;
      for (size_t j=0;j<=jUpper2;j++) all += fabs(shc[j]);
      float s2 = 0;
      for (size_t j=1;j<=jUpper1; j++) s2 += fabs(shc[j]);
      float sMulti = 0;
      for (size_t j=6;j<=jUpper2;j++) sMulti += fabs(shc[j]);
      const float Rmulti = (all>0) ? sMulti/all : 0;
      DSPoint color((float)Rmulti,(float)Rmulti,(float)Rmulti);
      for (size_t v=0;v<nv;v++)
        odfSurface.vertexColor[v] = color;
      break;
    }
    case DS2 :
    {
      const size_t jUpper1 = std::min(5,jEnd);
      const size_t jUpper2 = std::min(14,jEnd);
      float all = 0, s2 = 0, sMulti = 0;
      for (size_t j=0;j<=jUpper2;j++) all += fabs(shc[j]);
      for (size_t j=1;j<=jUpper1; j++) s2 += fabs(shc[j]);
      for (size_t j=6;j<=jUpper2;j++) sMulti += fabs(shc[j]);
      DSPoint color;
      if (all>0)
      {
        color.x = (float)(fabs(shc[0])/all);
        color.y = (float)(s2/all);
        color.z = float(sMulti/all);
      }
      for (size_t v=0;v<nv;v++)
        odfSurface.vertexColor[v] = color;
      break;
    }
    case MaxDirection :
    {
      DSPoint color = odfSurface.vertexNormals[maxRid];
      color.x = fabs(color.x);
      color.y = fabs(color.y);
      color.z = fabs(color.z);
      for (size_t v=0;v<nv;v++) odfSurface.vertexColor[v] = color;
      break;
    }
    case FMI :
    {
      const size_t jUpper1 = std::min(5,jEnd);
      const size_t jUpper2 = std::min(14,jEnd);
      float d = 0;
      for (size_t j=1;j<=jUpper1;j++) d += shc[j]*shc[j];
      float n = 0;
      for (size_t j=6;j<=jUpper2;j++) n += shc[j]*shc[j];
      float FMI = (d>0) ? n / d : 0;
      DSPoint color((float)FMI,(float)FMI,(float)FMI);
      for (size_t v=0;v<nv;v++)
        odfSurface.vertexColor[v] =  color;
      break;
    }
    case Grey :
    default:
    {
      for (size_t v=0;v<nv;v++)
        odfSurface.vertexColor[v] =  DSPoint(0.7f,0.7f,0.7f);
    }
  }
}

void ODFFactory::newODF(Surface &odfSurface, DSPoint position, const std::vector<float> &shc)
{
  const size_t nv = sphereBase.vertices.size();
  const size_t nt = sphereBase.triangles.size();
  odfSurface.vertices.resize(nv);
  odfSurface.triangles.resize(nt);
  odfSurface.triangles = sphereBase.triangles;
  size_t maxRid = 0;
  const int jEnd = nCoefficients-1;
  if (normalize)
  {
    std::vector<float> radii(nv);
    for (size_t v=0;v<nv;v++)
    {
      float R=0;
      for (int j=0;j<=jEnd;j++)
      {
        R += shc[j]*sphereValues[j][v];
      }
      radii[v] = (R>0) ? R : 0;
    }
    std::vector<float>::iterator  minRit = std::min_element(radii.begin(),radii.end());
    std::vector<float>::iterator  maxRit = std::max_element(radii.begin(),radii.end());
    float maxR = *maxRit;
    float minR = *minRit;
    if (maxR<=0) maxR=1;
    if (minR<=0) minR=0;
    float scale = (maxR - minR);
    for (size_t v=0;v<nv;v++)
    {
      odfSurface.vertices[v] = position + ((radii[v]-minR)/scale)*sphereBase.vertices[v];
    }
  }
  else
  {
    for (size_t v=0;v<nv;v++)
    {
      float R=0;
      for (int j=0;j<=jEnd;j++)
      {
        R += shc[j]*sphereValues[j][v];
      }
      if (!((R<0)&&nonNegative))
        odfSurface.vertices[v] = position + ((R<0)&&nonNegative ? 0 : float(scale * R) * sphereBase.vertices[v]);
      else
        odfSurface.vertices[v] = position;
    }
  }
  odfSurface.vertexColor.resize(nv);
  odfSurface.computeNormals();

  const size_t jUpper1 = std::min(5,jEnd);
  const size_t jUpper2 = std::min(14,jEnd);
  colorModel=(normalize) ? DS : R2;
  switch (colorModel)
  {
    case DS :
    {
      for (size_t v=0;v<nv;v++) odfSurface.vertexColor[v] = abs((odfSurface.vertices[v]-position)).unit();
      break;
    }
    case R0 :
    {
      float all = 0;
      for (size_t j=0;j<=jUpper2;j++) all += fabs(shc[j]);
      float s2 = 0;
      for (size_t j=1;j<=jUpper1; j++) s2 += fabs(shc[j]);
      float sMulti = 0;
      for (size_t j=6;j<=jUpper2;j++) sMulti += fabs(shc[j]);
      const float R0 = (all>0) ? fabs(shc[0])/all : 0;
      DSPoint color((float)R0,(float)R0,(float)R0);
      for (size_t v=0;v<nv;v++)
        odfSurface.vertexColor[v] = color;
      break;
    }
    case R2 :
    {
      float all = 0;
      for (size_t j=0;j<=jUpper2;j++) all += fabs(shc[j]);
      float s2 = 0;
      for (size_t j=1;j<=jUpper1; j++) s2 += fabs(shc[j]);
      float sMulti = 0;
      for (size_t j=6;j<=jUpper2;j++) sMulti += fabs(shc[j]);
      const float R2 = (all>0) ? s2/all : 0;
      DSPoint color((float)R2,(float)R2,(float)R2);
      for (size_t v=0;v<nv;v++)
        odfSurface.vertexColor[v] = color;
      break;
    }
    case RMulti :
    {
      float all = 0;
      for (size_t j=0;j<=jUpper2;j++) all += fabs(shc[j]);
      float s2 = 0;
      for (size_t j=1;j<=jUpper1; j++) s2 += fabs(shc[j]);
      float sMulti = 0;
      for (size_t j=6;j<=jUpper2;j++) sMulti += fabs(shc[j]);
      const float Rmulti = (all>0) ? sMulti/all : 0;
      DSPoint color((float)Rmulti,(float)Rmulti,(float)Rmulti);
      for (size_t v=0;v<nv;v++)
        odfSurface.vertexColor[v] = color;
      break;
    }
    case DS2 :
    {
      float all = 0, s2 = 0, sMulti = 0;
      for (size_t j=0;j<=jUpper2;j++) all += fabs(shc[j]);
      for (size_t j=1;j<=jUpper1; j++) s2 += fabs(shc[j]);
      for (size_t j=6;j<=jUpper2;j++) sMulti += fabs(shc[j]);
      DSPoint color;
      if (all>0)
      {
        color.x = (float)(fabs(shc[0])/all);
        color.y = (float)(s2/all);
        color.z = float(sMulti/all);
      }
      for (size_t v=0;v<nv;v++)
        odfSurface.vertexColor[v] = color;
      break;
    }
    case MaxDirection :
    {
      DSPoint color = odfSurface.vertexNormals[maxRid];
      color.x = fabs(color.x);
      color.y = fabs(color.y);
      color.z = fabs(color.z);
      for (size_t v=0;v<nv;v++) odfSurface.vertexColor[v] = color;
      break;
    }
    case FMI :
    {
      float d = 0;
      for (size_t j=1;j<=jUpper1;j++) d += shc[j]*shc[j];
      float n = 0;
      for (size_t j=6;j<=jUpper2;j++) n += shc[j]*shc[j];
      float FMI = (d>0) ? n / d : 0;
      DSPoint color((float)FMI,(float)FMI,(float)FMI);
      for (size_t v=0;v<nv;v++)
        odfSurface.vertexColor[v] =  color;
      break;
    }
    case Grey :
    default:
    {
      for (size_t v=0;v<nv;v++)
        odfSurface.vertexColor[v] =  DSPoint(0.7f,0.7f,0.7f);
    }
  }
}

void ODFFactory::makeGlyphs(Surface &surface, Vol3D<float32> vIn[], Vol3D<uint8> &vMask, IPoint3D p0, IPoint3D p1, float base)
{
  if (!vMask.isCompatible(vIn[0]))
  {
    makeGlyphs(surface, vIn, p0, p1, base);
    return;
  }
  std::ostringstream ostr;
  DSPoint minColor;
  DSPoint maxColor;
  validateRanges(p0,vIn[0]);
  validateRanges(p1,vIn[0]);
  int nvox = nonzero(vMask,p0,p1);

  const size_t glyphNV = sphereBase.vertices.size();
  const size_t glyphNT = sphereBase.triangles.size();
  ostr<<"Creating "<<nvox<<" voxels with "<<glyphNV<<" verts and "<<glyphNT<<" triangles."<<std::endl;

  surface.vertices.resize(nvox*glyphNV);
  surface.triangles.resize(nvox*glyphNT);
  surface.vertexColor.resize(surface.nv());
  surface.vertexNormals.resize(surface.nv());
  Timer t2; t2.start();
  size_t tcount=0;
  size_t vcount=0;
  size_t voxcount=0;
  Surface s;
  const float rx=vIn[0].rx;
  const float ry=vIn[0].ry;
  const float rz=vIn[0].rz;
  int look=0;
  const int jEnd = nCoefficients-1;
  for (int z=p0.z;z<=p1.z;z++)
  {
    for (int y=p0.y;y<=p1.y;y++)
    {
      for (int x=p0.x;x<=p1.x;x++)
      {
        look++;
        if (!vMask(x,y,z)) continue;
        IPoint3D ip(x,y,z);
        std::vector<float> shc(jEnd+1,0);
        for (int j=0;j<=jEnd;j++) shc[j] = vIn[j](ip);
        shc[0] += base;

        newODF(s,DSPoint(ip.x*rx,ip.y*ry,ip.z*rz),shc);
        if (tcount + s.nt() > surface.triangles.size())
        {
          ostr<<"Error: have too many triangles: "<<tcount<<" "<<surface.triangles.size()<<", trying to add "<<s.nt()<<std::endl;
          break;
        }
        for (size_t t=0;t<s.nt();t++) surface.triangles[tcount++] = Triangle(s.triangles[t].a+(int)vcount,s.triangles[t].b+(int)vcount,s.triangles[t].c+(int)vcount);
        for (size_t v=0;v<s.nv();v++) surface.vertices[vcount+v] = s.vertices[v];
        if (s.vertexColor.size()==s.nv())
        {
          for (size_t v=0;v<s.nv();v++) surface.vertexColor[vcount+v] = s.vertexColor[v];
          if (voxcount==0)
          {
            minColor = s.vertexColor[0];
            maxColor = s.vertexColor[0];
          }
          else
          {
            if (s.vertexColor[0].x<minColor.x) minColor.x = s.vertexColor[0].x;
            if (s.vertexColor[0].y<minColor.y) minColor.y = s.vertexColor[0].y;
            if (s.vertexColor[0].z<minColor.z) minColor.z = s.vertexColor[0].z;
            if (s.vertexColor[0].x>maxColor.x) maxColor.x = s.vertexColor[0].x;
            if (s.vertexColor[0].y>maxColor.y) maxColor.y = s.vertexColor[0].y;
            if (s.vertexColor[0].z>maxColor.z) maxColor.z = s.vertexColor[0].z;
          }
        }
        else
        {
          ostr<<"Error in color size: "<<s.vertexColor.size()<<'\t'<<s.nv()<<std::endl;
        }
        if (s.vertexNormals.size()==s.nv())
        {
          for (size_t v=0;v<s.nv();v++) surface.vertexNormals[vcount+v] = s.vertexNormals[v];
        }
        vcount += s.nv();
        voxcount++;
      }
    }
  }
  ostr<<"made "<<voxcount<<", expected "<<nvox<<", looked at "<<look<<std::endl;
  surface.vertices.resize(vcount);
  surface.vertexColor.resize(vcount);
  surface.triangles.resize(tcount);
  t2.stop(); ostr<<"making glyphs took "<<t2.elapsed()<<std::endl;
  switch (colorModel)
  {
    case ODFFactory::R0:
    case ODFFactory::R2:
    case ODFFactory::RMulti:
    case ODFFactory::FMI:
    case ODFFactory::DS2:
    {
      if (maxColor.x<=0) maxColor.x = 1;
      if (maxColor.y<=0) maxColor.y = 1;
      if (maxColor.z<=0) maxColor.z = 1;
      for (size_t i=0;i<surface.nv();i++)
      {
        surface.vertexColor[i].x /= maxColor.x;
        surface.vertexColor[i].y /= maxColor.y;
        surface.vertexColor[i].z /= maxColor.z;
      }
    }
    default:
      break;
  }
  ostr<<minColor<<" : "<<maxColor<<std::endl;
  message = ostr.str();
}

void ODFFactory::makeGlyphs(Surface &surface, Vol3D<float32> vIn[], IPoint3D p0, IPoint3D p1, float base)
{
  std::ostringstream ostr;
  DSPoint minColor;
  DSPoint maxColor;
  validateRanges(p0,vIn[0]);
  validateRanges(p1,vIn[0]);

  // could compute more simply, but leaving here in case it returns to only rendering non-zero ones
  int nvox = 0;
  for (int z=p0.z;z<=p1.z;z++)
  {
    for (int y=p0.y;y<=p1.y;y++)
    {
      for (int x=p0.x;x<=p1.x;x++)
      {
        //				if (vIn[0](x,y,z))
        nvox++;
      }
    }
  }
  const size_t glyphNV = sphereBase.vertices.size();
  const size_t glyphNT = sphereBase.triangles.size();

  surface.vertices.resize(nvox*glyphNV);
  surface.triangles.resize(nvox*glyphNT);
  surface.vertexColor.resize(surface.nv());
  surface.vertexNormals.resize(surface.nv());
  Timer t2; t2.start();
  size_t tcount=0;
  size_t vcount=0;
  size_t voxcount=0;
  Surface s;
  const float rx=vIn[0].rx;
  const float ry=vIn[0].ry;
  const float rz=vIn[0].rz;
  int look=0;
  const int jEnd = nCoefficients-1;
  for (int z=p0.z;z<=p1.z;z++)
  {
    for (int y=p0.y;y<=p1.y;y++)
    {
      for (int x=p0.x;x<=p1.x;x++)
      {
        look++;
        IPoint3D ip(x,y,z);
        std::vector<float> shc(jEnd+1,0);
        for (int j=0;j<=jEnd;j++) shc[j] = vIn[j](ip);
        shc[0] += base;

        newODF(s,DSPoint(ip.x*rx,ip.y*ry,ip.z*rz),shc);
        if (tcount + s.nt() > surface.triangles.size())
        {
          ostr<<"Error: have too many triangles: "<<tcount<<" "<<surface.triangles.size()<<", trying to add "<<s.nt()<<std::endl;
          break;
        }
        for (size_t t=0;t<s.nt();t++) surface.triangles[tcount++] = Triangle(s.triangles[t].a+(int)vcount,s.triangles[t].b+(int)vcount,s.triangles[t].c+(int)vcount);
        for (size_t v=0;v<s.nv();v++) surface.vertices[vcount+v] = s.vertices[v];
        if (s.vertexColor.size()==s.nv())
        {
          for (size_t v=0;v<s.nv();v++) surface.vertexColor[vcount+v] = s.vertexColor[v];
          if (voxcount==0)
          {
            minColor = s.vertexColor[0];
            maxColor = s.vertexColor[0];
          }
          else
          {
            if (s.vertexColor[0].x<minColor.x) minColor.x = s.vertexColor[0].x;
            if (s.vertexColor[0].y<minColor.y) minColor.y = s.vertexColor[0].y;
            if (s.vertexColor[0].z<minColor.z) minColor.z = s.vertexColor[0].z;
            if (s.vertexColor[0].x>maxColor.x) maxColor.x = s.vertexColor[0].x;
            if (s.vertexColor[0].y>maxColor.y) maxColor.y = s.vertexColor[0].y;
            if (s.vertexColor[0].z>maxColor.z) maxColor.z = s.vertexColor[0].z;
          }
        }
        else
        {
          ostr<<"Error in color size: "<<s.vertexColor.size()<<'\t'<<s.nv()<<std::endl;
        }
        if (s.vertexNormals.size()==s.nv())
        {
          for (size_t v=0;v<s.nv();v++) surface.vertexNormals[vcount+v] = s.vertexNormals[v];
        }
        vcount += s.nv();
        voxcount++;
      }
    }
  }
  surface.vertices.resize(vcount);
  surface.vertexColor.resize(vcount);
  surface.triangles.resize(tcount);
  switch (colorModel)
  {
    case ODFFactory::R0:
    case ODFFactory::R2:
    case ODFFactory::RMulti:
    case ODFFactory::FMI:
    case ODFFactory::DS2:
      {
        if (maxColor.x<=0) maxColor.x = 1;
        if (maxColor.y<=0) maxColor.y = 1;
        if (maxColor.z<=0) maxColor.z = 1;
        for (size_t i=0;i<surface.nv();i++)
        {
          surface.vertexColor[i].x /= maxColor.x;
          surface.vertexColor[i].y /= maxColor.y;
          surface.vertexColor[i].z /= maxColor.z;
        }
      }
      break;
    default:
      break;
  }
  message = ostr.str();
}

