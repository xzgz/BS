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

#include <cmath>
#include "brainsuitesettings.h"
#include <glglyphs.h>
#include <fibertrackset.h>
#include "fiberrenderer.h"
#include <brainsuiterenderer.h>
#include <glglyphs.h>

inline void FiberRenderer::rotateCylinder(const DSPoint &d)
{
  const float m = d.mag();
  float angle = std::acos(d.z/m)*(float)(180.0/M_PI);
  glRotatef(-angle, d.y, -d.x, 0);
}

FiberRenderer::FiberRenderer()
{
  cylinder.create(1.0f,10);
}

void FiberRenderer::setup(float radius, int nCylinderPanels)
{
  if (cylinder.nPanels != nCylinderPanels)
  {
    cylinder.create(radius,nCylinderPanels);
  }
}

void FiberRenderer::drawCylinder(DSPoint &p0, DSPoint &p1, DSPoint color, float radius)
{
  glColor3f(color.x,color.y,color.z);
  glEnable(GL_NORMALIZE);

  glEnableClientState( GL_VERTEX_ARRAY );
  glEnableClientState( GL_NORMAL_ARRAY );
  glDisableClientState( GL_COLOR_ARRAY );
  glDisableClientState( GL_INDEX_ARRAY );
  glVertexPointer( 3, GL_FLOAT, 0, &cylinder.vertices[0]);  // Set The Vertex Pointer To Our Vertex Data
  glNormalPointer(GL_FLOAT, 0, &cylinder.normals[0]);       // Set The Vertex Normal Pointer To The Vertex Normal Buffer

  glPushMatrix();
  DSPoint d = p1-p0;
  glTranslatef(p0.x,p0.y,p0.z);
  rotateCylinder(d);
  glScalef(radius,radius,d.mag());
  glDrawArrays(GL_QUAD_STRIP,0,(GLsizei)cylinder.vertices.size());
  glPopMatrix();
}

void FiberRenderer::render(const std::vector<DSPoint> &pointBuffer, DSPoint color, float radius)
{
  glColor3f(color.x,color.y,color.z);
  const size_t nPoints=pointBuffer.size();
  glEnable(GL_NORMALIZE);
  {
    glEnableClientState( GL_VERTEX_ARRAY );            // Enable Vertex Arrays
    glEnableClientState( GL_NORMAL_ARRAY );            // Enable Normal Arrays
    glDisableClientState( GL_COLOR_ARRAY );
    glDisableClientState( GL_INDEX_ARRAY );
    glVertexPointer( 3, GL_FLOAT, 0, &cylinder.vertices[0]); // Set The Vertex Pointer To Our Vertex Data
    glNormalPointer(GL_FLOAT, 0, &cylinder.normals[0]);      // Set The Vertex Normal Pointer To The Vertex Normal Buffer
    for (size_t i=1;i<nPoints;i++)
    {
      glPushMatrix();
      DSPoint d = pointBuffer[i]-pointBuffer[i-1];
      glTranslatef(pointBuffer[i-1].x,pointBuffer[i-1].y,pointBuffer[i-1].z);
      rotateCylinder(d);
      glScalef(radius,radius,d.mag());
      glDrawArrays(GL_QUAD_STRIP,0,(GLsizei)cylinder.vertices.size());
      glPopMatrix();
    }
  }
  return;
}

void FiberRenderer::renderFiberTrack(const FiberTrack &curve, float radius)
{
  DSPoint color=(BrainSuiteSettings::trackColorModel==BrainSuiteSettings::SingleColor) ? BrainSuiteSettings::defaultFiberColor : curve.attributes.color;
  const bool colorByAngle(BrainSuiteSettings::trackColorModel==BrainSuiteSettings::LocalAngle);
  glColor3fv(&color.x);
  const size_t nPoints=curve.points.size();
  glEnable(GL_NORMALIZE);
  {
    glEnableClientState( GL_VERTEX_ARRAY );            // Enable Vertex Arrays
    glEnableClientState( GL_NORMAL_ARRAY );            // Enable Normal Arrays
    glDisableClientState( GL_COLOR_ARRAY );            // Enable Vertex Arrays
    glDisableClientState( GL_INDEX_ARRAY );            // Enable Vertex Arrays
    glVertexPointer( 3, GL_FLOAT, 0, &cylinder.vertices[0]); // Set The Vertex Pointer To Our Vertex Data
    glNormalPointer(GL_FLOAT, 0, &cylinder.normals[0]);    // Set The Vertex Normal Pointer To The Vertex Normal Buffer
    int start=0;
    int stop=nPoints;
    for (auto i=start+1;i<stop;i++)
    {
      glPushMatrix();
      DSPoint d = curve.points[i]-curve.points[i-1];
      if (colorByAngle)
      {
        DSPoint color=abs(d.unit());
        glColor3fv(&color.x);
      }
      glTranslatef(curve.points[i-1].x,curve.points[i-1].y,curve.points[i-1].z);
      rotateCylinder(d);
      glScalef(radius,radius,d.mag());
      glDrawArrays(GL_QUAD_STRIP,0,(GLsizei)cylinder.vertices.size());
      glPopMatrix();
    }
  }
  return;
}

void FiberRenderer::drawLineAngleColor(const FiberTrack &curve)
{
  if (curve.points.size()==0) return;
  const size_t nPoints = curve.points.size();
  ::glBegin(GL_LINE_STRIP);
  DSPoint color=curve.attributes.color;
  const DSPoint d=curve.points[1]-curve.points[0];
  if (d.pwr())
  {
    if (nPoints>1) color=abs(d.unit());
    glColor3fv(&color.x);
    glVertex3fv(&curve.points[0].x);
  }
  for (size_t i=1;i<nPoints;i++)
  {
    const DSPoint d=curve.points[i]-curve.points[i-1];
    if (d.pwr()) color=abs(d.unit());
    glColor3fv(&color.x);
    glVertex3fv(&curve.points[i].x);
  }
  ::glEnd();
}

void FiberRenderer::drawPolyline(const FiberTrack &curve, DSPoint color)
{
  if (curve.points.size()==0) return;
  const size_t nPoints = curve.points.size();
  if (nPoints==0) return;
  glColor3fv(&color.x);
  ::glBegin(GL_LINE_STRIP);
  for (size_t i=0;i<nPoints;i++) glVertex3fv(&curve.points[i].x);
  ::glEnd();
}

bool FiberRenderer::draw(FiberTrackSet &fiberSet)
{
  if (fiberSet.curves.size()==0) return true;
  glPushAttrib(GL_ALL_ATTRIB_BITS);
  GLfloat modelview[16];
  glGetFloatv( GL_MODELVIEW_MATRIX, modelview );
  const DSPoint view(modelview[2],modelview[6],modelview[10]);

  glPolygonMode(GL_FRONT_AND_BACK,BrainSuiteSettings::fiberWireframe ? GL_LINE : GL_FILL);
  glEnable(GL_COLOR_MATERIAL);
  if (BrainSuiteSettings::showFibersAsTubes)
  {
    for (std::vector<FiberTrack>::iterator curve=fiberSet.curves.begin(); curve != fiberSet.curves.end(); curve++)
      renderFiberTrack(*curve,BrainSuiteSettings::tubeSize);
  }
  else
  {
    ::glEnable(GL_NORMALIZE);
    glNormal3f(view.x,view.y,view.z);
    ::glLineWidth(BrainSuiteSettings::fiberLineWidth);
    switch (BrainSuiteSettings::trackColorModel)
    {
      case BrainSuiteSettings::LocalAngle:
        for (std::vector<FiberTrack>::iterator curve=fiberSet.curves.begin(); curve != fiberSet.curves.end(); curve++)
          drawLineAngleColor(*curve);
        break;
      case BrainSuiteSettings::AverageAngle:
        for (std::vector<FiberTrack>::iterator curve=fiberSet.curves.begin(); curve != fiberSet.curves.end(); curve++)
          drawPolyline(*curve,curve->attributes.color);
        break;
      case BrainSuiteSettings::FiberLength:
        for (std::vector<FiberTrack>::iterator curve=fiberSet.curves.begin(); curve != fiberSet.curves.end(); curve++)
          drawPolyline(*curve,curve->attributes.color);
        break;
      case BrainSuiteSettings::SingleColor:
        for (std::vector<FiberTrack>::iterator curve=fiberSet.curves.begin(); curve != fiberSet.curves.end(); curve++)
          drawPolyline(*curve,BrainSuiteSettings::defaultFiberColor);
        break;
    }
    ::glLineWidth(1);
  }
  glDisable(GL_COLOR_MATERIAL);
  glPopAttrib();
  return true;
}
