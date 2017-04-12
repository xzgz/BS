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

#include <dstube.h>
#include <glcompatibility.h>
#include <point3d.h>

template <class S, class T, class U>
inline void glVertex3stu(const S& x, const T &y, const U &z)
{
  glVertex3f(float(x),float(y),float(z));
}


void dsTubeX(const IPoint3D &p, const int cx)
{
  const float width = 0.25f;
  glBegin(GL_QUADS);
  glVertex3stu(0,p.y,p.z+width);
  glVertex3stu(0,p.y+width,p.z);
  glVertex3stu(cx,p.y+width,p.z);
  glVertex3stu(cx,p.y,p.z+width);
  glVertex3stu(0,p.y,p.z-width);
  glVertex3stu(0,p.y+width,p.z);
  glVertex3stu(cx,p.y+width,p.z);
  glVertex3stu(cx,p.y,p.z-width);
  glVertex3stu(0,p.y,p.z-width);
  glVertex3stu(0,p.y-width,p.z);
  glVertex3stu(cx,p.y-width,p.z);
  glVertex3stu(cx,p.y,p.z-width);
  glVertex3stu(0,p.y,p.z+width);
  glVertex3stu(0,p.y-width,p.z);
  glVertex3stu(cx,p.y-width,p.z);
  glVertex3stu(cx,p.y,p.z+width);
  glEnd();
}

void dsTubeY(const IPoint3D &p, const int cy)
{
  const float width = 0.1f;
  glBegin(GL_QUADS);
  glVertex3stu(p.x,0,p.z+width);
  glVertex3stu(p.x+width,0,p.z);
  glVertex3stu(p.x+width,cy,p.z);
  glVertex3stu(p.x,cy,p.z+width);
  glVertex3stu(p.x,0,p.z-width);
  glVertex3stu(p.x+width,0,p.z);
  glVertex3stu(p.x+width,cy,p.z);
  glVertex3stu(p.x,cy,p.z-width);
  glVertex3stu(p.x,0,p.z-width);
  glVertex3stu(p.x-width,0,p.z);
  glVertex3stu(p.x-width,cy,p.z);
  glVertex3stu(p.x,cy,p.z-width);
  glVertex3stu(p.x,0,p.z+width);
  glVertex3stu(p.x-width,0,p.z);
  glVertex3stu(p.x-width,cy,p.z);
  glVertex3stu(p.x,cy,p.z+width);
  glEnd();
}

void dsTubeZ(const IPoint3D &p, const int cz)
{
  const float width = 0.1f;
  glBegin(GL_QUADS);
  glVertex3stu(p.x,p.y+width,0);
  glVertex3stu(p.x+width,p.y,0);
  glVertex3stu(p.x+width,p.y,cz);
  glVertex3stu(p.x,p.y+width,cz);
  glVertex3stu(p.x,p.y-width,0);
  glVertex3stu(p.x+width,p.y,0);
  glVertex3stu(p.x+width,p.y,cz);
  glVertex3stu(p.x,p.y-width,cz);
  glVertex3stu(p.x,p.y-width,0);
  glVertex3stu(p.x-width,p.y,0);
  glVertex3stu(p.x-width,p.y,cz);
  glVertex3stu(p.x,p.y-width,cz);
  glVertex3stu(p.x,p.y+width,0);
  glVertex3stu(p.x-width,p.y,0);
  glVertex3stu(p.x-width,p.y,cz);
  glVertex3stu(p.x,p.y+width,cz);
  glEnd();
}
