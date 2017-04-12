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

#include "dslight.h"
#include <qgl.h>
#include <qdebug.h>

DSLight::DSLight(const int lightID) : lightID(lightID), position(0.25f,0.7f,1.0f), diffuseColor(0.0f, 0.0f, 0.0f),
  ambientLight(0.0f), diffuseLight(0.0f), specularLight(0), enabled(false)
{
}

void DSLight::lightItUp()
{
  if (enabled)
  {
    glEnable(lightID);
    glLightfv(lightID,GL_POSITION,&position.x);
    GLfloat ambient[4]={ambientLight,ambientLight,ambientLight,1.0f};
    GLfloat specular[4] = {specularLight,specularLight,specularLight,1.0f};
    GLfloat diffuse[4] = {1,1,1,1.0f};
    glLightfv(lightID,GL_AMBIENT,ambient);
    glLightfv(lightID,GL_SPECULAR,specular);
    glLightfv(lightID,GL_DIFFUSE,diffuse);
  }
  else
    glDisable(lightID);
}
