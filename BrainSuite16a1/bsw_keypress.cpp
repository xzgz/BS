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

#include "brainsuitewindow.h"
#include "ui_brainsuitewindow.h"
#include <qevent.h>
#include <imagewidget.h>
#include <dsglwidget.h>
#include <imageframe.h>
#include <delineationtoolboxform.h>
#include <brainsuitesettings.h>
#include <diffusiontoolboxform.h>
#include <surfacealpha.h>
#include <imagedisplaypropertiesform.h>
#include <surfacedisplayform.h>
#include <curvetoolboxform.h>
#include <gotocoordinatedialog.h>
#include <qclipboard.h>

DSPoint defaultRotations[] =
{
  DSPoint(0,0,0),
  DSPoint(0,180,0),
  DSPoint(270,0,0),
  DSPoint(270,180,0),
  DSPoint(270,270,0),
  DSPoint(270,90,0)
};

void toggleWireframe(BrainSuiteDataManager::SurfaceHandle &handle)
{
  if (handle.surface) handle.surface->wireFrame^=true;
}

void cycleClipping(BrainSuiteSettings::Clipping &clipState)
{
  switch (clipState)
  {
    case BrainSuiteSettings::Off : clipState = BrainSuiteSettings::Positive; break;
    case BrainSuiteSettings::Positive : clipState = BrainSuiteSettings::Negative; break;
    case BrainSuiteSettings::Negative : clipState = BrainSuiteSettings::Off; break;
  }
}

void BrainSuiteWindow::setShowVolume(bool state)
{
  imageState.viewVolume = state;
  for (size_t i=0;i<ports.size();i++)
    if (ports[i])
    {
      ports[i]->image()->imageState.viewVolume = imageState.viewVolume;
    }
  updateImages();
  updateSurfaceView();
  if (toolManager.imageDisplayPropertiesForm)
    toolManager.imageDisplayPropertiesForm->updateDisplay();
}

void BrainSuiteWindow::setShowOverlay1(bool state)
{
  imageState.viewOverlay = state;
  for (size_t i=0;i<ports.size();i++)
    if (ports[i])
    {
      ports[i]->image()->imageState.viewOverlay = imageState.viewOverlay;
    }
  updateImages();
  updateSurfaceView();
  if (toolManager.imageDisplayPropertiesForm)
    toolManager.imageDisplayPropertiesForm->updateDisplay();
}

void BrainSuiteWindow::setShowOverlay2(bool state)
{
  imageState.viewOverlay2 = state;
  for (size_t i=0;i<ports.size();i++)
    if (ports[i])
    {
      ports[i]->image()->imageState.viewOverlay2 = imageState.viewOverlay2;
    }
  updateImages();
  updateSurfaceView();
  if (toolManager.imageDisplayPropertiesForm)
    toolManager.imageDisplayPropertiesForm->updateDisplay();
}

void BrainSuiteWindow::setShowLabels(bool state)
{
  imageState.viewLabels = state;
  for (size_t i=0;i<ports.size();i++)
    if (ports[i])
      ports[i]->image()->imageState.viewLabels = imageState.viewLabels;
  updateImages();
  updateSurfaceView();
  if (toolManager.imageDisplayPropertiesForm)
    toolManager.imageDisplayPropertiesForm->updateDisplay();
}

void BrainSuiteWindow::setShowMask(bool state)
{
  imageState.viewMask = state;
  for (size_t i=0;i<ports.size();i++)
    if (ports[i])
      ports[i]->image()->imageState.viewMask = imageState.viewMask;
  updateImages();
  updateMenuChecks();
}

void BrainSuiteWindow::setShowLabelOutline(bool state)
{
  imageState.viewLabelOutline = state;
  for (size_t i=0;i<ports.size();i++)
    if (ports[i])
      ports[i]->image()->imageState.viewLabelOutline = imageState.viewLabelOutline;
  updateImages();
  updateSurfaceView();
  if (toolManager.imageDisplayPropertiesForm)
    toolManager.imageDisplayPropertiesForm->updateDisplay();
}

void BrainSuiteWindow::resetClippingAndPosition()
{
  for (size_t i=0;i<ports.size();i++)
    if (ports[i])
      ports[i]->image()->resetOffsets();
  updateImages();
  dsglWidget->glViewController.properties.xShift=0;
  dsglWidget->glViewController.properties.yShift=0;
  BrainSuiteSettings::clipXPlane = BrainSuiteSettings::Off;
  BrainSuiteSettings::clipYPlane = BrainSuiteSettings::Off;
  BrainSuiteSettings::clipZPlane = BrainSuiteSettings::Off;
  updateSurfaceView();
}

template <class T>
IPoint3D findFirstLabel(Vol3D<T> &vLabel, int labelID)
{
  const int cx=vLabel.cx;
  const int cy=vLabel.cy;
  const int cz=vLabel.cz;
  int i=0;
  for (int z=0;z<cz;z++)
    for (int y=0;y<cy;y++)
      for (int x=0;x<cx;x++)
        if (vLabel[i++]==labelID)
        {
          return IPoint3D(x,y,z);
        }
  return IPoint3D(-1,-1,-1);
}

void BrainSuiteWindow::moveToFirstVoxelLabeled(int labelID)
{
  if (brainSuiteDataManager.vLabel)
  {
    IPoint3D p(-1,-1,-1);
    switch (brainSuiteDataManager.vLabel->typeID())
    {
      case SILT::Uint8: p=findFirstLabel(*(Vol3D<uint8>*)brainSuiteDataManager.vLabel,labelID); break;
      case SILT::Uint16: p=findFirstLabel(*(Vol3D<uint16>*)brainSuiteDataManager.vLabel,labelID); break;
      case SILT::Sint16: p=findFirstLabel(*(Vol3D<sint16>*)brainSuiteDataManager.vLabel,labelID); break;
      default: return;
    }
    if (p.x>=0)moveTo(p);
  }
}

void BrainSuiteWindow::setActivePort(const int nextPort)
{
  if (nextPort<0) activePort=0;
  if (nextPort>(int)ports.size()) activePort=(int)ports.size()-1;
}

void BrainSuiteWindow::setSinglePort(int portNo)
{
  bool returningToSameView=false;
  if (portNo<0||portNo>=(int)ports.size())
  {
    for (size_t i=0;i<ports.size();i++)
      if (ports[i])
        ports[i]->show();
    if (dsglWidget)
    {
      dsglWidget->show();
      dsglWidget->update();
    }
  }
  else
  {
    ImageFrame *port = ports[portNo];
    if (port)
    {
      switch (viewMode)
      {
        case Axial : if (port->image()->orientation==Orientation::Axial) return; break;
        case Coronal : if (port->image()->orientation==Orientation::Coronal) return; break;
        case Sagittal : if (port->image()->orientation==Orientation::Sagittal) return; break;
        case Ortho3D : break;
        default: break;
      }
      ViewMode newViewMode(NoView);
      switch (port->image()->orientation)
      {
        case Orientation::Axial: newViewMode=Axial; break;
        case Orientation::Coronal: newViewMode=Coronal; break;
        case Orientation::Sagittal: newViewMode=Sagittal; break;
        default: break;
      }
      returningToSameView = (lastViewMode==newViewMode&&viewMode==SurfaceOnly);
      lastViewMode=viewMode;
      viewMode=newViewMode;
    }
    for (size_t i=0;i<ports.size();i++)
      if (ports[i] && (int)i!=portNo) ports[i]->hide();
    if (ports[portNo]) ports[portNo]->show();
    if (dsglWidget && popoutSurfaceView)
    {
      dsglWidget->update();
    }
    else
      dsglWidget->hide();
  }
  ImageWidget::blockPainting=true;
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents); // this is to resize the frames only, so don't redraw
  ImageWidget::blockPainting=false;
  if (returningToSameView)
  {
    updateImages();
    return;
  }
  if (BrainSuiteSettings::zoomBestFitOnResize)
    zoomBestFit(); // this will redraw
  else
  {
    for (size_t i=0;i<ports.size();++i)
      if (ports[i])
        ports[i]->image()->updateOffsets();
    updateImages();
  }
}

inline uint16 keymap(uint16 ucs)
{
  switch (ucs) {
    case 161 : return '1';
    case 8482 : return '2';
    case 163 : return '3';
    case 162 : return '4';
    case 8734 : return '5';
    case 167 : return '6';
    case 182 : return '7';
    case 8226 : return '8';
    case 170 : return '9';
    case 186 : return '0';
  }
  return ucs;
}

void BrainSuiteWindow::keyPressEvent ( QKeyEvent * event )
{
  if (!event) return;
  bool ctrlkey = event->modifiers()&(Qt::ControlModifier|Qt::MetaModifier);
  bool altkey = event->modifiers()&Qt::AltModifier;
  bool shiftkey = event->modifiers()&Qt::ShiftModifier;
  int ascii=(event->key()<255) ? event->key() : 0;
  QChar a=QChar(event->text()[0]);
  if (a.unicode()>=128) { ascii=keymap(a.unicode()); }
  if (ascii>255) ascii=0;
  if (::isupper(ascii)&&!shiftkey) ascii=::tolower(ascii);
  if (altkey)
  {
    if (ascii==0)
    {
      ImageWidget *imageWidget = (activePort>=0 && ports[activePort] && ports[activePort]->image()) ? ports[activePort]->image() : 0;
      switch (event->key())
      {
        case Qt::Key_Left : shiftImageViews(imageWidget,QPoint(-5,0)); break;
        case Qt::Key_Right: shiftImageViews(imageWidget,QPoint(+5,0)); break;
        case Qt::Key_Up   : shiftImageViews(imageWidget,QPoint(0,-5)); break;
        case Qt::Key_Down : shiftImageViews(imageWidget,QPoint(0,+5)); break;
        default:
          break;
      }
      setCursor(Qt::OpenHandCursor);
    }
    switch (ascii)
    {
      case '1' :
        if (dsglWidget) dsglWidget->setRotation(defaultRotations[0]);
        event->setAccepted(true);
        break;
      case '2' :
        if (dsglWidget) dsglWidget->setRotation(defaultRotations[1]);
        event->setAccepted(true);
        break;
      case '3' :
        if (dsglWidget) dsglWidget->setRotation(defaultRotations[2]);
        event->setAccepted(true);
        break;
      case '4' :
        if (dsglWidget) dsglWidget->setRotation(defaultRotations[3]);
        event->accept();
        break;
      case '5' :
        if (dsglWidget) dsglWidget->setRotation(defaultRotations[4]);
        event->accept();
        break;
      case '6' :
        if (dsglWidget) dsglWidget->setRotation(defaultRotations[5]);
        event->accept();
        break;
      default: break;
    }
  }
  if (ascii==0)
  {
    ImageWidget *imageWidget = (activePort>=0 && ports[activePort] && ports[activePort]->image()) ? ports[activePort]->image() : 0;
    int lrPort=-1;
    int udPort=-1;
    if (ports.size()==3)
    {
      switch (activePort)
      {
        case 0 : lrPort=1; udPort=2; break;
        case 1 : lrPort=0; udPort=2; break;
        case 2 : lrPort=1; udPort=0; break;
      }
    }
    ImageWidget *imageWidgetLR=(activePort>=0 && lrPort>=0 && ports[lrPort]) ? ports[lrPort]->image() : 0;
    ImageWidget *imageWidgetUD=(activePort>=0 && udPort>=0 && ports[udPort]) ? ports[udPort]->image() : 0;
    bool needToUpdateScreenView=false;
    if (imageWidget)
    {
      switch (event->key())
      {
        case Qt::Key_Up   : if (!shiftkey) { imageWidget->keyUp(1); needToUpdateScreenView=true; } break;
        case Qt::Key_Down : if (!shiftkey) { imageWidget->keyDown(1); needToUpdateScreenView=true; } break;
        case Qt::Key_PageUp : imageWidget->keyUp(5); needToUpdateScreenView=true; break;
        case Qt::Key_PageDown : imageWidget->keyDown(5); needToUpdateScreenView=true; break;
      }
    }
    if (imageWidgetLR)
    {
      switch (event->key())
      {
        case Qt::Key_Left : imageWidgetLR->keyUp(1); needToUpdateScreenView=true; break;
        case Qt::Key_Right : imageWidgetLR->keyDown(1); needToUpdateScreenView=true; break;
      }
    }
    if (imageWidgetUD && shiftkey)
    {
      switch (event->key())
      {
        case Qt::Key_Up   : if (shiftkey) imageWidgetUD->keyUp(1); needToUpdateScreenView=true; break;
        case Qt::Key_Down : if (shiftkey) imageWidgetUD->keyDown(1); needToUpdateScreenView=true; break;
      }
    }
    if (needToUpdateScreenView && ctrlkey && toolManager.diffusionToolboxForm)
    {
      toolManager.diffusionToolboxForm->moveSelectedROIto(currentVolumePosition);
    }
  }
  if (ctrlkey)
  {
    switch (ascii)
    {
      case '1' : setSinglePort(0); break;
      case '2' : setSinglePort(1); break;
      case '3' : setSinglePort(2); break;
      case '4' : setSinglePort(-1); break;
      case '[' :
        break;
      case ']' :
        break;
      case 'A': // ctrl
      case 'a':
        autoscaleImages();
        break;
      case 'b' :
      case 'B' :
        if (brainSuiteDataManager.previousSurface())
        {
          updateSurfaceView();
          toolManager.surfaceDisplayForm->selectShownSurfaces();
          toolManager.surfaceDisplayForm->updateSurfaceSelection();
        }
        break;
      case 'c' :
      case 'C' :
        {
          if (altkey && (activePort>=0))
          {
            ImageFrame *p=ports[activePort];
            if (p)
            {

              QApplication::clipboard()->setImage(p->image()->blendBitmap.image);
              break;
            }
          }
          if (dsglWidget)
          {
            QImage im=dsglWidget->grabFrameBuffer();
            QApplication::clipboard()->setImage(im);
            break;
          }
          break;
        }
      case 'F' :
      case 'f' :
        if (brainSuiteDataManager.nextSurface())
        {
          updateSurfaceView();
          toolManager.surfaceDisplayForm->selectShownSurfaces();
          toolManager.surfaceDisplayForm->updateSurfaceSelection();
        }
        break;
      case 'G' :
      case 'g' :
        {
          GotoCoordinateDialog dlg(this);
          dlg.exec();
        }
        break;
      case 'L':
        setShowLabelOutline(imageState.viewLabelOutline != true);
        break;
      case 'l':
        setShowLabels(imageState.viewLabels != true);
        break;
      case 'M':
        imageState.maskMode = ImageState::MaskMode((int)imageState.maskMode + 1);
        if (imageState.maskMode>ImageState::MaskOnly)
          imageState.maskMode = ImageState::Off;
        imageState.viewMask = imageState.maskMode!=0;
        for (size_t i=0;i<ports.size();i++)
          if (ports[i])
          {
            ports[i]->image()->imageState.viewMask = imageState.viewMask;
            ports[i]->image()->imageState.maskMode = imageState.maskMode;
          }
        updateMenuChecks();
        updateImages();
        break;
      case 'm':				// toggleMask
        setShowMask(imageState.viewMask != true);
        break;
      case 'N' :
      case 'n' :
        if (toolManager.delineationToolboxForm && toolManager.delineationToolboxForm->isVisible())
        {
          toolManager.delineationToolboxForm->newVolume();
        }
        break;
      case 'P' :
        on_actionSave_Custom_Sized_Rendering_triggered();
        break;
      case 'p' :
        on_actionSave_Surface_View_Screenshow_triggered();
        break;
      case 'D':
      case 'd':
        break;
      case 'R':
      case 'r':
        setShowOverlay1(imageState.viewOverlay != true);
        break;
      case 'T':
      case 't':
        setShowOverlay2(imageState.viewOverlay2 != true);
        break;
      case 's' :
      case 'S' :
        setShowSurfaces(!BrainSuiteSettings::showSurfaces);
        event->accept();
        break;
      case 'V' :
      case 'v' :
        BrainSuiteSettings::renderSlices ^= true;
        if (toolManager.surfaceDisplayForm)
          toolManager.surfaceDisplayForm->updateSliceViewStatus();
        updateSurfaceView();
        event->accept();
        break;
      case 'U':
      case 'u':
        BrainSuiteSettings::showOrientation^=true;
        break;
      case 'X':
      case 'x':
        imageState.showCursor ^= true;
        for (size_t i=0;i<ports.size();i++) if (ports[i]) ports[i]->image()->imageState.showCursor = imageState.showCursor;
        updateImages();
        break;
      case 'Y':
      case 'y':
        break;
      case 'Z':
      case 'z':
        if (toolManager.delineationToolboxForm)
        {
          toolManager.delineationToolboxForm->undoLastAction();
        }
        break;
    }
  }
  else
  {
    switch(ascii)
    {
      case '=' :
      case '+' : zoomIn(); break;
      case '_' :
      case '-' : zoomOut(); break;
      case '*' : zoomBestFit(); break;
      case '.' : zoomSmallest(); break;
      case '/' : zoomUnity(); break;
      case '<' : if (toolManager.delineationToolboxForm && toolManager.delineationToolboxForm->isVisible())
        { toolManager.delineationToolboxForm->decrementLabel(); break; }
        else { ImageWidget::curveCutoff-=0.25f; if (ImageWidget::curveCutoff<0) ImageWidget::curveCutoff = 0; updateImages(); }
        break;
      case '>' : if (toolManager.delineationToolboxForm && toolManager.delineationToolboxForm->isVisible())
        {	toolManager.delineationToolboxForm->incrementLabel(); break; }
        else { ImageWidget::curveCutoff-=0.25f; if (ImageWidget::curveCutoff<0) ImageWidget::curveCutoff = 0; updateImages(); }
        break;
      case ']' :
        if (toolManager.delineationToolboxForm) toolManager.delineationToolboxForm->increaseBrushSize();
        break;
      case '[' :
        if (toolManager.delineationToolboxForm) toolManager.delineationToolboxForm->decreaseBrushSize();
        break;
      case 'A' : if (ports.size()) { activePort--; if (activePort<0) activePort=(int)ports.size()-1; updateImages(); } break;
      case 'a' : if (ports.size()) { activePort++; if (activePort>=(int)ports.size()) activePort=0; updateImages(); } break;
      case 'B' :
      case 'b' :
        break;
      case 'c' :
      case 'C' : on_action_Curve_Tool_triggered(); break;
      case 'd' :
      case 'D' : showDiffusionToolbox(); break;
      case 'e' :
      case 'E' :
        {
          ui->delineationToolboxDockWidget->show();
          ui->delineationToolboxDockWidget->raise();
          if (toolManager.delineationToolboxForm)
            toolManager.delineationToolboxForm->toggleEditState();
        }
        break;
      case 'f' :
      case 'F' : setShowFibers(!BrainSuiteSettings::showFibers); event->accept(); break;
      case 'G' :
      case 'g' : setShowTensors(!BrainSuiteSettings::showTensors); event->accept(); break;
      case 'H' :
      case 'h' : resetClippingAndPosition(); break;
      case 'i' :
      case 'I' : on_action_Image_Display_Properties_triggered(); event->accept(); break;
      case 'L':
      case 'l': setShowLighting(!BrainSuiteSettings::useLighting); break;
      case 'm' :
      case 'M' : on_action_Mask_Tool_triggered(); event->accept(); break;
      case 'o' :
      case 'O' : setShowODFs(!BrainSuiteSettings::showODFs); event->accept(); break;
      case 'p' :
      case 'P' :
        ui->delineationToolboxDockWidget->show();
        ui->delineationToolboxDockWidget->raise();
        if (toolManager.delineationToolboxForm)
          toolManager.delineationToolboxForm->showPainterTool();
        event->accept();
        break;
      case 's' :
      case 'S' : showSurfaceDisplayProperties(); break;
      case 'v' :
      case 'V' :
        break;
      case 'W' :
      case 'w' :
        std::for_each(brainSuiteDataManager.surfaces.begin(),brainSuiteDataManager.surfaces.end(),toggleWireframe);
        if (toolManager.surfaceDisplayForm)
          toolManager.surfaceDisplayForm->updateSurfaceSelection();
        updateSurfaceView();
        BrainSuiteSettings::fiberWireframe ^= true;
        break;
      case 'X' :
      case 'x' :
        cycleClipping(BrainSuiteSettings::clipXPlane);
        updateSurfaceView();
        event->accept();
        break;
      case 'Y' :
      case 'y' :
        cycleClipping(BrainSuiteSettings::clipYPlane);
        updateSurfaceView();
        event->accept();
        break;
      case 'Z' :
      case 'z' :
        cycleClipping(BrainSuiteSettings::clipZPlane);
        updateSurfaceView();
        event->accept();
        break;
    }
  }
  updateMenuChecks();
}

