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
#include "dsglslider.h"
#include <qpainter.h>

DSGLSlider::DSGLSlider() : triangleSize(7), triangleOffset(4), x0(0), x1(0), y0(0), y1(0), penumbra(5), color0(1,0,0), color1(0,0,1), frameColor(1,1,1),
  indicatorColor(1,1,1)
{
}

bool DSGLSlider::inside(const float x, const float y) const
{
  return ((x>=x0-penumbra)&&(x<=x1+penumbra)&&(y>=y0-penumbra)&&(y<=y1+penumbra));
}

void DSGLSlider::renderQt(QPainter &painter, int x, int y, int width, int height, float thresh)
{
  painter.save();
  x0=x;x1=x+width;y0=y;y1=y+height;
  int /*float*/ yPos=y+(int)(height*(1-thresh));
  QRect rect(x0,y0,width,height);
  QLinearGradient gradient(rect.topLeft(), rect.bottomLeft());
  gradient.setColorAt(0, QColor(color0.x*255,color0.y*255,color0.z*255));
  gradient.setColorAt(1, QColor(color1.x*255,color1.y*255,color1.z*255));
  painter.fillRect(rect, gradient);
  QPen pen;
  pen.setWidth(2);
  pen.setColor(QColor(frameColor.x*255,frameColor.y*255,frameColor.z*255));
  painter.setPen(pen);
  painter.drawRect(rect);
  glColor3fv(&indicatorColor.x);
  QPen pen2;
  pen2.setColor(QColor(indicatorColor.x*255,indicatorColor.y*255,indicatorColor.z*255));
  pen2.setWidth(2);
  painter.setPen(pen2);
  QPointF p0(x+triangleOffset, yPos);
  QPointF p1(x-triangleSize+triangleOffset, yPos+triangleSize);
  QPointF p2(x-triangleSize+triangleOffset, yPos-triangleSize);
  painter.drawLine(p0,p1);
  painter.drawLine(p1,p2);
  painter.drawLine(p2,p0);

  painter.drawLine(QPoint(x0,yPos),QPoint(x1,yPos));
  painter.restore();
}

void DSGLSlider::render(int x, int y, int width, int height, float thresh)
{
  x0=x;x1=x+width;y0=y;y1=y+height;
  int /*float*/ yPos=y+(int)(height*(1-thresh));
  glBegin(GL_QUADS);
  glColor3fv(&color0.x);
  glVertex2i(x0,y0);
  glVertex2i(x1,y0);
  glColor3fv(&color1.x);
  glVertex2i(x1,y1);
  glVertex2i(x0,y1);
  glEnd();
  glColor3fv(&frameColor.x);
  glBegin(GL_LINE_LOOP);
  glVertex2i(x0, y0);
  glVertex2i(x1, y0);
  glVertex2i(x1, y1);
  glVertex2i(x0, y1);
  glEnd();
  glColor3fv(&indicatorColor.x);
  glBegin(GL_TRIANGLES);
  glVertex2i(x+triangleOffset, yPos);
  glVertex2i(x-triangleSize+triangleOffset, yPos+triangleSize);
  glVertex2i(x-triangleSize+triangleOffset, yPos-triangleSize);
  glEnd();
  glBegin(GL_LINES);
  glVertex2i(x0, yPos);
  glVertex2i(x1, yPos);
  glEnd();
}
