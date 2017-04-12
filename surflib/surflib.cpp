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

#include <surflib.h>
#include <surface.h>
#include <graph.h>
#include <list>

bool SurfLib::verbose = false;

void SurfLib::revtri(Surface &s)
{
  const size_t nt = s.nt();
  Triangle *t = &s.triangles[0];
  for (size_t i=0;i<nt;i++)
  {
    std::swap(t[i].b,t[i].c);
  }
}

void SurfLib::computeCurvature(Surface &surface) // really the convexity!
{
  const size_t nv = surface.vertices.size();
  if (surface.connectivity.size()!=nv) surface.computeConnectivity();
  if (surface.vertexNormals.size()!=nv) surface.computeNormals();
  surface.meanCurvature.resize(nv);
  float *cnv = &surface.meanCurvature[0];
  DSPoint *vertexNormal = &surface.vertexNormals[0];
  DSPoint *v = &surface.vertices[0];
  for (size_t n=0;n<nv;n++)
  {
    Surface::VertexConnection &c = surface.connectivity[n];
    const int nc = c.size();
    if (nc==0) { cnv[n]=0; continue; }
    DSPoint norm = vertexNormal[n];
    DSPoint vert = v[n];
    float f = 0.0;
    for (int j=0;j<nc;j++)
    {
      float f0 = dot(norm,(vert-v[surface.connectivity[n][j]]).unit() );
      f += f0;
    }
    float cv = f/nc;
    cnv[n] = cv;
  }
}

int SurfLib::segmentSurface(std::vector<int> &surfaceLabels, const Surface &surface)
{
  if (surface.vertexLabels.size() != surface.nv())
  {
    std::cerr<<"no labels on surface!"<<std::endl;
    return 0;
  }
  surfaceLabels.resize(surface.nv());
  const size_t nv = surface.nv();
  const size_t nt = surface.nt();
  Graph graph((int)nv);
  graph.reset((int)nv);
  int nLinks = 0;
  int nBroken = 0;
  for (size_t i=0;i<nt;i++)
  {
    Triangle t = surface.triangles[i];
    const int a = t.a;
    const int b = t.b;
    const int c = t.c;
    if (surface.vertexLabels[a]==surface.vertexLabels[b])
    {
      graph.link(a,b);
      nLinks++;
    }
    else nBroken++;
    if (surface.vertexLabels[b]==surface.vertexLabels[c])
    {
      graph.link(b,c);
      nLinks++;
    }
    else nBroken++;
    if (surface.vertexLabels[c]==surface.vertexLabels[a])
    {
      graph.link(c,a);
      nLinks++;
    }
    else nBroken++;
  }
  return graph.makemap(&surfaceLabels[0]);
}

void SurfLib::fastFill(Surface &surface, bool verbose)
{
  const size_t nv = surface.nv();
  std::vector<uint16> labels;
  labels.resize(nv);
  for (size_t i=0;i<nv;i++)
    labels[i] = surface.vertexLabels[i];
  surface.computeConnectivity();
  uint16 *oldLabels = &surface.vertexLabels[0];
  for (;;)
  {
    int changed = 0;
    for (size_t i=0;i<nv;i++)
    {
      if (oldLabels[i]==0)
      {
        Surface::VertexConnection &con = surface.connectivity[i];
        int closest = -1;
        float dist = 0;
        for (size_t j=0;j<con.size();j++)
        {
          const int id = con[j];
          if (oldLabels[id] && (oldLabels[id]!=255))
          {
            float d2 = (surface.vertices[i] - surface.vertices[j]).pwr();
            if ((closest<0)||(d2 < dist))
            {
              dist = d2;
              closest = id;
            }
          }
        }
        if (closest>=0)
        {
          changed++;
          labels[i] = oldLabels[closest];
        }
      }
    }
    if (verbose) std::cout<<"changed "<<changed<<" labels."<<std::endl;
    if (changed<=0) break;
    for (size_t i=0;i<nv;i++)
    {
      oldLabels[i] = labels[i];
    }
  }
}

void SurfLib::smoothCurvature(Surface &surf, const float alpha)
{
  if (surf.meanCurvature.size()!=surf.nv())
  {
    computeCurvature(surf);
  }
  std::vector<float> newCurv(surf.meanCurvature.size());
  const size_t nv = surf.nv();
  const float *mc = &surf.meanCurvature[0];
  for (size_t i=0;i<nv;i++)
  {
    newCurv[i]=surf.meanCurvature[i];
    Surface::VertexConnection &connectivity = surf.connectivity[i];
    const int nc = connectivity.size();
    if (nc==0) continue;
    float sum = 0.0f;
    for (int index=0;index<nc;index++)
    {
      int j = connectivity[index];
      sum += mc[j];
    }
    newCurv[i] = (alpha * surf.meanCurvature[i]) + (1-alpha) * sum / nc;
  }
  for (size_t i=0;i<nv;i++) surf.meanCurvature[i] = newCurv[i];
}


void SurfLib::dilate(Surface &surface, std::vector<uint8> &v)
{
  std::vector<uint8> v2;
  const size_t nv = v.size();
  v2.resize(nv);
  for (size_t i=0;i<nv;i++) v2[i] = v[i];
  surface.computeConnectivity();
  for (size_t i=0;i<nv;i++)
  {
    if (v[i])
    {
      Surface::VertexConnection &con = surface.connectivity[(int)i];
      const int nc = con.size();
      for (int r=0;r<nc;r++)
      {
        int j = con[r];
        v2[j] = 1;
      }
    }
  }
  for (size_t i=0;i<nv;i++)
    v[i] = v2[i];
}

void SurfLib::erode(Surface &surface, std::vector<uint8> &v)
{
  std::vector<uint8> v2;
  const size_t nv = v.size();
  v2.resize(nv);
  for (size_t i=0;i<nv;i++) v2[i] = v[i];
  surface.computeConnectivity();
  for (size_t i=0;i<nv;i++)
  {
    if (!v[i])
    {
      Surface::VertexConnection &con = surface.connectivity[(int)i];
      const int nc = con.size();
      for (int r=0;r<nc;r++)
      {
        int j = con[r];
        v2[j] = 0;
      }
    }
  }
  for (size_t i=0;i<nv;i++)
    v[i] = v2[i];
}


typedef std::pair<uint16,uint16> Edge;

class BoundaryList {
public:
  void join(std::list<SurfLib::EdgeList>::iterator iterA, std::list<SurfLib::EdgeList>::iterator iterB)
  {
    if (iterA->back()==iterB->front())
    {
      iterB->pop_front();
      while (iterB->size()>0)
      {
        iterA->push_back(iterB->front());
        iterB->pop_front();
      }
    }
  }
  void insert(int a, int b)
  {
    for (std::list<SurfLib::EdgeList>::iterator iter = edgeLists.begin(); iter != edgeLists.end(); iter++)
    {
      if (iter->size()==0) continue;
      if (iter->front()==a)
      {
        iter->push_front(b); // join b -> b
        for (std::list<SurfLib::EdgeList>::iterator iter2 = edgeLists.begin(); iter2 != edgeLists.end(); iter2++)
        {
          if (iter2==iter) continue; // ignore circular lists.
          if (iter2->back()==b)
          {
            join(iter2,iter);
          }
        }
        return;
      }
      if (iter->front()==b)
      {
        iter->push_front(a); // join a -> a
        for (std::list<SurfLib::EdgeList>::iterator iter2 = edgeLists.begin(); iter2 != edgeLists.end(); iter2++)
        {
          if (iter2==iter) continue; // ignore circular lists.
          if (iter2->back()==a)
          {
            join(iter2,iter);
          }
        }
        break;
      }
      if (iter->back()==a)
      {
        iter->push_back(b); // join b -> b
        for (std::list<SurfLib::EdgeList>::iterator iter2 = edgeLists.begin(); iter2 != edgeLists.end(); iter2++)
        {
          if (iter2==iter) continue; // don't care about circular lists.
          if (iter2->front()==b)
          {
            join(iter,iter2);
          }
        }
        break;
      }
      if (iter->front()==b)
      {
        iter->push_back(a); // join a -> a
        for (std::list<SurfLib::EdgeList>::iterator iter2 = edgeLists.begin(); iter2 != edgeLists.end(); iter2++)
        {
          if (iter2==iter) continue; // ignore circular lists.
          if (iter2->front()==a)
          {
            join(iter,iter2);
          }
        }
        break;
      }
    }
    edgeLists.push_back(SurfLib::EdgeList());
    edgeLists.front().push_back(a);
    edgeLists.front().push_back(b);
  }
  std::list<SurfLib::EdgeList> edgeLists;
};


bool SurfLib::findBorders(std::vector<uint16> &boundaryPoints, Surface &surface, const int structure)
{
  BoundaryList edgeMap;
  surface.computeConnectivity();
  const size_t nv = surface.nv();
  if (surface.vertexLabels.size() != nv) return false;

  boundaryPoints.resize(nv);
  uint16 *labels = &surface.vertexLabels[0];

  for (size_t i=0;i<nv;i++)
  {
    boundaryPoints[i] = 0;
    int diffcount = 0;
    Surface::VertexConnection &con = surface.connectivity[i];
    int label = labels[i];
    if (label!=structure) continue;
    for (size_t index=0;index<con.size();index++)
    {
      const int j = con[index];
      if (labels[j] != label)
      {
        diffcount++;
      }
    }
    if (diffcount>0) boundaryPoints[i] = 1;
  }
  return true;
}


bool SurfLib::sortBorders(std::list< std::list<int> >&borders, std::vector<uint16> &boundaryPoints, Surface &surface)							
{
  BoundaryList edgeMap;
  const size_t nv = surface.nv();
  if (boundaryPoints.size() != nv)
  {
    std::cerr<<"boundary and surface have different number of vertices."<<std::endl;
    return false;
  }
  for (size_t i=0;i<nv;i++)
  {
    if (boundaryPoints[i])
    {
      Surface::VertexConnection &con = surface.connectivity[i];
      for (size_t index=0;index<con.size();index++)
      {
        const int j = con[index];
        if (boundaryPoints[j])
        {
          edgeMap.insert(i,j);
        }
      }
    }
  }
  borders.clear();
  for (std::list<SurfLib::EdgeList>::iterator iter = edgeMap.edgeLists.begin(); iter != edgeMap.edgeLists.end(); iter++)
  {
    if (iter->size()==0) continue;
    std::list<int> blanklist;
    borders.push_back(blanklist);
    for (std::list<int>::iterator it2 = iter->begin(); it2 != iter->end(); it2++)
    {
      borders.back().push_back(*it2);
    }
  }
  return true;
}

void SurfLib::laplaceSmooth(Surface &surface, const int n, const float alpha)
{
  const size_t nv = surface.vertices.size();
  if (surface.connectivity.size()!=nv) surface.computeConnectivity();
  Surface::VertexConnection *connex = &surface.connectivity[0];
  DSPoint *point = &surface.vertices[0];
  DSPoint *dst = point;
  const float a = alpha;
  const float b = 1-a;

  for (int k=0;k<n;k++)
  {
    for (size_t i=0;i<nv;i++)
    {
      Surface::VertexConnection &c = connex[i];
      const int N = c.size();
      DSPoint d(0,0,0);
      for (int j=0;j<N;j++)
      {
        d += point[c[j]];
      }
      dst[i] = b * point[i] + (a/N) * d;
    }
  }
}

void SurfLib::laplaceSmoothCurvatureWeighted(Surface &s, int niter, float alpha, float c)
{
  const size_t nv = s.nv();
  DSPoint *verts = &s.vertices[0];
  SurfLib::computeCurvature(s);
  for (int n=0;n<niter;n++)
  {
    for (size_t i=0;i<nv;i++)
    {
      Surface::VertexConnection &conn = s.connectivity[i];
      DSPoint dP(0,0,0);
      const int N = conn.size();
      for (int jID=0;jID<N;jID++)
      {
        const int j=conn[jID];
        dP += verts[j];
      }
      const float m = c*fabs(s.meanCurvature[i]);
      float am = alpha * m;
      if (am>1) am = 1;
      const float b = 1 - am;
      verts[i] = b * verts[i] + (am/N) * dP;
    }
    SurfLib::computeCurvature(s);
  }
}

class SurfacePatchInfo {
public:
  SurfacePatchInfo() : nv(0), originalLabel(-1), segmentID(-1) {}
  int nv; // size (count)
  int originalLabel; // anatomical label
  int segmentID;		 // ID in the table
};

void SurfLib::selectLargestPatches(Surface &surface)
{
  std::vector<int> surfaceLabels;
  SurfLib::segmentSurface(surfaceLabels,surface);
  size_t mx=0;
  for (size_t i=1;i<surfaceLabels.size();i++)
    if (surfaceLabels[i]>surfaceLabels[mx]) mx = i;
  int maxLabel = surfaceLabels[mx];
  if (verbose) std::cout<<maxLabel<<std::endl;
  std::vector<SurfacePatchInfo> bestLabels(256); //
  std::vector<SurfacePatchInfo> surfacePatchInfo(maxLabel+1);
  for (size_t i=0;i<surface.nv();i++)
  {
    surfacePatchInfo[surfaceLabels[i]].originalLabel = surface.vertexLabels[i];
    surfacePatchInfo[surfaceLabels[i]].segmentID = surfaceLabels[i];
    surfacePatchInfo[surfaceLabels[i]].nv++;
  }
  for (int i=0;i<=maxLabel;i++)
  {
    if (verbose) std::cout<<i<<" : "<<surfacePatchInfo[i].nv<<" ( "<<surfacePatchInfo[i].originalLabel<<" )"<<std::endl;
    int OL = surfacePatchInfo[i].originalLabel;
    if (bestLabels[OL].nv < surfacePatchInfo[i].nv)
      bestLabels[OL] = surfacePatchInfo[i];
  }
  if (verbose)
    for (int i=0;i<256;i++)
    {
      if (bestLabels[i].nv>0)
        std::cout<<i<<" : "<<bestLabels[i].nv<<" ( "<<bestLabels[i].segmentID<<" )"<<std::endl;
    }
  for (size_t i=0;i<surface.nv();i++)
  {
    if (bestLabels[surface.vertexLabels[i]].segmentID != surfaceLabels[i])
      surface.vertexLabels[i] = 0;
  }
}
