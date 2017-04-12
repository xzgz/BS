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

#include "connectivityglwidget.h"
#include <brainsuitewindow.h>
#include <QWheelEvent>
#include <connectivityrenderer.h>
#include <connectivitymap.h>
#include <diffusiontoolboxform.h>
#include <DS/timer.h>
#include <brainsuitesettings.h>
#include <trackfilter.h>

ConnectivityRenderer connectivityRenderer;

ConnectivityGLWidget::ConnectivityGLWidget(BrainSuiteWindow *parent) :
  QGLWidget(QGLFormat(QGL::SampleBuffers), parent),
  brainSuiteWindow(parent), computing(false), sliding(false)
{
}

void ConnectivityGLWidget::initializeGL()
{
  glFrame.clearColor = connectivityRenderer.clearColor;
  glFrame.initialize();
  connectivityRenderer.initialize();
}

void ConnectivityGLWidget::resizeEvent(QResizeEvent * /* event */)
{
}

void ConnectivityGLWidget::wheelEvent( QWheelEvent * event )
{
  int numDegrees = event->delta() / 8;
  int numSteps = numDegrees / 15;
  connectivityRenderer.weightThreshold += numSteps*0.01f;
  if (connectivityRenderer.weightThreshold<0) connectivityRenderer.weightThreshold=0;
  if (connectivityRenderer.weightThreshold>1) connectivityRenderer.weightThreshold=1;
  update();
}

void ConnectivityGLWidget::paintEvent(QPaintEvent * /*event*/)
{
  if (!brainSuiteWindow) return; // should not happen, but just in case...
  ::connectivityRenderer.retinaScale = devicePixelRatio();
  QPainter p(this);
  p.beginNativePainting();
  connectivityRenderer.render(p,brainSuiteWindow->brainSuiteDataManager.connectivityMap,
                                brainSuiteWindow->brainSuiteDataManager.labelDescriptionSet.labelDetails);
  p.endNativePainting();
}

void ConnectivityGLWidget::mousePressEvent(QMouseEvent *event)
{
  grabMouse();
  if (event->buttons() & Qt::LeftButton)
  {
    QPoint p=event->pos();
    sliding = connectivityRenderer.inSlider(p.x(),p.y());
    mouseMoveEvent(event);
  }
}

void ConnectivityGLWidget::mouseReleaseEvent(QMouseEvent * event)
{
  releaseMouse();
  if (event->buttons() & Qt::LeftButton)
  {
    sliding=false;
    brainSuiteWindow->updateSurfaceView();
    update();
  }
}

void ConnectivityGLWidget::mouseMoveEvent(QMouseEvent *event)
{
  QPoint point = event->pos();
  if (sliding)
  {
    connectivityRenderer.moveSlider(point.x(),point.y());
    update();
  }
  else
  {
    bool stateChange = false;
    if (event->modifiers()&Qt::ShiftModifier)
    {
      stateChange = connectivityRenderer.addSelection(brainSuiteWindow->brainSuiteDataManager.connectivityMap,point.x(),point.y());
      connectivityRenderer.groupMode = false;
    }
    else if (event->modifiers()&Qt::ControlModifier)
    {
      stateChange = connectivityRenderer.addSecondSelection(brainSuiteWindow->brainSuiteDataManager.connectivityMap,point.x(),point.y());
      connectivityRenderer.groupMode = true;
    }
    else
      stateChange = connectivityRenderer.makeSelection(brainSuiteWindow->brainSuiteDataManager.connectivityMap,point.x(),point.y());
    if (stateChange)
    {
      regroup();
    }
    update();
  }
}

//TODO: move to track filter
FiberTrackSet *pickSubset(FiberTrackSet *curveSet, Vol3DBase *vLabel, int labelID, float lengthThreshold);
FiberTrackSet *pickSubset(FiberTrackSet *curveSet, Vol3DBase *vLabel, std::vector<int> &selection, float lengthThreshold, bool opAnd);
FiberTrackSet *pickSubset(FiberTrackSet *curveSet, Vol3DBase *vLabel, int labelID, float lengthThreshold)
{
  if (!curveSet) return 0;
  if (!vLabel) return 0;
  FiberTrackSet *subset = 0;
  switch (vLabel->typeID())
  {
    case SILT::Uint8 : subset = TrackFilter::filterTractsByLabelAndLength(*(Vol3D<uint8 > *)vLabel,curveSet,labelID,lengthThreshold); break;
    case SILT::Uint16: subset = TrackFilter::filterTractsByLabelAndLength(*(Vol3D<uint16> *)vLabel,curveSet,labelID,lengthThreshold); break;
    case SILT::Sint16: subset = TrackFilter::filterTractsByLabelAndLength(*(Vol3D<sint16> *)vLabel,curveSet,labelID,lengthThreshold); break;
    case SILT::Uint32: subset = TrackFilter::filterTractsByLabelAndLength(*(Vol3D<uint32> *)vLabel,curveSet,labelID,lengthThreshold); break;
    case SILT::Sint32: subset = TrackFilter::filterTractsByLabelAndLength(*(Vol3D<sint32> *)vLabel,curveSet,labelID,lengthThreshold); break;
    default: break;
  }
  return subset;
}

FiberTrackSet *pickSubset(FiberTrackSet *curveSet, Vol3DBase *vLabel, std::vector<int> &selection, float lengthThreshold, bool opAnd)
{
  if (!curveSet) return 0;
  if (!vLabel) return 0;
  FiberTrackSet *subset = 0;
  std::vector<int> excludes;
  switch (vLabel->typeID())
  {
    case SILT::Uint8 : subset = TrackFilter::filterTractsByLabelAndLength(*(Vol3D<uint8 > *)vLabel,curveSet,selection,excludes,lengthThreshold,opAnd); break;
    case SILT::Uint16: subset = TrackFilter::filterTractsByLabelAndLength(*(Vol3D<uint16> *)vLabel,curveSet,selection,excludes,lengthThreshold,opAnd); break;
    case SILT::Sint16: subset = TrackFilter::filterTractsByLabelAndLength(*(Vol3D<sint16> *)vLabel,curveSet,selection,excludes,lengthThreshold,opAnd); break;
    case SILT::Uint32: subset = TrackFilter::filterTractsByLabelAndLength(*(Vol3D<uint32> *)vLabel,curveSet,selection,excludes,lengthThreshold,opAnd); break;
    case SILT::Sint32: subset = TrackFilter::filterTractsByLabelAndLength(*(Vol3D<sint32> *)vLabel,curveSet,selection,excludes,lengthThreshold,opAnd); break;
    default: break;
  }
  return subset;
}

Vol3DBase *ConnectivityGLWidget::vLabel()
{
  if (!brainSuiteWindow) return 0;
  if (brainSuiteWindow->brainSuiteDataManager.connectivityProperties.vMergedLabels.size()) return &brainSuiteWindow->brainSuiteDataManager.connectivityProperties.vMergedLabels;
  return brainSuiteWindow->brainSuiteDataManager.vLabel;
}

FiberTrackSet *ConnectivityGLWidget::getSubgroup()
{
  std::vector<int> selection;
  if (connectivityRenderer.getSelectedLabels(selection,brainSuiteWindow->brainSuiteDataManager.connectivityMap))
  {

    if (brainSuiteWindow->brainSuiteDataManager.fiberTrackSet && brainSuiteWindow->brainSuiteDataManager.vLabel)
    {
      const float lengthThreshold=(brainSuiteWindow->toolManager.diffusionToolboxForm) ? brainSuiteWindow->toolManager.diffusionToolboxForm->getLengthThreshold() : 0.0f;
      FiberTrackSet *subset = pickSubset(brainSuiteWindow->brainSuiteDataManager.fiberTrackSet,vLabel(),selection,lengthThreshold,connectivityRenderer.groupMode);
      std::ostringstream ostr;
      connectivityRenderer.statusMessage = ostr.str();
      return subset;
    }
  }
  return 0;
}

FiberTrackSet *ConnectivityGLWidget::applyFilter(FiberTrackSet *tractSet)
{
  std::vector<int> selection;
  if (connectivityRenderer.getSelectedLabels(selection,brainSuiteWindow->brainSuiteDataManager.connectivityMap))
  {
    if (tractSet && brainSuiteWindow->brainSuiteDataManager.vLabel)
    {
      Timer t; t.start();
      FiberTrackSet *subset = pickSubset(tractSet,vLabel(),selection,0,connectivityRenderer.groupMode);
      t.stop();
      std::ostringstream ostr;
      connectivityRenderer.statusMessage = ostr.str();
      return subset;
    }
  }
  return 0;
}


void ConnectivityGLWidget::regroup()
{
  if (brainSuiteWindow->toolManager.diffusionToolboxForm && brainSuiteWindow->toolManager.diffusionToolboxForm->isVisible())
    brainSuiteWindow->toolManager.diffusionToolboxForm->applyCurrentFilters();
  else
  {
    FiberTrackSet *subset = getSubgroup();
    if (subset)
    {
      delete brainSuiteWindow->brainSuiteDataManager.fiberTrackSubset;
      brainSuiteWindow->brainSuiteDataManager.fiberTrackSubset = subset;
      BrainSuiteSettings::showTrackSubset = true;
      brainSuiteWindow->updateSurfaceView();
    }
    else
    {
      BrainSuiteSettings::showTrackSubset = false;
      brainSuiteWindow->updateSurfaceView();
    }
  }
}

