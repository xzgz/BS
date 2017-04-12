// Copyright (C) 2016 The Regents of the University of California
//
// Created by David W. Shattuck, Ph.D.
//
// This file is part of Vol3D.
//
// Vol3D is free software; you can redistribute it and/or
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

#include <vol3dreorder.h>
#include <vol3dbase.h>

DSPoint Vol3DReorder::codeToRASVector(char code)
{
  DSPoint ev(1,0,0);
  switch (code)
  {
    case 'L' : ev=DSPoint(-1, 0, 0); break;
    case 'R' : ev=DSPoint( 1, 0, 0); break;
    case 'P' : ev=DSPoint( 0,-1, 0); break;
    case 'A' : ev=DSPoint( 0, 1, 0); break;
    case 'I' : ev=DSPoint( 0, 0,-1); break;
    case 'S' : ev=DSPoint( 0, 0, 1); break;
  }
  return ev;
}

std::string Vol3DReorder::getOrientationRAS(const DSPoint &vector)
{
  std::ostringstream ostr;
  char orientationX = vector.x < 0 ? 'L' : 'R';
  char orientationY = vector.y < 0 ? 'P' : 'A';
  char orientationZ = vector.z < 0 ? 'I' : 'S';
  double absX = std::fabs(vector.x);
  double absY = std::fabs(vector.y);
  double absZ = std::fabs(vector.z);
  for (int i=0; i<3; i++)
  {
    if (absX>0.1 && absX>absY && absX>absZ)
    {
      ostr<<orientationX;
      absX=0;
    }
    else if (absY>0.1 && absY>absX && absY>absZ)
    {
      ostr<<orientationY;
      absY=0;
    }
    else if (absZ>0.1 && absZ>absX && absZ>absY)
    {
      ostr<<orientationZ;
      absZ=0;
    }
    else break;
  }
  return ostr.str();
}
