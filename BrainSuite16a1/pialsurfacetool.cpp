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

#include <pialsurfacetool.h>

PialSurfaceTool::PialSurfaceTool() : iterationNo(0), numIterations(100), presmoothIterations(0), estimateCorticalThickness(false)
{
}

bool PialSurfaceTool::resetIterations()
{
  halt = false;
  iterationNo = 0;
  sIn.computeConnectivity();
  copySurf(sOut,sIn);
  if (presmoothIterations>0)
  {
    SurfLib::laplaceSmoothCurvatureWeighted(sOut,presmoothIterations,0.75f,5.0f);
  }
  sOut.computeNormals();
  int maxX = 0;
  int maxY = 0;
  int maxZ = 0;
  const int cx=vTissue.cx;
  const int cy=vTissue.cy;
  const int cz=vTissue.cz;
  for (int z=0;z<cz;z++)
    for (int y=0;y<cy;y++)
      for (int x=0;x<cx;x++)
      {
        if (vTissue(x,y,z))
        {
          if (x>maxX) maxX=x;
          if (y>maxY) maxY=y;
          if (z>maxZ) maxZ=z;
        }
      }
  return true;
}

void PialSurfaceTool::performNextIteration()
{
  iterationNo++;
  step(sOut,sIn,vTissue);
}

template <class T> void vcopy(T &dst, const T &src)
{
  const size_t nv = src.size();
  dst.resize(nv);
  for (size_t i=0;i<nv;i++) dst[i] = src[i];
}

void PialSurfaceTool::copySurf(Surface &second, const Surface &first)
{
  vcopy(second.vertices,first.vertices);
  vcopy(second.triangles,first.triangles);
  vcopy(second.vertexColor,first.vertexColor);
  second.computeConnectivity();
  second.computeNormals();
  second.filename = "<gm surface>";
}
