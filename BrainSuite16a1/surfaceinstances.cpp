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

#include <surfaceinstances.h>
#include <brainsuitesettings.h>
#include <glcompatibility.h>
#include <sstream>

SurfaceInstanceSet::SurfaceInstanceSet() : scale(1.0f), wireframe(false)
{
}

bool SurfaceInstanceSet::initialize(std::vector<Triangle> &triangles_, std::vector<DSPoint> &vertexColors_, const size_t nInstances)
{
  triangles = triangles_;
  vertexColors = vertexColors_;
  instances.resize(nInstances);
  return instances.size()==nInstances;
}

void SurfaceInstanceSet::draw(bool viewLighting)
{
  const size_t ninstances = instances.size();
  if (ninstances==0) return;
  if (wireframe)
  {
    ::glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
  }
  else
    ::glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
  const size_t nt = triangles.size();
  std::ostringstream ostr;
  if (nt==0) return;
  if (viewLighting) glEnable(GL_LIGHTING);
  {
    ::glEnable(GL_NORMALIZE);
    const size_t bNV = instances[0].vertex.size();
    if (vertexColors.size()==bNV)
    {
      glEnable(GL_COLOR_MATERIAL);
      glEnableClientState(GL_COLOR_ARRAY);
      glColorPointer(3,GL_FLOAT,0,&vertexColors[0]);
    }
    else
    {
      glDisableClientState(GL_COLOR_ARRAY);
      glEnable(GL_COLOR_MATERIAL);
      glColor3f(0.7f,0.7f,0.7f);
    }
    for (size_t i=0;i<ninstances;i++)
    {
      SurfaceInstance &instance = instances[i];
      const size_t bNV = instance.vertex.size();
      if (!bNV) continue;
      ::glPushMatrix();
      ::glTranslatef(instance.position.x,instance.position.y,instance.position.z);
      ::glScalef(scale,scale,scale);
      {
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(3,GL_FLOAT,0,&instance.vertex[0]);
        if (viewLighting && (instance.vertexNormal.size()==bNV))
        {
          glEnable(GL_LIGHTING);
          glEnableClientState(GL_NORMAL_ARRAY);
          glNormalPointer(GL_FLOAT,0,&instance.vertexNormal[0]);
        }
        else
        {
          glDisable(GL_LIGHTING);
          glDisableClientState(GL_NORMAL_ARRAY);
        }
        {
          glDrawElements(GL_TRIANGLES,(GLsizei)nt*3,GL_UNSIGNED_INT,&triangles[0]);
        }
      }
      ::glPopMatrix();
    }
    ::glDisable(GL_NORMALIZE);
  }
}
