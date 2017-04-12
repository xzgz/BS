// Copyright (C) 2016 The Regents of the University of California
//
// Created by David W. Shattuck, Ph.D.
//
// This file is part of Pialmesh.
//
// Pialmesh is free software; you can redistribute it and/or
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

#include <surfaceexpander.h>
#include <collisionlist.h>
#include <cmath>
#include <DS/timer.h>
#include <vol3d_t.h>
#include <surflib.h>
#include <surface.h>

template<>
bool Vol3D<CollisionData>::copyCast(Vol3DBase *& /*dest*/) const
{
  return false;
}

template<>
bool Vol3D<CollisionData>::write(std::string /*ifile*/)
{
  return false;
}

template<>
bool Vol3D<CollisionData>::maskWith(const Vol3D<uint8> &/*vMask*/)
{
  return false;
}

std::pair<DSPoint,DSPoint> tangentPlane(const DSPoint &n0)
{
  std::pair<DSPoint,DSPoint> plane;
  if (n0.x == 0)
  {
    plane.first = DSPoint(1,0,0);
    plane.second = n0.cross(plane.first).unit();
  }
  else
  {
    plane.first = DSPoint(0,1,0);
    plane.second = n0.cross(plane.first).unit();
  }
  plane.first = n0.cross(plane.second).unit();
  return plane;
}

  /* A -- B
     |    |
     D -- C */
//	const float A0 = v(ip.x  ,ip.y  ,ip.z);
//	const float A1 = v(ip.x  ,ip.y  ,ip.z+1);
template <class T>
float interpolate(const Vol3D<T> &v, const DSPoint &pWC)
{
  const DSPoint pVC(pWC.x/v.rx,pWC.y/v.ry,pWC.z/v.rz);
  IPoint3D ip((int)pVC.x,(int)pVC.y,(int)pVC.z);
  const DSPoint alpha(pVC-ip);
  const float az = alpha.z;
  const float ay = alpha.y;
  const float ax = alpha.x;

  const T *value = v.start() + v.index(ip.x,ip.y,ip.z);
  const auto ss=v.cx*v.cy; // slice size
  const auto cy=v.cx;
  const float A = (1-az) * value[   0] + az * value[ss     ];//v(ip.x  ,ip.y  ,ip.z+1);
  const float B = (1-az) * value[   1] + az * value[ss   +1];//v(ip.x+1,ip.y  ,ip.z+1);
  const float C = (1-az) * value[cy+1] + az * value[ss+cy+1];//v(ip.x+1,ip.y+1,ip.z+1);
  const float D = (1-az) * value[cy]   + az * value[ss+cy  ];//v(ip.x  ,ip.y+1,ip.z+1);
  return
      (1.0f-ay)*((1.0f-ax) * A + ax * B)
      + (ay)*((1.0f-ax) * D + ax * C);
}

inline DSPoint rgb2pt(const uint32 a)
{
  unsigned char *p = (unsigned char *)&a;
  return DSPoint(p[0]/255.0f,p[1]/255.0f,p[2]/255.0f);
}

inline float value(const DSPoint &p, const Vol3D<float> *vTissue)
{
  if (!vTissue) return 2.5f;
  const int x = int((p.x/vTissue->rx)+0.5f);
  const int y = int((p.y/vTissue->ry)+0.5f);
  const int z = int((p.z/vTissue->rz)+0.5f);
  const int cx = vTissue->cx;
  const int cy = vTissue->cy;
  const int cz = vTissue->cz;

  if ((x<0)||(y<0)||(z<0)) return 0;
  if ((x>=cx)||(y>=cy)||(z>=cz)) return 0;
  return vTissue->operator()(x,y,z);
}

inline float value(const DSPoint &p, const Vol3D<float> &vTissue)
{
  const int x = int((p.x/vTissue.rx)+0.5f);
  const int y = int((p.y/vTissue.ry)+0.5f);
  const int z = int((p.z/vTissue.rz)+0.5f);
  const int cx = vTissue.cx;
  const int cy = vTissue.cy;
  const int cz = vTissue.cz;

  if ((x<0)||(y<0)||(z<0)) return 0;
  if ((x>=cx)||(y>=cy)||(z>=cz)) return 0;
  return vTissue(x,y,z);
}

inline bool intersect(std::vector<DSPoint> &verts, const Triangle &t, const int a, const int b)
// test: does vi-vj intersect t?
{
  const DSPoint va = verts[a];
  const DSPoint vb = verts[b];
  const DSPoint v0 = verts[t.a];
  const DSPoint v1 = verts[t.b];
  const DSPoint v2 = verts[t.c];

  const DSPoint e1 = v1-v0;
  const DSPoint e2 = v2-v0;
  const DSPoint tn = e1.cross(e2).unit();

  const float da = (va - v0).dot(tn);
  const float db = (vb - v0).dot(tn);
  if ((da*db)>0.0f) return false;

  const DSPoint x = (da*vb - db*va)/(da-db);
  float test1 = (x-v0).dot((v2-v0).cross(tn));
  float test2 = (x-v2).dot((v1-v2).cross(tn));
  float test3 = (x-v1).dot((v0-v1).cross(tn));
  bool flag = ((test1>0.0f)&&(test2>0.0f)&&(test3>0.0f));
  return flag;
}


inline bool isVertex(const int &vID, const Triangle &tri)
{
  return ((tri.a==vID)||(tri.b==vID)||(tri.c==vID));
}

void SurfaceExpander::step(SILT::Surface &movingSurface, SILT::Surface &initSurface, const Vol3D<float> &volume)
{
  Timer t;
  t.start();
  int hitLimit=0;
  status = "iterating.";
  const size_t nv = movingSurface.nv();

  int first = (firstVertex);
  int last  = (lastVertex);
  if (first<0) first = 0;
  if (last<0) last = (int)(nv-1);
  if (first>=(int)nv) first = (int)(nv-1);
  if (last>=(int)nv) last = (int)(nv-1);
  if (verbosity>2) std::cout<<"expanding "<<first<<" to "<<last<<std::endl;
  if (thickness.size() != nv)
  {
    thickness.resize(nv);
    for (size_t i=0;i<nv;i++) thickness[i] = 0;
  }

  int nChanged = 0;
  int nCollided = 0;
  std::vector<DSPoint> oldPoints(movingSurface.vertices.begin(),movingSurface.vertices.end());
  const float maxd2 = maximumThickness * maximumThickness;
  collisionVolume.verbosity = verbosity;
  collisionVolume.build(movingSurface,searchRadius);
  size_t checkedCount=0;
  size_t possibleCount=0;
  for (int currentVertex=first;currentVertex<=last;currentVertex++)
  {
    if (halt) break;
    DSPoint norm = movingSurface.vertexNormals[currentVertex];
    const DSPoint oldPoint = movingSurface.vertices[currentVertex];
    const float oldTissueValue = ::interpolate(volume,oldPoint);
    float localStepSize = (oldTissueValue-tissueThreshold)/(3.0f-tissueThreshold);
    if (localStepSize<0) localStepSize = 0;
    if (localStepSize>1) localStepSize = 1;
    DSPoint newPoint = oldPoint + stepSize * localStepSize * norm;
    {
      float sideways=0;
      const SILT::Surface::VertexConnection &vc = movingSurface.connectivity[currentVertex];
      if (vc.size())
      {

        std::pair<DSPoint,DSPoint> tplane = tangentPlane(norm);
        if (tanConstant>0)
        {
          float e0=0;
          float e1=0;
          for (size_t nj = 0; (nj < vc.size()); nj++)
          {
            DSPoint radial = (oldPoints[vc[nj]] - oldPoints[currentVertex]);//.unit();
            sideways += norm.dot(radial);
            e0 += tplane.first.dot(radial);
            e1 += tplane.second.dot(radial);
          }
          sideways /= vc.size();
          e0 /= vc.size();
          e1 /= vc.size();
          newPoint += radConstant * sideways * norm;
          newPoint += tanConstant * e0 * tplane.first;
          newPoint += tanConstant * e1 * tplane.second;
        }
        else
        {
          newPoint += radConstant * sideways * norm;
          for (size_t nj = 0; (nj < vc.size()); nj++)
          {
            DSPoint radial = (oldPoints[vc[nj]] - oldPoints[currentVertex]);//.unit();
            sideways += norm.dot(radial);
          }
          sideways /= vc.size();
          newPoint += radConstant * sideways * norm;
        }
      }
    }
    float d2 = (newPoint - initSurface.vertices[currentVertex]).pwr();
    if (d2>maxd2)
    {
      float alpha = sqrtf(d2/maxd2);
      hitLimit++;
      if (alpha>stepSize) alpha = stepSize;
      movingSurface.vertices[currentVertex] = oldPoint + alpha*(oldPoint - newPoint);
    }
    else
    {
      movingSurface.vertices[currentVertex] = newPoint;
      const float newTissueValue = ::interpolate(volume,newPoint);
      if (newTissueValue<tissueThreshold) // check if we are we still in the correct tissue
      {
        movingSurface.vertices[currentVertex] = oldPoint;
      }
    }
    CollisionData collisionData;
    collisionVolume.groupD26(collisionData,movingSurface.vertices[currentVertex]);
    SILT::Surface::VertexConnection &vc = movingSurface.connectivity[currentVertex];
    bool okay = true;
    possibleCount += collisionData.triangleList.size();
    for (std::set<int>::iterator tID=collisionData.triangleList.begin(); tID !=collisionData.triangleList.end(); tID++)
    {
      checkedCount++;
      Triangle &t  = movingSurface.triangles[*tID];
      if (isVertex(currentVertex,t)) continue;
      for (size_t nj = 0; (nj < vc.size()); nj++)
      {
        const int j = vc[nj];
        if (isVertex(j,t)) continue;
        if (intersect(movingSurface.vertices,t,currentVertex,j))
        {
          okay=false;
          break;
        }
      }
      if (!okay) break;
    }
    if (okay)
    {
      nChanged++;
    }
    else
    {
      movingSurface.vertices[currentVertex] = oldPoint;
      nCollided++;
    }
  }
  t.stop();
  if (verbosity>2) std::cout<<"changed "<<nChanged<<'\t'<<"collided "<<nCollided<<'\t'<<t.elapsed()<<std::endl;
  if (verbosity>2) std::cout<<"checked "<<checkedCount<<" and could have checked "<<possibleCount<<std::endl;
}
