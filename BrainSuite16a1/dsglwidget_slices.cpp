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

#include <dsglwidget.h>
#include <imagewidget.h>
#include <brainsuitewindow.h>
#include <imageframe.h>
#include <QtOpenGL>
#include <brainsuitesettings.h>

void DSGLWidget::glRenderPortXY(ImageWidget *port, const float alpha, const bool mask)
{
  if (!port) return;
  port->updateImage();
  QSize size = port->blendBitmap.size();
  const int cx=size.width();
  const int cy=size.height();
  if (cx<=0) return;
  if (cy<=0) return;
  const int z = port->imageState.currentPosition.z;
  uint8 *bits = port->blendBitmap.image.bits();
  uint8 *maskbits = port->maskBitmap.image.bits();
  if (mask && maskbits && (port->blendBitmap.size()==port->maskBitmap.size()))
  {
    glBegin(GL_QUADS);
    for (int y=0;y<cy;y++)
      for (int x=0;x<cx;x++)
      {
        if (maskbits[0])
        {
          glColor4f(bits[2]/255.0f,bits[1]/255.0f,bits[0]/255.0f,alpha);
          glVertex3i(x  ,y  ,z);
          glVertex3i(x+1,y  ,z);
          glVertex3i(x+1,y+1,z);
          glVertex3i(x  ,y+1,z);
        }
        maskbits+=4;
        bits+=4;
      }
    glEnd();
  }
  else
  {
    glShadeModel(GL_FLAT);
    for (int y=0;y<cy;y++)
    {
      ::glBegin(GL_QUAD_STRIP);
      glVertex3i(0  ,y  ,z);
      glVertex3i(0  ,y+1,z);
      for (int x=0;x<cx;x++)
      {
        glVertex3i(x+1,y  ,z);
        glColor4f(bits[2]/255.0f,bits[1]/255.0f,bits[0]/255.0f,alpha); // GL_QUAD_STRIPS are colored by the 4th vertex
        glVertex3i(x+1,y+1,z);
        bits+=4;
      }
      ::glEnd();
    }
    glShadeModel(GL_SMOOTH);
  }
}

void DSGLWidget::glRenderPortXZ(ImageWidget *port, const float alpha, const bool mask)
{
  if (!port) return;
  port->updateImage();
  QSize size = port->blendBitmap.size();
  const int cx=size.width();
  const int cz=size.height();
  if (cx<=0) return;
  if (cz<=0) return;
  const int y = port->imageState.currentPosition.y;
  {
    const uint8 *bits = port->blendBitmap.image.bits();
    const uint8 *maskbits = port->maskBitmap.image.bits();
    if (mask && maskbits && (port->blendBitmap.size()==port->maskBitmap.size()))
    {
      ::glBegin(GL_QUADS);
      for (int z=0;z<cz;z++)
        for (int x=0;x<cx;x++)
        {
          if (maskbits[0])
          {
            glColor4f(bits[2]/255.0f,bits[1]/255.0f,bits[0]/255.0f,alpha);
            glVertex3i(x  ,y  ,z);
            glVertex3i(x+1,y  ,z);
            glVertex3i(x+1,y,z+1);
            glVertex3i(x  ,y,z+1);
          }
          maskbits+=4;
          bits+=4;
        }
      ::glEnd();
    }
    else
    {
      glShadeModel(GL_FLAT);
      for (int z=0;z<cz;z++)
      {
        ::glBegin(GL_QUAD_STRIP);
        glVertex3i(0  ,y  ,z);
        glVertex3i(0  ,y,z+1);
        for (int x=0;x<cx;x++)
        {
          glVertex3i(x+1,y  ,z);
          glColor4f(bits[2]/255.0f,bits[1]/255.0f,bits[0]/255.0f,alpha);
          glVertex3i(x+1,y,z+1);
          bits+=4;
        }
        ::glEnd();
      }
      glShadeModel(GL_SMOOTH);
    }
  }
}

void DSGLWidget::glRenderPortYZ(ImageWidget *port, const float alpha, const bool mask)
{
  if (!port) return;
  port->updateImage();
  QSize size = port->blendBitmap.size();
  const int cy=size.width();
  const int cz=size.height();
  if (cy<=0) return;
  if (cz<=0) return;
  const int x = port->imageState.currentPosition.x;
  {
    const uint8 *bits = port->blendBitmap.image.bits();
    const uint8 *maskbits = port->maskBitmap.image.bits();
    if (mask && maskbits && (port->blendBitmap.size()==port->maskBitmap.size()))
    {
      ::glBegin(GL_QUADS);
      for (int z=0;z<cz;z++)
        for (int y=0;y<cy;y++)
        {
          if (maskbits[0])
          {
            glColor4f(bits[2]/255.0f,bits[1]/255.0f,bits[0]/255.0f,alpha);
            glVertex3i(x  ,y  ,z);
            glVertex3i(x  ,y+1,z);
            glVertex3i(x  ,y+1,z+1);
            glVertex3i(x  ,y  ,z+1);
          }
          maskbits+=4;
          bits+=4;
        }
      ::glEnd();
    }
    else
    {
      glShadeModel(GL_FLAT);
      for (int z=0;z<cz;z++)
      {
        ::glBegin(GL_QUAD_STRIP);
        glVertex3i(x  ,0  ,z);
        glVertex3i(x  ,0,z+1);
        for (int y=0;y<cy;y++)
        {
          glVertex3i(x,y+1,z);
          glColor4f(bits[2]/255.0f,bits[1]/255.0f,bits[0]/255.0f,alpha);
          glVertex3i(x,y+1,z+1);
          bits+=4;
        }
        ::glEnd();
      }
      glShadeModel(GL_SMOOTH);
    }
  }
}

void DSGLWidget::glRenderPort(ImageWidget *port, const float alpha)
{
  if (!port) return;
  bool mask = port->imageState.viewMask;
  if (mask)
    switch (port->imageState.maskMode)
    {
      case ImageState::Off:
      case ImageState::Outline:
        mask = false;
        break;
      case ImageState::Blend:
      case ImageState::Stencil:
      case ImageState::MaskOnly :
        mask = true;
        break;
    }
  switch (port->orientation)
  {
    case Orientation::XY :
      if (surfaceViewProperties.showXYPlane)
      {
        glTranslatef(-0.5f,-0.5f, 0);
        glRenderPortXY(port, alpha, mask);
        glTranslatef( 0.5f, 0.5f, 0);
      }
      break;
    case Orientation::XZ :
      if (surfaceViewProperties.showXZPlane)
      {
        glTranslatef(-0.5f,0,-0.5f);
        glRenderPortXZ(port, alpha, mask);
        glTranslatef( 0.5f,0, 0.5f);
      }
      break;
    case Orientation::YZ :
      if (surfaceViewProperties.showYZPlane)
      {
        glTranslatef(0,-0.5f,-0.5f);
        glRenderPortYZ(port, alpha, mask);
        glTranslatef(0, 0.5f, 0.5f);
      }
      break;
    default:
      break;
  }
}

void drawCube(DSPoint start, DSPoint stop)
{
  {
    ::glBegin(GL_LINE_LOOP);
    ::glVertex3i(start.x,start.y,start.z);
    ::glVertex3i(stop.x,start.y,start.z);
    ::glVertex3i(stop.x,stop.y,start.z);
    ::glVertex3i(start.x,stop.y,start.z);
    glEnd();
    ::glBegin(GL_LINES);
    ::glVertex3i(start.x,start.y,start.z);
    ::glVertex3i(start.x,start.y,stop.z);
    ::glVertex3i(stop.x,start.y,start.z);
    ::glVertex3i(stop.x,start.y,stop.z);
    ::glVertex3i(start.x,stop.y,start.z);
    ::glVertex3i(start.x,stop.y,stop.z);
    ::glVertex3i(stop.x,stop.y,start.z);
    ::glVertex3i(stop.x,stop.y,stop.z);
    glEnd();
    ::glBegin(GL_LINE_LOOP);
    ::glVertex3i(start.x,start.y,stop.z);
    ::glVertex3i(stop.x,start.y,stop.z);
    ::glVertex3i(stop.x,stop.y,stop.z);
    ::glVertex3i(start.x,stop.y,stop.z);
    glEnd();
  }
}

void drawCube(IPoint3D start, IPoint3D stop)
{
  {
    ::glBegin(GL_LINE_LOOP);
    ::glVertex3i(start.x,start.y,start.z);
    ::glVertex3i(stop.x,start.y,start.z);
    ::glVertex3i(stop.x,stop.y,start.z);
    ::glVertex3i(start.x,stop.y,start.z);
    glEnd();
    ::glBegin(GL_LINES);
    ::glVertex3i(start.x,start.y,start.z);
    ::glVertex3i(start.x,start.y,stop.z);
    ::glVertex3i(stop.x,start.y,start.z);
    ::glVertex3i(stop.x,start.y,stop.z);
    ::glVertex3i(start.x,stop.y,start.z);
    ::glVertex3i(start.x,stop.y,stop.z);
    ::glVertex3i(stop.x,stop.y,start.z);
    ::glVertex3i(stop.x,stop.y,stop.z);
    glEnd();
    ::glBegin(GL_LINE_LOOP);
    ::glVertex3i(start.x,start.y,stop.z);
    ::glVertex3i(stop.x,start.y,stop.z);
    ::glVertex3i(stop.x,stop.y,stop.z);
    ::glVertex3i(start.x,stop.y,stop.z);
    glEnd();
  }
}

std::string glerrstring(int code)
{
  switch (code)
  {
    case 0x0500: return "GL_INVALID_ENUM";
    case 0x0501: return "GL_INVALID_VALUE";
    case 0x0502: return "GL_INVALID_OPERATION";
    case 0x0503: return "GL_STACK_OVERFLOW";
    case 0x0504: return "GL_STACK_UNDERFLOW";
    case 0x0505: return "GL_OUT_OF_MEMORY";
    case 0x0506: return "GL_INVALID_FRAMEBUFFER_OPERATION";
  }
  return "unk";
}


void paintBoundingBox(Vol3DBase *volume)
{
  if (!volume) return;
  ::glPushAttrib(GL_ALL_ATTRIB_BITS);
  ::glPushMatrix();
  ::glDisable(GL_LIGHTING);
  float px=volume->cx*volume->rx;
  float py=volume->cy*volume->ry;
  float pz=volume->cz*volume->rz;
  glColor3f(0,0,1);
  drawCube(DSPoint(),DSPoint(px,py,pz));
  ::glPopMatrix();
  ::glPopAttrib();
}

void DSGLWidget::paintBoundingBoxes()
{
  ::paintBoundingBox(brainSuiteWindow->brainSuiteDataManager.volume);
}

void DSGLWidget::paintSlices(bool translucent)
{  
  if (!BrainSuiteSettings::renderSlices) return;
  if (!brainSuiteWindow) return;
  Vol3DBase *volume = brainSuiteWindow->brainSuiteDataManager.volume;
  if (!volume) return;
  ::glPushAttrib(GL_ALL_ATTRIB_BITS);
  ::glPushMatrix();
  ::glDisable(GL_LIGHTING);
  ::glScalef(volume->rx,volume->ry,volume->rz);
  // removing Translation for slice painting
  ::glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
  if (translucent)
  {
    ::glEnable(GL_ALPHA_TEST);
    ::glEnable (GL_BLEND);
    ::glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    ::glDepthFunc(GL_LEQUAL);

  }
  for (size_t i=0;i<brainSuiteWindow->ports.size();i++)
  {
    glRenderPort(brainSuiteWindow->ports[i]->image(), BrainSuiteSettings::sliceOpacity);
  }
  ::glPopMatrix();
  if (translucent) glDisable (GL_BLEND);
  ::glPopAttrib();
}

