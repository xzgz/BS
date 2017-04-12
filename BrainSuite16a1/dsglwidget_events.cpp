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

#include "dsglwidget.h"
#include <QMouseEvent>
#include <ipoint3d.h>
#include <brainsuitewindow.h>
#include <dspoint.h>
#include <curvetoolboxform.h>

void DSGLWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
  if (brainSuiteWindow)
  {
    DSPoint point=getOGLPos(event->pos());
    brainSuiteWindow->moveToWC(point, event);
  }
}

void DSGLWidget::mousePressEvent(QMouseEvent *event)
{
  blockSpin=true;
  grabMouse();
  hasMouse=true;
  if (event->buttons() & Qt::LeftButton)
  {
    glViewController.setMousePoint(event->pos());
    glViewController.leftButtonDown(event->pos());
  }
}

void DSGLWidget::mouseMoveEvent(QMouseEvent *event)
{
  glViewController.retinaScale = devicePixelRatio();
  tracing=false;
  if (glViewController.mouseMove(event->buttons()|event->modifiers(),(event->pos())))
  {
    update();
  }
  previousPosition = event->pos();
}

void DSGLWidget::mouseReleaseEvent(QMouseEvent * event)
{
  releaseMouse();
  hasMouse=false;
  if (event->buttons() & Qt::LeftButton)
  {
    glViewController.setMousePoint(event->pos().x(),event->pos().y());
  }
  blockSpin=false;
  update();
}

void DSGLWidget::wheelEvent( QWheelEvent * event )
{
  int n = abs(event->delta()/120);
  for (int i=0;i<n;i++)
    glViewController.properties.zoom *= (((event->delta())<0) ? 1.2f : 1.0f/1.2f);
  updateGL();
}

