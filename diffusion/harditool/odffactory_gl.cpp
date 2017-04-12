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
#include <DS/timer.h>
#include <surfaceinstances.h>
// includes odffactory code that is specific to the Graphics version

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

int nonzero(Vol3D<uint8> &vMask, IPoint3D &pMin, IPoint3D &pMax);

inline void copyFromSurface(SurfaceInstance &instance, Surface &s)
{
  instance.vertex = s.vertices;
  instance.vertexNormal = s.vertexNormals;
  instance.vertexColor = s.vertexColor;
}

inline void copyFromSurfaceNoColor(SurfaceInstance &instance, Surface &s)
{
  instance.vertex = s.vertices;
  instance.vertexNormal = s.vertexNormals;
}

void ODFFactory::makeGlyph(SurfaceInstance &instance, Vol3D<Coefficients> &vSphericalHarmonicCoefficients, const IPoint3D &voxel)
{
  static SILT::Surface surface;
  minmaxODF(surface,vSphericalHarmonicCoefficients(voxel));
  copyFromSurfaceNoColor(instance,surface);
  const float rx=vSphericalHarmonicCoefficients.rx;
  const float ry=vSphericalHarmonicCoefficients.ry;
  const float rz=vSphericalHarmonicCoefficients.rz;
  instance.position = DSPoint(voxel.x*rx,voxel.y*ry,voxel.z*rz);
}


void ODFFactory::makeGlyphs(SurfaceInstanceSet &glyphCollection, Vol3D<Coefficients> &vSphericalHarmonicCoefficients, IPoint3D p0, IPoint3D p1)
{
  validateRanges(p0,vSphericalHarmonicCoefficients);
  validateRanges(p1,vSphericalHarmonicCoefficients);
  int nvox = (p1.x-p0.x+1)*(p1.y-p0.y+1)*(p1.z-p0.z+1);
  glyphCollection.initialize(sphereBase.triangles,sphereBase.colors,nvox);
  size_t voxcount=0;
  for (int z=p0.z;z<=p1.z;z++)  // TODO: multi-thread this
  {
    for (int y=p0.y;y<=p1.y;y++)
    {
      for (int x=p0.x;x<=p1.x;x++)
      {
        makeGlyph(glyphCollection.instances[voxcount++],vSphericalHarmonicCoefficients,IPoint3D(x,y,z));
      }
    }
  }
}


void ODFFactory::makeGlyphs(SurfaceInstanceSet &glyphCollection, Vol3D<Coefficients> &vSphericalHarmonicCoefficients, Vol3D<uint8> &vMask, IPoint3D p0, IPoint3D p1)
{
  if (!vMask.isCompatible(vSphericalHarmonicCoefficients))
  {
    makeGlyphs(glyphCollection, vSphericalHarmonicCoefficients, p0, p1);
    return;
  }
  validateRanges(p0,vSphericalHarmonicCoefficients);
  validateRanges(p1,vSphericalHarmonicCoefficients);
  int nvox = nonzero(vMask,p0,p1);
  glyphCollection.initialize(sphereBase.triangles,sphereBase.colors,nvox);
  size_t voxcount=0;
  for (int z=p0.z;z<=p1.z;z++)
  {
    for (int y=p0.y;y<=p1.y;y++)
    {
      for (int x=p0.x;x<=p1.x;x++)
      {
        if (vMask(x,y,z))
        {
          makeGlyph(glyphCollection.instances[voxcount++],vSphericalHarmonicCoefficients,IPoint3D(x,y,z));
        }
      }
    }
  }
}

void ODFFactory::makeGlyph(SurfaceInstance &instance, Vol3D<float32> vSphericalHarmonicCoefficients[], const IPoint3D &voxel)
{
  const int jStart=0;
  const int jEnd = nCoefficients-1;
  std::vector<float> shc(jEnd+1,0);
  for (int j=jStart;j<=jEnd;j++) shc[j] = vSphericalHarmonicCoefficients[j](voxel);
  SILT::Surface surface;
  minmaxODF(surface,shc);
  copyFromSurface(instance,surface);
  const float rx=vSphericalHarmonicCoefficients[0].rx;
  const float ry=vSphericalHarmonicCoefficients[0].ry;
  const float rz=vSphericalHarmonicCoefficients[0].rz;
  instance.position = DSPoint(voxel.x*rx,voxel.y*ry,voxel.z*rz);
}


void ODFFactory::makeGlyphs(SurfaceInstanceSet &glyphCollection, Vol3D<float32> vIn[], Vol3D<uint8> &vMask, IPoint3D p0, IPoint3D p1)
{
  if (!vMask.isCompatible(vIn[0]))
  {
    makeGlyphs(glyphCollection, vIn, p0, p1);
    return;
  }
  validateRanges(p0,vIn[0]);
  validateRanges(p1,vIn[0]);
  int nvox = nonzero(vMask,p0,p1);
  glyphCollection.initialize(sphereBase.triangles,sphereBase.colors,nvox);

  size_t voxcount=0;
  for (int z=p0.z;z<=p1.z;z++)
  {
    for (int y=p0.y;y<=p1.y;y++)
    {
      for (int x=p0.x;x<=p1.x;x++)
      {
        if (vMask(x,y,z))
          makeGlyph(glyphCollection.instances[voxcount++],vIn,IPoint3D(x,y,z));
      }
    }
  }
}

void ODFFactory::makeGlyphs(SurfaceInstanceSet &glyphCollection, Vol3D<float32> vIn[], IPoint3D p0, IPoint3D p1)
{
  DSPoint minColor;
  DSPoint maxColor;
  validateRanges(p0,vIn[0]);
  validateRanges(p1,vIn[0]);

  int nvox = (p1.x-p0.x+1)*(p1.y-p0.y+1)*(p1.z-p0.z+1);
  glyphCollection.initialize(sphereBase.triangles,sphereBase.colors,nvox);

  Timer t2; t2.start();
  size_t voxcount=0;
  for (int z=p0.z;z<=p1.z;z++)
  {
    for (int y=p0.y;y<=p1.y;y++)
    {
      for (int x=p0.x;x<=p1.x;x++)
      {
        makeGlyph(glyphCollection.instances[voxcount++],vIn,IPoint3D(x,y,z));
      }
    }
  }
}

