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

#include <glcompatibility.h>
#include <surface.h>
#include <glglyphs.h>
#include "surfacealpha.h"
#include "brainsuiterenderer.h"
#include "brainsuitesettings.h"
#include "fiberrenderer.h"
#include <dstube.h>
#include <protocolcurveset.h>

SILT::Surface roiSphere;
GLEllipse glEllipse0(0);
GLEllipse glEllipse1(1);
GLEllipse glEllipse2(2);
GLEllipse glEllipse3(3);
GLEllipse glSphere(2);
FiberRenderer fiberRenderer;
SurfaceAlpha BrainSuiteRenderer::unitSphere;

void BrainSuiteRenderer::drawSolidColorSurface(const SILT::Surface &surface, const DSPoint &color, bool viewLighting)
{
  const size_t nt = surface.triangles.size();
  if (nt==0) return;
  glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
  glEnableClientState(GL_VERTEX_ARRAY);
  glVertexPointer(3,GL_FLOAT,0,&surface.vertices[0]);
  if (viewLighting && (surface.vertexNormals.size()>0))
  {
    glEnable(GL_LIGHTING);
    glEnableClientState(GL_NORMAL_ARRAY);
    glNormalPointer(GL_FLOAT,0,&surface.vertexNormals[0]);
  }
  else
  {
    glDisable(GL_LIGHTING);
    glDisableClientState(GL_NORMAL_ARRAY);
  }
  glDisableClientState(GL_COLOR_ARRAY);
  glEnable(GL_COLOR_MATERIAL);
  glColor3f(color.x,color.y,color.z);
  glDrawElements(GL_TRIANGLES,(GLsizei)surface.nt()*3,GL_UNSIGNED_INT,&surface.triangles[0]);
}

GLEllipse *currentEllipse()
{
  switch (BrainSuiteSettings::glyphQuality)
  {
    case 3 : return &glEllipse3;
    case 2 : return &glEllipse2;
    case 1 : return &glEllipse1;
    case 0 :
    default: break;
  }
  return &glEllipse0;
}

void BrainSuiteRenderer::initialize()
{
  GLEllipse sphere(2); // LOD = 2
  unitSphere.triangles = sphere.t;
  unitSphere.vertices = sphere.v;
  unitSphere.computeNormals();
  unitSphere.useSolidColor = true;
}

void BrainSuiteRenderer::drawFiberTrackSet(FiberTrackSet &fiberTrackSet)
{
  fiberRenderer.draw(fiberTrackSet);
}

void BrainSuiteRenderer::drawSphereStrip(std::vector<DSPoint> &points, DSPoint color, float scale)
{
  glEnable(GL_NORMALIZE);
  for (size_t j=0;j<points.size();j++)
  {
    glPushMatrix();
    glTranslatef(points[j].x,points[j].y,points[j].z);
    ::glScalef(scale,scale,scale);
    drawSolidColorSurface(unitSphere,color,true);
    glPopMatrix();
  }
}

void BrainSuiteRenderer::drawLineStrip(std::vector<DSPoint> &points, DSPoint color, bool selected)
{
  const GLfloat linesize=BrainSuiteSettings::defaultCurveWidth;
  glPushAttrib(GL_LINE_BIT|GL_LIGHTING_BIT);
  glDisable(GL_LIGHTING);
  glEnableClientState( GL_VERTEX_ARRAY );
  glColor3f(color.x,color.y,color.z);
  glLineWidth((selected) ? linesize*2.0f : linesize);
  glVertexPointer(3,GL_FLOAT,0,&points[0].x);
  glDrawArrays(GL_LINE_STRIP,0,(GLsizei)points.size());
  glDisableClientState( GL_VERTEX_ARRAY );
  glEnable(GL_LIGHTING);
  glPopAttrib();
}

void BrainSuiteRenderer::drawLineStrips(ProtocolCurveSet &curveSet, int selected)
{
  const GLfloat linesize=BrainSuiteSettings::defaultCurveWidth;
  glPushAttrib(GL_LINE_BIT|GL_LIGHTING_BIT);
  glDisable(GL_LIGHTING);
  glEnableClientState( GL_VERTEX_ARRAY );
  for (size_t i=0; i != curveSet.curves.size(); i++)
  {
    ProtocolCurve &curve=curveSet.curves[i];
    if (curve.points.size()==0) continue;
    glColor3f(curve.attributes.color.x,curve.attributes.color.y,curve.attributes.color.z);
    glLineWidth((selected==(int)i) ? linesize*2.0f : linesize);
    glVertexPointer(3,GL_FLOAT,0,&curve.points[0].x);
    glDrawArrays(GL_LINE_STRIP,0,(GLsizei)curve.points.size());
  }
  glDisableClientState( GL_VERTEX_ARRAY );
  glPopAttrib();
}

void BrainSuiteRenderer::draw(std::vector<DSPoint> &points, DSPoint color, bool selected)
{
  if (BrainSuiteSettings::showCurvesAsTubes)
  {
    float radius=(selected) ? 2.0f*BrainSuiteSettings::curveTubeSize : BrainSuiteSettings::curveTubeSize;
    fiberRenderer.render(points,color,radius);
    drawSphereStrip(points,color,radius);
  }
  else
  {
    drawLineStrip(points,color,selected);
  }
}

void BrainSuiteRenderer::drawCurveset(ProtocolCurveSet &curveSet, int selected)
{
  if (BrainSuiteSettings::showCurvesAsTubes)
  {
    if (BrainSuiteSettings::fiberWireframe)
    {
      ::glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
    }
    else
    {
      ::glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    }
    for (size_t i=0; i != curveSet.curves.size(); i++)
    {
      float radius=(selected==(int)i) ? 2.0f*BrainSuiteSettings::curveTubeSize : BrainSuiteSettings::curveTubeSize;
      fiberRenderer.render(curveSet.curves[i].points,curveSet.curves[i].attributes.color,radius);
      drawSphereStrip(curveSet.curves[i].points,curveSet.curves[i].attributes.color,radius);
    }
  }
  else
  {
    drawLineStrips(curveSet,selected);
  }
}

void BrainSuiteRenderer::setClippingPlane(BrainSuiteSettings::Clipping clipping, int glClipCode, DSPoint normal, DSPoint point, float offset)
{
  switch (clipping)
  {
    case BrainSuiteSettings::Positive :
    {
      double eqn[4] = { 1, 0, 0, 0 };
      point -= DSPoint(0.50f,0.50f,0.50f);
      float intercept = normal.dot(point) + offset;
      eqn[0] = -normal.x;
      eqn[1] = -normal.y;
      eqn[2] = -normal.z;
      eqn[3] = intercept;
      ::glClipPlane(glClipCode,eqn);
      ::glEnable(glClipCode);
    }
      break;
    case BrainSuiteSettings::Negative :
    {
      double eqn[4] = { 1, 0, 0, 0 };
      point -= DSPoint(0.50f,0.50f,0.51f);
      float intercept = normal.dot(point) - offset;
      eqn[0] = normal.x;
      eqn[1] = normal.y;
      eqn[2] = normal.z;
      eqn[3] = -intercept;
      ::glClipPlane(glClipCode,eqn);
      ::glEnable(glClipCode);
    }
      break;
    default:
      glDisable(glClipCode);
  }
}
void BrainSuiteRenderer::setClip(bool flag)
{
  if (flag)
  {
    if (BrainSuiteSettings::clipXPlane) { ::glEnable(GL_CLIP_PLANE0); } else glDisable(GL_CLIP_PLANE0);
    if (BrainSuiteSettings::clipYPlane) { ::glEnable(GL_CLIP_PLANE1); } else glDisable(GL_CLIP_PLANE1);
    if (BrainSuiteSettings::clipZPlane) { ::glEnable(GL_CLIP_PLANE2); } else glDisable(GL_CLIP_PLANE2);
  }
  else
  {
    glDisable(GL_CLIP_PLANE0);
    glDisable(GL_CLIP_PLANE1);
    glDisable(GL_CLIP_PLANE2);
  }
}

void BrainSuiteRenderer::resetClip()
{
  if (BrainSuiteSettings::clipXPlane) ::glEnable(GL_CLIP_PLANE0); else glDisable(GL_CLIP_PLANE0);
  if (BrainSuiteSettings::clipYPlane) ::glEnable(GL_CLIP_PLANE1); else glDisable(GL_CLIP_PLANE1);
  if (BrainSuiteSettings::clipZPlane) ::glEnable(GL_CLIP_PLANE2); else glDisable(GL_CLIP_PLANE2);
}

void BrainSuiteRenderer::drawCrossHairs(const Vol3DBase *volume, const IPoint3D &point)
{
  if (!volume) return;
  const float rx = volume->rx;
  const float ry = volume->ry;
  const float rz = volume->rz;
  const int cx = volume->cx;
  const int cy = volume->cy;
  const int cz = volume->cz;
  GLfloat modelview[16];
  glGetFloatv( GL_MODELVIEW_MATRIX, modelview );
  DSPoint p(modelview[2],modelview[6],modelview[10]);
  glPushAttrib(GL_ALL_ATTRIB_BITS);
  glPushMatrix();
  setClip(false);
  glDisable(GL_LIGHTING);
  if (!BrainSuiteSettings::ignoreVolumePosition)
    glTranslatef(volume->origin.x,volume->origin.y,volume->origin.z);
  glScalef(rx,ry,rz);
  glColor3f(0,0,1);
  dsTubeX(point,cx);
  dsTubeY(point,cy);
  dsTubeZ(point,cz);
  glPopMatrix();
  glPopAttrib();
  resetClip();
}

void BrainSuiteRenderer::drawTrackROISphere(const DSPoint &position, const float radius, const DSPoint &color, bool wireframe)
{
  bool viewLighting=BrainSuiteSettings::useLighting;
  if (roiSphere.nv()==0)
  {
    roiSphere.vertices = glEllipse3.v;
    roiSphere.triangles = glEllipse3.t;
    roiSphere.computeNormals();
  }
  glPushAttrib(GL_ALL_ATTRIB_BITS);
  glPushMatrix();
  glEnable(GL_NORMALIZE);
  glTranslatef(position.x,position.y,position.z);
  glScalef(radius,radius,radius);
  ::glPolygonMode(GL_FRONT_AND_BACK,wireframe ? GL_LINE : GL_FILL);
  glEnableClientState(GL_VERTEX_ARRAY);
  glVertexPointer(3,GL_FLOAT,0,&roiSphere.vertices[0]);
  if (viewLighting && (roiSphere.vertexNormals.size()>0))
  {
    glEnable(GL_LIGHTING);
    glEnableClientState(GL_NORMAL_ARRAY);
    glNormalPointer(GL_FLOAT,0,&roiSphere.vertexNormals[0]);
  }
  else
  {
    glDisable(GL_LIGHTING);
    glDisableClientState(GL_NORMAL_ARRAY);
  }
  glDisableClientState(GL_COLOR_ARRAY);
  glEnable(GL_COLOR_MATERIAL);
  glColor3f(color.x,color.y,color.z);
  glDrawElements(GL_TRIANGLES,(GLsizei)roiSphere.nt()*3,GL_UNSIGNED_INT,&roiSphere.triangles[0]);
  glPopMatrix();
  glPopAttrib();
}

void BrainSuiteRenderer::drawSolidSurface(SurfaceAlpha &surface, bool viewLighting)
{
  const size_t nt = surface.triangles.size();
  if (nt<=0) return;
  ::glPolygonMode(GL_FRONT_AND_BACK,surface.wireFrame ? GL_LINE : GL_FILL);
  const size_t nv = surface.vertices.size();
  DSPoint *verts = (surface.displayAlternate && surface.alternateVertices.size()==nv) ? &surface.alternateVertices[0] : &surface.vertices[0];
  if (nt>0)
  {
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3,GL_FLOAT,0,verts);
    {
      if (viewLighting && (surface.vertexNormals.size()>0))
      {
        glEnable(GL_LIGHTING);
        glEnableClientState(GL_NORMAL_ARRAY);
        glNormalPointer(GL_FLOAT,0,&surface.vertexNormals[0]);
      }
      else
      {
        glDisable(GL_LIGHTING);
        glDisableClientState(GL_NORMAL_ARRAY);
      }
      if (surface.useVertexColor&&surface.vertexColor.size()==surface.vertices.size())
      {
        glEnable(GL_COLOR_MATERIAL);
        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(3,GL_FLOAT,0,&surface.vertexColor[0]);
      }
      else
      {
        glDisableClientState(GL_COLOR_ARRAY);
        glEnable(GL_COLOR_MATERIAL);
        glColor3f(surface.solidColor.x,surface.solidColor.y,surface.solidColor.z);
      }
      {
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_TEXTURE_COORD_ARRAY);
      }
      glDrawElements(GL_TRIANGLES,(GLsizei)surface.nt()*3,GL_UNSIGNED_INT,&surface.triangles[0]);
    }
  }
}

void drawSurfaceFlatAlpha(SurfaceAlpha &surface, bool /*wtf*/, bool /*depthSort*/=false)
{
  if (surface.vertexUV.size()!=surface.vertices.size()) return;
  ::glPolygonMode(GL_FRONT_AND_BACK,surface.wireFrame ? GL_LINE : GL_FILL);
  const size_t nt = surface.triangles.size();
  const size_t nv = surface.vertexUV.size();
  if (nt>0)
  {
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2,GL_FLOAT,0,&surface.vertexUV[0].u);
    {
      glDisable(GL_LIGHTING);
      glDisableClientState(GL_NORMAL_ARRAY);
    }
    if (nv!=surface.vertexColor.size())
    {
      surface.useVertexColor = false;
      surface.useSolidColor = true;
    }
    else
    {
      if (surface.useVertexColor)
      {
        if (surface.alphaColor.size() !=surface.vertexColor.size())
        {
          surface.alphaColor.resize(surface.vertexColor.size());
          const size_t n = surface.alphaColor.size();
          for (size_t i=0;i<n;i++)
          {
            surface.alphaColor[i] = RGBAf(surface.vertexColor[i],0.5f);
          }
        }
      }
    }
    if (surface.useVertexColor&&surface.alphaColor.size()==nv)
    {
      if (surface.alphaColor.size()>0)
      {
        if (surface.alpha != surface.alphaColor[0].a)
        {
          const size_t nv = surface.alphaColor.size();
          const float alpha = surface.alpha;
          for (size_t i=0;i<nv;i++)
          {
            surface.alphaColor[i].a = alpha;
          }
        }
        glEnable(GL_COLOR_MATERIAL);
        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(4,GL_FLOAT,0,&surface.alphaColor[0].r);
      }
    }
    else
    {
      glDisableClientState(GL_COLOR_ARRAY);
      glEnable(GL_COLOR_MATERIAL);
      glColor4f(surface.solidColor.x,surface.solidColor.y,surface.solidColor.z,surface.alpha);
    }
    glDrawElements(GL_TRIANGLES,(GLsizei)surface.nt()*3,GL_UNSIGNED_INT,&surface.triangles[0].a);
  }
}
