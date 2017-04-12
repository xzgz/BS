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

#include <silttypes.h>
#include <qgl.h>
#include "brainsuitewindow.h"
#include "ui_brainsuitewindow.h"
#include <iostream>
#include <QtGui>
#include <dsglwidget.h>
#include <dsglviewdialog.h>
#include <surfacedisplayform.h>
#include <imageframe.h>
#include <imagewidget.h>
#include <brainsuiteinstalldialog.h>
#include <brainsuiteqtsettings.h>
#include <brainsuitesettings.h>
#include <delineationtoolboxform.h>
#include <diffusiontoolboxform.h>
#include <qmessagebox.h>
#include <displaycode.h>
#include <imagedisplaypropertiesform.h>
#include <waitcursor.h>
#include <protocolcurveset.h>
#include <corticalextractionsequencedialog.h>
#include <aboutbrainsuitedialog.h>
#include <svregistrationdialog.h>
#include <iomanip>
#include <brainsuitewindow_t.h>
#include <skullstrippingdialog.h>
#include <skullandscalpdialog.h>
#include <nonuniformitycorrectiondialog.h>
#include <tissueclassificationdialog.h>
#include <cerebrumextractiondialog.h>
#include <cortexidentificationdialog.h>
#include <scrubmaskdialog.h>
#include <topologicalcorrectiondialog.h>
#include <dewispdialog.h>
#include <innercorticalsurfacedialog.h>
#include <pialsurfacedialog.h>
#include <splithemispheresdialog.h>
#include <svregguitool.h>
#include <curvetoolboxform.h>
#include <hardirenderer.h>
#include <connectivityviewdialog.h>
#include <computerange.h>


// TODO: move to appropriate tool manager
extern SVRegGUITool svregGUITool;
HARDITool hardiTool;
HARDIRenderer hardiRenderer(hardiTool);

void BrainSuiteWindow::clearMRUList()
{
}

void BrainSuiteWindow::showSurfaceDisplayProperties()
{
  ui->surfaceDisplayPropertiesDockWidget->show();
  ui->surfaceDisplayPropertiesDockWidget->raise();
}

void BrainSuiteWindow::showDiffusionToolbox()
{
  ui->diffusionToolboxDockWidget->show();
  ui->diffusionToolboxDockWidget->raise();
}

void BrainSuiteWindow::updateSurfaceDisplayLists()
{
  if (toolManager.surfaceDisplayForm)
    toolManager.surfaceDisplayForm->updateSurfaceList();
}

void BrainSuiteWindow::detachDSGLView()
{
  popoutSurfaceView = false;
  ui->action_Popout_Surface_View->setChecked(popoutSurfaceView);
  if (toolManager.dsglViewDialog)
  {
    toolManager.dsglViewDialog->hide();
    toolManager.dsglViewDialog->dettach(dsglWidget);
  }
  ui->gridLayout->addWidget(dsglWidget,1,1);
}

void BrainSuiteWindow::updateMenuChecks()
{
  ui->actionShow_Volume->setChecked(imageState.viewVolume);
  ui->actionShow_Overlay_1->setChecked(imageState.viewOverlay);
  ui->actionShow_Overlay_2->setChecked(imageState.viewOverlay2);
  ui->actionShow_Label_Overlay->setChecked(imageState.viewLabels);
  ui->actionScreen_with_Mask_Volume->setChecked(imageState.viewMask);
  ui->action_Continuous_sync_with_image_view->setChecked(viewSettings.syncGL);
  ui->action_Stencil->setChecked(imageState.maskMode==ImageState::Stencil);
  ui->action_Blend->setChecked(imageState.maskMode==ImageState::Blend);
  ui->action_Outline->setChecked(imageState.maskMode==ImageState::Outline);
  ui->action_Mask_Only->setChecked(imageState.maskMode==ImageState::MaskOnly);
  ui->actionDisable_Mask_Undo_System->setChecked(BrainSuiteSettings::maskUndoDisabled);
  ui->actionAutozoom_on_Resize->setChecked(BrainSuiteSettings::zoomBestFitOnResize);
  ui->actionAutozoom_on_Resize->setChecked(imageState.viewLabelOutline);
}

ImageWidget *BrainSuiteWindow::activeImageWidget()
{
  if (activePort<0) return 0;
  if (activePort>=(int)ports.size()) return 0;
  return  (ports[activePort]!=0) ? ports[activePort]->image() : 0;
}

bool BrainSuiteWindow::alwaysEditMask()
{
  return (toolManager.delineationToolboxForm && toolManager.delineationToolboxForm->alwaysEditMask());
}

bool BrainSuiteWindow::editingLabels()
{
  return (toolManager.delineationToolboxForm && toolManager.delineationToolboxForm->isEditingLabels());
}

bool BrainSuiteWindow::editingMask()
{
  return (toolManager.delineationToolboxForm && toolManager.delineationToolboxForm->isEditingMask());
}

void BrainSuiteWindow::updateRecentFileActions()
{
  QSettings settings("BrainSuite",BrainSuiteQtSettings::brainSuiteQtKey);
  QStringList files = settings.value("recentFileList").toStringList();
  int numRecentFiles = std::min(files.size(),(int)recentFileActions.size());
  for (int i = 0; i < numRecentFiles; i++)
  {
    QString filename = QFileInfo(files[i]).fileName();
    QString text = tr("&%1 %2").arg(i + 1).arg(filename);
    recentFileActions[i]->setText(text);
    recentFileActions[i]->setData(files[i]);
    recentFileActions[i]->setVisible(true);
  }
  for (size_t j = numRecentFiles; j < recentFileActions.size(); ++j)
    recentFileActions[j]->setVisible(false);
}

void BrainSuiteWindow::createMenus()
{
  const int maxRecentFiles = 8;
  recentFileActions.resize(8,(QAction *)0);
  for (int i = 0; i < maxRecentFiles; ++i)
  {
    recentFileActions[i] = new QAction(this);
    recentFileActions[i]->setVisible(false);
    connect(recentFileActions[i], SIGNAL(triggered()),this, SLOT(openRecentFile()));
  }
  for (int i = 0; i < maxRecentFiles; ++i)
    ui->menuRecent_Volume->addAction(recentFileActions[i]);
  updateRecentFileActions();
}

IPoint3D BrainSuiteWindow::newVolumePosition(IPoint3D position)
{
  Vol3DBase *volume = brainSuiteDataManager.volume;
  if (volume)
  {
    if (position.x>(int)(volume->cx-1)) position.x=volume->cx-1;
    if (position.y>(int)(volume->cy-1)) position.y=volume->cy-1;
    if (position.z>(int)(volume->cz-1)) position.z=volume->cz-1;
    if (position.x<0) position.x=0;
    if (position.y<0) position.y=0;
    if (position.z<0) position.z=0;
  }
  return position;
}

void BrainSuiteWindow::mousePressed(ImageWidget *port, const IPoint3D &position, QMouseEvent *event)
{
  if (event->modifiers()&Qt::AltModifier)
  {
    setVolumePosition(position,true);
    setCursor(Qt::ClosedHandCursor);
  }
  else
    mouseMoveTo(port,position,event);
}

void BrainSuiteWindow::mouseDoubleClicked(ImageWidget */*port*/, const IPoint3D &/*position*/, QMouseEvent */*event*/)
{
}

void BrainSuiteWindow::mouseReleased(ImageWidget *port, const IPoint3D &position, QMouseEvent *event)
{
  setCursor(Qt::ArrowCursor);
  if (event && event->modifiers()&Qt::AltModifier)
  {
    mouseMoveTo(port,position,event);
  }
  else
  {
    if (editingLabels())
      toolManager.delineationToolboxForm->mouseReleased(port,position,event);
    else if (editingMask())
      toolManager.delineationToolboxForm->mouseReleased(port,position,event);
    else
      mouseMoveTo(port,position,event);
    updateSurfaceView();
  }
}

void BrainSuiteWindow::shiftImageViews(ImageWidget *widget, QPoint delta)
{
  if (!widget) return;
  int dx=0,dy=0,dz=0;
  switch (widget->orientation)
  {
    case Orientation::XY : dx = delta.x(); dy = delta.y(); break;
    case Orientation::XZ : dx = delta.x(); dz = delta.y(); break;
    case Orientation::YZ : dy = delta.x(); dz = delta.y(); break;
    case Orientation::Oblique : break;
  }
  for (size_t i=0;i<ports.size();i++)
    if (ports[i])
    {
      ImageWidget *widgetI=ports[i]->image();
      switch (widgetI->orientation)
      {
        case Orientation::XY : widgetI->shiftOffsets(QPoint(dx,dy)); break;
        case Orientation::XZ : widgetI->shiftOffsets(QPoint(dx,dz)); break;
        case Orientation::YZ : widgetI->shiftOffsets(QPoint(dy,dz)); break;
        case Orientation::Oblique : break;
      }
      widgetI->update();
    }
}

void BrainSuiteWindow::mouseMoveTo(ImageWidget *widget, const IPoint3D &position_, QMouseEvent *event, bool alwaysSync3D)
{
  IPoint3D position = newVolumePosition(position_);
  for (size_t i=0;i<ports.size();i++)
  {
    if (ports[i] && widget==ports[i]->image())
    {
      if (activePort != (int)i)
      {
        activePort = (int)i;
      }
      break;
    }
  }
  if (event && event->modifiers()&Qt::AltModifier)
  {
    if (widget)
    {
      setCursor(Qt::ClosedHandCursor);
      shiftImageViews(widget,event->pos()-widget->lastClick);
    }
    return;
  }
  else
  {
    setCursor(Qt::ArrowCursor);
    if (editingLabels())
      toolManager.delineationToolboxForm->mouseMoveTo(widget,position,event);
    else if (editingMask())
    {
      toolManager.delineationToolboxForm->mouseMoveTo(widget,position,event);
    }
    else if (toolManager.diffusionToolboxForm)
    {
      toolManager.diffusionToolboxForm->mouseMoveTo(widget,position,event);
    }
  }
  viewSettings.blockSync = (alwaysSync3D==false) && (viewSettings.syncGL==false);
  setVolumePosition(position,true);
  viewSettings.blockSync = false;
}


void BrainSuiteWindow::status(std::string statusMessage)
{
  statusBar()->showMessage(statusMessage.c_str());
}

void BrainSuiteWindow::changeEvent(QEvent *e)
{
  QMainWindow::changeEvent(e);
  switch (e->type()) {
  case QEvent::LanguageChange:
    ui->retranslateUi(this);
    break;
  default:
    break;
  }
}

bool BrainSuiteWindow::newLabelVolume()
{
  if (brainSuiteDataManager.volume==0)
  {
    std::cerr<<"no primary file loaded"<<std::endl;
    return false;
  }
  if (brainSuiteDataManager.vLabel)
  {
    QMessageBox msgBox(this);
    msgBox.setText("New Label Volume");
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setInformativeText("Are you sure you want to erase the current label volume?");
    msgBox.setStandardButtons(QMessageBox::Yes|QMessageBox::No);
    if (msgBox.exec()==QMessageBox::No) return false;
  }
  bool use16bits=false;
  {
    QMessageBox msgBox(this);
    msgBox.setText("Datatype Selection");
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setInformativeText("What datatype would you like to use for the new label volume?");
    QPushButton *byteButton = msgBox.addButton("8 bit",QMessageBox::AcceptRole);
    QPushButton *wordButton = msgBox.addButton("16 bit",QMessageBox::AcceptRole);
    msgBox.setStandardButtons(QMessageBox::Cancel);
    msgBox.exec();
    if (msgBox.clickedButton()==(QAbstractButton *)byteButton) use16bits = false;
    else if (msgBox.clickedButton()==(QAbstractButton *)wordButton) use16bits = true;
    else return false;
  }
  Vol3DBase *newLabelVolume = 0;
  if (use16bits)
  {
    Vol3D<uint16> *vol = new Vol3D<uint16>;
    if (vol)
    {
      vol->makeCompatible(*brainSuiteDataManager.volume);
      vol->set(0);
      newLabelVolume = vol;
    }
  }
  else
  {
    Vol3D<uint8> *vol = new Vol3D<uint8>;
    if (vol)
    {
      vol->makeCompatible(*brainSuiteDataManager.volume);
      vol->set(0);
      newLabelVolume = vol;
    }
  }
  if (newLabelVolume)
  {
    delete brainSuiteDataManager.vLabel;
    brainSuiteDataManager.vLabel = newLabelVolume;
  }
  else
  {
    std::cerr<<"could not allocate new volume"<<std::endl;
    return false;
  }
  updateView(DisplayCode::Dirty);
  return true;
}

void BrainSuiteWindow::keyReleaseEvent ( QKeyEvent * event )
{
  if (!event) return;
  bool shiftkey = event->modifiers()&Qt::ShiftModifier;
  int ascii=(event->key()<255) ? event->key() : 0;
  if (::isupper(ascii)&&!shiftkey) ascii=::tolower(ascii);
  setCursor(Qt::ArrowCursor);
}

void BrainSuiteWindow::togglePort(DSGLWidget *widget)
{
  if (!widget) return;
  if (widget==dsglWidget)
  {
    viewSettings.singleView ^= true;
    if (viewSettings.singleView)
    {
      for (size_t i=0;i<ports.size();++i) { if (ports[i]) ports[i]->setVisible(false); }
    }
    else
    {
      for (size_t i=0;i<ports.size();++i) { if (ports[i]) ports[i]->setVisible(true); }
    }
    dsglWidget->setVisible(true);
  }
}

void BrainSuiteWindow::on_actionExit_triggered()
{
  close();
}

void BrainSuiteWindow::updateImageNames()
{
  if (toolManager.imageDisplayPropertiesForm)
    toolManager.imageDisplayPropertiesForm->updateNames();
}


void BrainSuiteWindow::markImagesDirty()
{
  for (size_t i=0;i<ports.size();i++)
    if (ports[i]) ports[i]->image()->imageState.overlayAlpha = imageState.overlayAlpha;
}

void BrainSuiteWindow::setOverlay1Alpha(int alpha)
{
  imageState.overlayAlpha = (alpha<0) ? 0 : (alpha>255) ? 255 : alpha;
  for (size_t i=0;i<ports.size();i++)
    if (ports[i]) ports[i]->image()->imageState.overlayAlpha = imageState.overlayAlpha;
  updateImages();
}

void BrainSuiteWindow::setOverlay2Alpha(int alpha)
{
  imageState.overlay2Alpha = (alpha<0) ? 0 : (alpha>255) ? 255 : alpha;
  for (size_t i=0;i<ports.size();i++)
    if (ports[i]) ports[i]->image()->imageState.overlay2Alpha = imageState.overlay2Alpha;
  updateImages();
}

void BrainSuiteWindow::setLabelAlpha(int alpha)
{
  imageState.labelAlpha = (alpha<0) ? 0 : (alpha>255) ? 255 : alpha;
  for (size_t i=0;i<ports.size();i++)
    if (ports[i]) ports[i]->image()->imageState.labelAlpha = imageState.labelAlpha;
  updateImages();
}
void resetTexture();
void BrainSuiteWindow::setVolumeBrightness(const ImageScale &brightness)
{
  imageState.brightness = brightness;
  for (size_t i=0;i<ports.size();i++)
    if (ports[i]) ports[i]->image()->imageState.brightness = imageState.brightness;
  updateImages();
}

void BrainSuiteWindow::setOverlay1Brightness(const ImageScale &brightness)
{
  imageState.overlay1Brightness = brightness;
  for (size_t i=0;i<ports.size();i++)
  {
    if (ports[i]) ports[i]->image()->imageState.overlay1Brightness = imageState.overlay1Brightness;
  }
  updateImages();
}

void BrainSuiteWindow::setOverlay2Brightness(const ImageScale &brightness)
{
  imageState.overlay2Brightness = brightness;
  for (size_t i=0;i<ports.size();i++)
    if (ports[i]) ports[i]->image()->imageState.overlay2Brightness = imageState.overlay2Brightness;
  updateImages();
}

void BrainSuiteWindow::autoscalePrimaryVolume()
{
  if (brainSuiteDataManager.volume)
  {
    imageState.brightness.maxv=(float)ComputeRange::findRange(brainSuiteDataManager.volume,0.999f);
    imageState.brightness.minv = ComputeRange::findMin(brainSuiteDataManager.volume);
    if (toolManager.imageDisplayPropertiesForm
        &&toolManager.imageDisplayPropertiesForm->imageSlider()
        &&toolManager.imageDisplayPropertiesForm->imageSlider()->symmetric)
    {
      if (imageState.brightness.maxv>0)
      {
        float f=std::max(std::abs(imageState.brightness.maxv),std::abs(imageState.brightness.minv));
        imageState.brightness.minv=-f;
        imageState.brightness.maxv=f;
      }
    }
    setVolumeBrightness(imageState.brightness);
    if (toolManager.delineationToolboxForm)
    {
      switch (brainSuiteDataManager.volume->typeID())
      {
        case SILT::Float32 :
        case SILT::Float64 :
          toolManager.delineationToolboxForm->setThresholdRange(
                (imageState.brightness.minv)>=0? 0 : imageState.brightness.minv,
                imageState.brightness.maxv,
                1000);
          break;
        default:
          toolManager.delineationToolboxForm->setThresholdRange(
                (imageState.brightness.minv)>=0? 0 : imageState.brightness.minv,
                imageState.brightness.maxv);
      }
    }
  }
}

void BrainSuiteWindow::autoscaleOverlay1()
{
  if (brainSuiteDataManager.imageOverlay1Volume)
  {
    imageState.overlay1Brightness.maxv = (float)ComputeRange::findRange(brainSuiteDataManager.imageOverlay1Volume,0.999f);
    imageState.overlay1Brightness.minv = ComputeRange::findMin(brainSuiteDataManager.imageOverlay1Volume);
    if (toolManager.imageDisplayPropertiesForm
        &&toolManager.imageDisplayPropertiesForm->imageOverlay1Slider()
        &&toolManager.imageDisplayPropertiesForm->imageOverlay1Slider()->symmetric)
    {
      if (imageState.overlay1Brightness.maxv>0)
      {
        float f=std::max(std::abs(imageState.overlay1Brightness.maxv),std::abs(imageState.overlay1Brightness.minv));
        imageState.overlay1Brightness.minv=-f;
        imageState.overlay1Brightness.maxv=f;
      }
    }
    for (size_t i=0;i<ports.size();i++) if (ports[i]) ports[i]->image()->imageState.overlay1Brightness = imageState.overlay1Brightness;
  }
}

void BrainSuiteWindow::autoscaleOverlay2()
{
  if (brainSuiteDataManager.imageOverlay2Volume)
  {
    imageState.overlay2Brightness.maxv=(float)ComputeRange::findRange(brainSuiteDataManager.imageOverlay2Volume,0.999f);
    imageState.overlay2Brightness.minv=ComputeRange::findMin(brainSuiteDataManager.imageOverlay2Volume);
    if (toolManager.imageDisplayPropertiesForm
        &&toolManager.imageDisplayPropertiesForm->imageOverlay2Slider()
        &&toolManager.imageDisplayPropertiesForm->imageOverlay2Slider()->symmetric)
    {
      if (imageState.overlay2Brightness.maxv>0)
      {
        float f=std::max(std::abs(imageState.overlay2Brightness.maxv),std::abs(imageState.overlay2Brightness.minv));
        imageState.overlay2Brightness.minv=-f;
        imageState.overlay2Brightness.maxv=f;
      }
    }
    for (size_t i=0;i<ports.size();i++) if (ports[i]) ports[i]->image()->imageState.overlay2Brightness = imageState.overlay2Brightness;
  }
}

void BrainSuiteWindow::makeLUTPopupMenu(QPoint point, uint32 *&lut)
{
  if (lut)
  {
    LUTMenuSystem menuSystem(this,lut);
    menuSystem.doPopup(point);
  }
}

void BrainSuiteWindow::autoscaleImages()
{
  autoscalePrimaryVolume();
  autoscaleOverlay1();
  autoscaleOverlay2();
  if (toolManager.imageDisplayPropertiesForm)
    toolManager.imageDisplayPropertiesForm->updateDisplay();
  updateImageDisplaySliders();
  toolManager.imageDisplayPropertiesForm->setSliderRanges();
  updateImages();
}

void BrainSuiteWindow::updateImageDisplaySliders()
{
  if (toolManager.imageDisplayPropertiesForm)
  {
    toolManager.imageDisplayPropertiesForm->updateDisplay();
  }
}

void BrainSuiteWindow::repaintImages()
{
  for (size_t i=0;i<ports.size();i++)
    if (ports[i])
    {
      ports[i]->image()->markDirty();
      ports[i]->image()->repaint();
    }
  updateSurfaceView();
}

void BrainSuiteWindow::updateImages()
{
  for (size_t i=0;i<ports.size();i++)
    if (ports[i])
    {
      ports[i]->image()->markDirty();
      ports[i]->updateSlider();
      ports[i]->updateText();
    }
  if (viewSettings.blockSync==false)
  {
    updateSurfaceView();
  }
}

void BrainSuiteWindow::moveToWC(const DSPoint &p, QMouseEvent *event)
{
  if (brainSuiteDataManager.volume)
  {
    Vol3DBase *v=brainSuiteDataManager.volume;
    mouseMoveTo(0,IPoint3D(p.x/v->rx,p.y/v->ry,p.z/v->rz),event);
  }
  else
    mouseMoveTo(0,IPoint3D(p.x,p.y,p.z),event);
}

void BrainSuiteWindow::moveTo(const DSPoint &p)
{
  currentVolumePosition = IPoint3D(p.x,p.y,p.z);
  updateView(DisplayCode::MaskOn|DisplayCode::Dirty);
}

void BrainSuiteWindow::moveTo(const IPoint3D &p)
{
  currentVolumePosition = p;
  updateView(DisplayCode::MaskOn|DisplayCode::Dirty);
}

void BrainSuiteWindow::updateView(uint32 viewUpdateCode)
{
  bool updateDisplaySliders=false;
  if (viewUpdateCode & DisplayCode::Image1Only) { imageState.viewOverlay = false;  imageState.viewOverlay2 = false;  imageState.viewMask = false;  imageState.viewLabels = false; }
  if (viewUpdateCode & DisplayCode::MaskOn) { imageState.viewMask = true; }
  if (viewUpdateCode & DisplayCode::MaskOff) { imageState.viewMask = false; }
  if (viewUpdateCode & DisplayCode::ShowOverlay1) { imageState.viewOverlay = true; imageState.overlayAlpha = 255; updateDisplaySliders=true; }
  if (viewUpdateCode & DisplayCode::ShowLabels) { imageState.viewLabels = true; if (!imageState.labelAlpha) imageState.labelAlpha = 64; updateDisplaySliders=true; }
  if (viewUpdateCode & DisplayCode::ShowOverlay2) { imageState.viewOverlay2 = true; imageState.overlayAlpha = 255; updateDisplaySliders=true; }
  if (viewUpdateCode & DisplayCode::HideOverlay1) { imageState.viewOverlay = false; }
  if (viewUpdateCode & DisplayCode::HideOverlay2) { imageState.viewOverlay2 = false; }
  bool forceDraw = (viewUpdateCode & DisplayCode::RedrawView);
  if (updateDisplaySliders)
    updateImageDisplaySliders();
  for (size_t i=0;i<ports.size();i++)
  {
    if (ports[i] && ports[i]->image())
    {
      ports[i]->image()->imageState.viewOverlay = imageState.viewOverlay;
      ports[i]->image()->imageState.overlayAlpha = imageState.overlayAlpha;
      ports[i]->image()->imageState.overlay2Alpha = imageState.overlay2Alpha;
      ports[i]->image()->imageState.viewMask = imageState.viewMask;
      ports[i]->image()->imageState.viewOverlay2 = imageState.viewOverlay2;
      ports[i]->image()->imageState.viewLabels = imageState.viewLabels;
      if (viewUpdateCode & DisplayCode::Dirty)
        ports[i]->image()->markDirty();
      if (forceDraw)
      {
        ports[i]->image()->updateImage();
        ports[i]->image()->repaint();
      }
      else
        ports[i]->image()->update();
    }
  }
  updateMenuChecks();
  updateSurfaceView();
  if (dsglWidget)
    dsglWidget->updateGL();
}

void BrainSuiteWindow::setZoomLevel(int n)
{
  imageState.zoom=n;
  imageState.zoomf = (float)pow(ImageWidget::zoomMultiplier,(float)imageState.zoom);
  for (size_t i=0;i<ports.size();i++)
  {
    if (ports[i])
    {
      ports[i]->image()->imageState.zoom = imageState.zoom;
      ports[i]->image()->imageState.zoomf = imageState.zoomf;
      ports[i]->image()->updateOffsets();
      ports[i]->image()->update();
      ports[i]->updateText();
    }
  }
}

void BrainSuiteWindow::zoomIn()
{
  imageState.zoom++;
  imageState.zoomf = (float)pow(ImageWidget::zoomMultiplier,(float)imageState.zoom);
  for (size_t i=0;i<ports.size();i++)
  {
    if (ports[i])
    {
      ports[i]->image()->imageState.zoom = imageState.zoom;
      ports[i]->image()->imageState.zoomf = imageState.zoomf;
      ports[i]->image()->updateOffsets();
      ports[i]->image()->update();
      ports[i]->updateText();
    }
  }
}

void BrainSuiteWindow::zoomUnity()
{
  imageState.zoom=0;
  imageState.zoomf = 1.0f;
  for (size_t i=0;i<ports.size();i++)
  {
    if (ports[i])
    {
      ports[i]->image()->imageState.zoom = imageState.zoom;
      ports[i]->image()->imageState.zoomf = imageState.zoomf;
      ports[i]->image()->updateOffsets();
      ports[i]->image()->update();
      ports[i]->updateText();
    }
  }
}

void BrainSuiteWindow::zoomOut()
{
  imageState.zoom--;
  imageState.zoomf = (float)pow(ImageWidget::zoomMultiplier,(float)imageState.zoom);
  for (size_t i=0;i<ports.size();i++)
  {
    if (ports[i])
    {
      ports[i]->image()->imageState.zoom = imageState.zoom;
      ports[i]->image()->imageState.zoomf = imageState.zoomf;
      ports[i]->image()->updateOffsets();
      ports[i]->image()->update();
      ports[i]->updateText();
    }
  }
}

DSPoint BrainSuiteWindow::getSurfaceBackgroundColor()
{
  if (dsglWidget)
  {
    return dsglWidget->getBackgroundColor();
  }
  return DSPoint(0.5625,0.5625,0.78125f);
}

void BrainSuiteWindow::setSurfaceBackgroundColor(DSPoint backgroundColor)
{
  if (dsglWidget)
    dsglWidget->setBackgroundColor(backgroundColor);
}

void BrainSuiteWindow::updateSurfaceView()
{
  if (dsglWidget)
  {
    dsglWidget->update();
  }
}

inline int labelValue(Vol3DBase *v, const IPoint3D &p)
{
  switch (v->typeID())
  {
  case SILT::Uint8 : return (*(Vol3D<uint8> *)v)(p.x,p.y,p.z); break;
  case SILT::Sint8 : return (*(Vol3D<sint8> *)v)(p.x,p.y,p.z); break;
  case SILT::Uint16: return (*(Vol3D<uint16> *)v)(p.x,p.y,p.z); break;
  case SILT::Sint16: return (*(Vol3D<sint16> *)v)(p.x,p.y,p.z); break;
  case SILT::Uint32: return (*(Vol3D<uint32> *)v)(p.x,p.y,p.z); break;
  case SILT::Sint32: return (*(Vol3D<sint32> *)v)(p.x,p.y,p.z); break;
  default: return 0;
  }
  return 0;
}

inline int labelValueI(Vol3DBase *v, const size_t p)
{
  switch (v->typeID())
  {
  case SILT::Uint8 : return (*(Vol3D<uint8> *)v)[p]; break;
  case SILT::Sint8 : return (*(Vol3D<sint8> *)v)[p]; break;
  case SILT::Uint16: return (*(Vol3D<uint16> *)v)[p]; break;
  case SILT::Sint16: return (*(Vol3D<sint16> *)v)[p]; break;
  case SILT::Uint32: return (*(Vol3D<uint32> *)v)[p]; break;
  case SILT::Sint32: return (*(Vol3D<sint32> *)v)[p]; break;
  default: return 0;
  }
  return 0;
}

inline std::string textValue(Vol3DBase *v, const IPoint3D &point)
{
  std::ostringstream sstr;
  if (v)
  {
    if (((point.x<0)||(point.x>=(int)v->cx))
        || ((point.y<0)||(point.y>=(int)v->cy))
        || ((point.z<0)||(point.z>=(int)v->cz)))
    {
      sstr<<"<?>";
    }
    else
    {
      switch (v->typeID())
      {
      case SILT::Eigensystem3x3f:
      {
        Vol3D<EigenSystem3x3f> &veigen= *(Vol3D<EigenSystem3x3f>*)v;
        EigenSystem3x3f &val = veigen(point.x,point.y,point.z);
        sstr<<'('<<val.l0<<','<<val.l1<<','<<val.l2<<")";
      }
        break;
      case SILT::RGB8:
      {
        Vol3D<rgb8> &vrgb = *(Vol3D<rgb8>*)v;
        rgb8 val = vrgb(point.x,point.y,point.z);
        sstr<<'('<<(int)val.r<<','<<(int)val.g<<','<<(int)val.b<<')';
      }
        break;
      case SILT::Float32: sstr<<(*(Vol3D<float32> *)v)(point.x,point.y,point.z); break;
      case SILT::Float64: sstr<<(*(Vol3D<float64> *)v)(point.x,point.y,point.z); break;
      default:
        sstr<<labelValue(v,point);
      }
    }
  }
  return sstr.str();
}

void BrainSuiteWindow::setVolumePosition(IPoint3D newPosition, const bool forcePaint)
{
  currentVolumePosition = newVolumePosition(newPosition);
  updateImages();
  if (brainSuiteDataManager.volume)
  {
    std::ostringstream sstr;
    sstr<<"V: "<<textValue(brainSuiteDataManager.volume,currentVolumePosition);
    if (brainSuiteDataManager.imageOverlay1Volume)
      sstr<<" R: "<<textValue(brainSuiteDataManager.imageOverlay1Volume,currentVolumePosition);
    Vol3DBase *ovr2 = brainSuiteDataManager.imageOverlay2Volume;
    if (ovr2)
      if (currentVolumePosition.x<(int)ovr2->cx&&currentVolumePosition.y<(int)ovr2->cy&&currentVolumePosition.z<(int)ovr2->cz)
      {
        sstr<<" R: "<<textValue(ovr2,currentVolumePosition);
      }
    if (brainSuiteDataManager.vLabel)
    {
      int val = labelValue(brainSuiteDataManager.vLabel,currentVolumePosition);
      sstr<<" L: "<<val;
      if (val>=0 && val<(int)brainSuiteDataManager.labelDescriptionSet.labelDetails.size())
        sstr<<"    "<<brainSuiteDataManager.labelDescriptionSet.labelDetails[val].tag<<"    "<<brainSuiteDataManager.labelDescriptionSet.labelDetails[val].descriptor;
    }
    status(sstr.str());
  }
  if (forcePaint)
  {
  }
}

void BrainSuiteWindow::zoomBestFit()
{
  if (viewSettings.singleView)
  {
    for (size_t i=0;i<ports.size();i++)
    {
      if (ports[i] && ports[i]->isVisible())
      {
        float z = ports[i]->image()->zoomBestFit();
        int zl = (int)(log(imageState.zoomf)/log(ImageWidget::zoomMultiplier));
        ports[i]->image()->imageState.zoom = z;
        ports[i]->image()->imageState.zoomf = zl;
        ports[i]->image()->updateOffsets();
        ports[i]->image()->update();
        ports[i]->updateText();
      }
    }
  }
  else
  {
    float min = -1.0f;
    for (size_t i=0;i<ports.size();i++)
    {
      if (ports[i] && ports[i]->isVisible())
      {
        float z = ports[i]->image()->zoomBestFit();
        if (min<0)
          min = z;
        else if (z<min)
          min = z;
      }
    }
    if (min>0)
    {
      imageState.zoomf = min;
      imageState.zoom = (int)(log(imageState.zoomf)/log(ImageWidget::zoomMultiplier));
    }
    for (size_t i=0;i<ports.size();i++)
    {
      if (ports[i])
      {
        ports[i]->image()->imageState.zoom = imageState.zoom;
        ports[i]->image()->imageState.zoomf = imageState.zoomf;
        ports[i]->image()->updateOffsets();
        ports[i]->image()->update();
        ports[i]->updateText();
      }
    }
  }
}

void BrainSuiteWindow::updateVolumeView(bool autoScale) // for use after a new volume is loaded
{
  if (autoScale && !BrainSuiteSettings::dontAutoScale)
  {
    autoscalePrimaryVolume();
    updateImageDisplaySliders();
    currentVolumePosition = IPoint3D(brainSuiteDataManager.volume->cx/2,brainSuiteDataManager.volume->cy/2,brainSuiteDataManager.volume->cz/2);
  }
  updateImageNames();
  zoomBestFit();
  for (size_t i=0;i<ports.size();i++)
    if (ports[i])
    {
      ports[i]->updateSlider();
      ports[i]->updateText();
    }
  update();
}

void BrainSuiteWindow::checkAutosavePath()
{
  if (disablePathChecking) return;
  if (toolManager.corticalExtractionSequenceDialog && brainSuiteDataManager.volume)
  {
    std::string autosave=toolManager.corticalExtractionSequenceDialog->formAutosavePrefix(brainSuiteDataManager.volume->filename);
    if (autosave != BrainSuiteSettings::autosavePrefix)
    {
      QMessageBox msgBox(this);
      msgBox.setText("Update autosave path?");
      msgBox.setIcon(QMessageBox::Question);
      std::ostringstream msg;
      msg<<"The volume you have loaded has a different path or filename than the autosave configuration."
           " Would you like to update the autosave to this path?\n"
           " Note: you can configure this manually using the Cortical Extraction Sequence Dialog\n";
      msgBox.setInformativeText(msg.str().c_str());
      msgBox.setStandardButtons(QMessageBox::Yes|QMessageBox::No);
      msgBox.setDefaultButton(QMessageBox::Yes);
      msgBox.exec();
      if (msgBox.standardButton(msgBox.clickedButton())==QMessageBox::Yes)
      {
        toolManager.corticalExtractionSequenceDialog->suggestOutputs();
      }
    }
  }
}

void BrainSuiteWindow::closeVolume()
{
  delete brainSuiteDataManager.volume;
  brainSuiteDataManager.volume = 0;
  updateImages();
  updateImageNames();
  updateImageDisplaySliders();
}

void BrainSuiteWindow::closeOverlay1Volume()
{
  delete brainSuiteDataManager.imageOverlay1Volume;
  brainSuiteDataManager.imageOverlay1Volume = 0;
  updateImages();
  updateImageNames();
  updateImageDisplaySliders();
}

void BrainSuiteWindow::closeOverlay2Volume()
{
  delete brainSuiteDataManager.imageOverlay2Volume;
  brainSuiteDataManager.imageOverlay2Volume = 0;
  updateImages();
  updateImageNames();
  updateImageDisplaySliders();
}

void BrainSuiteWindow::closeLabelVolume()
{
  delete brainSuiteDataManager.vLabel;
  brainSuiteDataManager.vLabel = 0;
  updateImages();
  updateImageNames();
  updateImageDisplaySliders();
}

void BrainSuiteWindow::closeMaskVolume()
{
  brainSuiteDataManager.vMask.setsize(0,0,0);
  brainSuiteDataManager.vMask.releaseMemory();
  updateImages();
  updateImageNames();
  updateImageDisplaySliders();
}


void BrainSuiteWindow::on_action_Image_Display_Properties_triggered()
{
  if (ui->imageDisplayPropertiesDockWidget)
  {
    ui->imageDisplayPropertiesDockWidget->show();
    ui->imageDisplayPropertiesDockWidget->raise();
  }
}

// BrainSuite Help Menu Items
void BrainSuiteWindow::on_action_About_BrainSuiteQT_triggered()
{
  AboutBrainSuiteDialog dlg;
  dlg.exec();
}

void BrainSuiteWindow::on_action_Visit_the_BrainSuite_website_triggered()
{
  QDesktopServices::openUrl(QUrl(BrainSuiteQtSettings::website.c_str()));
}


void BrainSuiteWindow::on_action_Visit_the_BrainSuite_Forum_triggered()
{
  QDesktopServices::openUrl(QUrl(BrainSuiteQtSettings::websiteForum.c_str()));
}

void BrainSuiteWindow::on_action_Visit_the_BrainSuite_Download_Page_triggered()
{
  QDesktopServices::openUrl(QUrl(BrainSuiteQtSettings::websiteDownload.c_str()));
}


void BrainSuiteWindow::on_action_Continuous_sync_with_image_view_toggled(bool arg1)
{
  viewSettings.syncGL=arg1;
}

void BrainSuiteWindow::on_actionSurface_DisplayProperties_triggered()
{
  showSurfaceDisplayProperties();
}

void BrainSuiteWindow::on_actionFiber_Tract_Tool_triggered()
{
  showDiffusionToolbox();
}

void BrainSuiteWindow::on_action_Show_Current_Display_Settings_triggered()
{
  std::cout<<"# current display settings\n";
  std::cout<<"set_voxel_position "<<currentVolumePosition<<'\n';
  std::cout<<"set_zoom_level "<<imageState.zoom<<'\n';
  if (dsglWidget)
  {
    std::cout<<"set_view_shift "<<std::setprecision(10)<<dsglWidget->glViewController.properties.xShift<<' '
            <<std::setprecision(10)<<dsglWidget->glViewController.properties.xShift<<'\n';
    std::cout<<"set_view_zoom "<<std::setprecision(10)<<dsglWidget->glViewController.properties.zoom<<'\n';
    std::cout<<"set_view_rotation "<<std::setprecision(10)<<dsglWidget->glViewController.properties.xAngle
            <<" "<<std::setprecision(10)<<dsglWidget->glViewController.properties.yAngle
           <<" "<<std::setprecision(10)<<dsglWidget->glViewController.properties.zAngle<<'\n';
  }
}

void BrainSuiteWindow::on_action_Connectivity_Viewer_triggered()
{
  showDialog(toolManager.connectivityViewDialog);
}

void BrainSuiteWindow::on_action_Surface_View_Only_triggered()
{
  if (dsglWidget)
  {
    lastViewMode=viewMode;
    viewMode=SurfaceOnly;
    for (size_t i=0;i<ports.size();++i) { if (ports[i]) ports[i]->setVisible(false); }
    if (popoutSurfaceView) detachDSGLView();
    dsglWidget->setVisible(true);
  }
}

void BrainSuiteWindow::on_action_Orthogonal_Views_Surface_View_triggered()
{
  if (viewMode==Ortho3D) return;
  bool returningToSameView = (lastViewMode==Ortho3D&&viewMode==SurfaceOnly);
  lastViewMode=viewMode;
  viewMode=Ortho3D;
  for (size_t i=0;i<ports.size();++i)
    if (ports[i])
      ports[i]->setVisible(true);
  if (dsglWidget)
    dsglWidget->setVisible(true);
  // update the frames only!
  ImageWidget::blockPainting=true;
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents); // this is to resize the frames only, so don't redraw
  ImageWidget::blockPainting=false;
  if (returningToSameView)
  {
    updateImages();
    return;
  }
  if (BrainSuiteSettings::zoomBestFitOnResize)
    zoomBestFit();
  else
  {
    for (size_t i=0;i<ports.size();++i)
      if (ports[i])
        ports[i]->image()->updateOffsets();
    updateImages();
  }
}

void BrainSuiteWindow::on_action_BSE_triggered()
{
  showDialog(toolManager.skullStrippingDialog);
}

void BrainSuiteWindow::on_actionNonuniformity_Correction_BFC_triggered()
{
  showDialog(toolManager.nonuniformityCorrectionDialog);
}

void BrainSuiteWindow::on_actionTissue_Classification_PVC_triggered()
{
  showDialog(toolManager.tissueClassificationDialog);
}

void BrainSuiteWindow::on_action_Cerebrum_Extraction_triggered()
{
  showDialog(toolManager.cerebrumExtractionDialog);
}

void BrainSuiteWindow::on_action_Inner_Cortex_Boundary_triggered()
{
  showDialog(toolManager.cortexIdentificationDialog);
}

void BrainSuiteWindow::on_action_Scrub_Mask_triggered()
{
  showDialog(toolManager.scrubMaskDialog);
}

void BrainSuiteWindow::on_action_Topological_Correction_triggered()
{
  showDialog(toolManager.topologicalCorrectionDialog);
}

void BrainSuiteWindow::on_action_Wisp_Filter_triggered()
{
  showDialog(toolManager.dewispDialog);
}

void BrainSuiteWindow::on_action_Inner_Cortical_Surface_triggered()
{
  showDialog(toolManager.innerCorticalSurfaceDialog);
}

void BrainSuiteWindow::on_action_Pial_Surface_triggered()
{
  showDialog(toolManager.pialSurfaceDialog);
}

void BrainSuiteWindow::on_action_Hemisphere_Splitting_triggered()
{
  showDialog(toolManager.splitHemispheresDialog);
}

void BrainSuiteWindow::on_action_Skull_and_Scalp_triggered()
{
  showDialog(toolManager.skullfinderDialog);
}

void BrainSuiteWindow::resizeEvent ( QResizeEvent *  )
{
  if (BrainSuiteSettings::zoomBestFitOnResize)
    zoomBestFit();
}

bool BrainSuiteWindow::updateImageDisplayProperties()
{
  updateImageNames();
  return true;
}


void hideSurface(BrainSuiteDataManager::SurfaceHandle &handle)
{
  if (handle.surface) handle.surface->show = false;
}

void BrainSuiteWindow::hideAllSurfaces()
{
  std::for_each(brainSuiteDataManager.surfaces.begin(), brainSuiteDataManager.surfaces.end(), ::hideSurface);
  updateSurfaceView();
}

void BrainSuiteWindow::showSingleSurface(SurfaceAlpha *primarySurface)
{
  std::for_each(brainSuiteDataManager.surfaces.begin(), brainSuiteDataManager.surfaces.end(), ::hideSurface);
  if (primarySurface) primarySurface->show=true;
  updateSurfaceView();
}

bool BrainSuiteWindow::addSurface(SurfaceAlpha *surface, bool giveOwnership)
{
  if (!brainSuiteDataManager.addSurface(surface,giveOwnership)) return false;
  updateSurfaceDisplayLists();
  updateSurfaceView();
  return true;
}

void BrainSuiteWindow::on_actionZoom_Best_Fit_triggered()
{
  zoomBestFit();
}

void BrainSuiteWindow::on_action_Mask_Tool_triggered()
{
  if (toolManager.delineationToolboxForm)
  {
    ui->delineationToolboxDockWidget->show();
    ui->delineationToolboxDockWidget->raise();
    toolManager.delineationToolboxForm->showMaskTool();
  }
  else
  {
    std::cerr<<"delineationToolboxForm does not exist!"<<std::endl;
  }
}

void BrainSuiteWindow::on_action_Painter_Tool_triggered()
{
  if (toolManager.delineationToolboxForm)
  {
    ui->delineationToolboxDockWidget->show();
    ui->delineationToolboxDockWidget->raise();
    toolManager.delineationToolboxForm->showPainterTool();
  }
}

void BrainSuiteWindow::on_action_Cortical_Extraction_Sequence_triggered()
{
  showDialog(toolManager.corticalExtractionSequenceDialog);
}


bool BrainSuiteWindow::saveVolume(std::string filename)
{
  if (!brainSuiteDataManager.volume) return false;
  return brainSuiteDataManager.volume->write(filename);
}

bool BrainSuiteWindow::saveVolume()
{
  if (!brainSuiteDataManager.volume) return false;
  QString filename = dsGetSaveFileNameQS("Save Image Volume","3D Image Volume Files (*.img *.nii *.img.gz *.nii.gz);;3D Image Volume Files (*.img *.nii *.img.gz *.nii.gz)","");
  if (filename.isEmpty()) return false;
  return saveVolume(filename.toStdString());
}

bool BrainSuiteWindow::saveOverlay1Volume(std::string overlayFilename)
{
  if (!brainSuiteDataManager.imageOverlay1Volume) return false;
  std::cout<<"saving "<<overlayFilename<<std::endl;
  return brainSuiteDataManager.imageOverlay1Volume->write(overlayFilename);
}

bool BrainSuiteWindow::saveOverlay1Volume()
{
  if (!brainSuiteDataManager.imageOverlay1Volume) return false;
  QString filename = dsGetSaveFileNameQS(("Save Image Volume"),("3D Image Volume Files (*.img *.nii *.img.gz *.nii.gz);;3D Image Volume Files (*.img *.nii *.img.gz *.nii.gz)"),"");
  if (filename.isEmpty()) return false;
  if (!saveOverlay1Volume(filename.toStdString())) { std::cerr<<"failed to save "<<filename.toStdString()<<std::endl; return false; }
  return true;
}

bool BrainSuiteWindow::saveOverlay2Volume(std::string overlayFilename)
{
  if (!brainSuiteDataManager.imageOverlay2Volume) return false;
  return brainSuiteDataManager.imageOverlay2Volume->write(overlayFilename);
}

bool BrainSuiteWindow::saveOverlay2Volume()
{
  if (!brainSuiteDataManager.imageOverlay2Volume) return false;
  QString filename = dsGetSaveFileNameQS(("Save Image Volume"),("3D Image Volume Files (*.img *.nii *.img.gz *.nii.gz);;3D Image Volume Files (*.img *.nii *.img.gz *.nii.gz)"),"");
  if (filename.isEmpty()) return false;
  return saveOverlay1Volume(filename.toStdString());
}

void BrainSuiteWindow::on_action_Save_Primary_Volume_triggered()
{
  saveVolume();
}

void BrainSuiteWindow::on_action_Save_Overlay1_Volume_triggered()
{
  saveOverlay1Volume();
}

void BrainSuiteWindow::on_action_Save_Overlay2_Volume_triggered()
{
  saveOverlay2Volume();
}

void BrainSuiteWindow::on_action_Save_Label_Volume_triggered()
{
  saveLabelVolume();
}

void BrainSuiteWindow::on_actionShow_Overlay_1_triggered()
{
  imageState.viewOverlay = ui->actionShow_Overlay_1->isChecked();
  for (size_t i=0;i<ports.size();i++)
  {
    if (ports[i] && ports[i]->image()) ports[i]->image()->imageState.viewOverlay = imageState.viewOverlay;
  }
}

void BrainSuiteWindow::on_actionShow_Overlay_2_triggered()
{
  imageState.viewOverlay2 = ui->actionShow_Overlay_2->isChecked();
  for (size_t i=0;i<ports.size();i++)
  {
    if (ports[i] && ports[i]->image()) ports[i]->image()->imageState.viewOverlay2 = imageState.viewOverlay2;
  }

}


void BrainSuiteWindow::on_action_Outline_triggered()
{
  imageState.viewMask = true;
  imageState.maskMode = ImageState::Outline;
  for (size_t i=0;i<ports.size();i++)
    if (ports[i])
    {
      ports[i]->image()->imageState.viewMask = imageState.viewMask;
      ports[i]->image()->imageState.maskMode = imageState.maskMode;
    }
  updateImages();
  updateMenuChecks();
}

void BrainSuiteWindow::on_action_Stencil_triggered()
{
  imageState.viewMask = true;
  imageState.maskMode = ImageState::Stencil;
  for (size_t i=0;i<ports.size();i++)
    if (ports[i])
    {
      ports[i]->image()->imageState.viewMask = imageState.viewMask;
      ports[i]->image()->imageState.maskMode = imageState.maskMode;
    }
  updateImages();
  updateMenuChecks();
}

void BrainSuiteWindow::on_action_Blend_triggered()
{
  imageState.viewMask = true;
  imageState.maskMode = ImageState::Blend;
  for (size_t i=0;i<ports.size();i++)
    if (ports[i])
    {
      ports[i]->image()->imageState.viewMask = imageState.viewMask;
      ports[i]->image()->imageState.maskMode = imageState.maskMode;
    }
  updateImages();
  updateMenuChecks();
}

void BrainSuiteWindow::on_action_Mask_Only_triggered()
{
  imageState.viewMask = true;
  imageState.maskMode = ImageState::MaskOnly;
  for (size_t i=0;i<ports.size();i++)
    if (ports[i])
    {
      ports[i]->image()->imageState.viewMask = imageState.viewMask;
      ports[i]->image()->imageState.maskMode = imageState.maskMode;
    }
  updateImages();
  updateMenuChecks();
}

void BrainSuiteWindow::on_action_Reset_Surface_View_triggered()
{
  if (dsglWidget)
    dsglWidget->resetView();
}

void BrainSuiteWindow::on_action_Curve_Tool_triggered()
{
  ui->curveToolDockWidget->show();
  ui->curveToolDockWidget->raise();
}


bool BrainSuiteWindow::setMaskUndoDescription(std::string description)
{
  ui->action_Undo_mask_edit->setText(("Undo " + description).c_str());
  return true;
}

void BrainSuiteWindow::on_action_Undo_mask_edit_triggered()
{
  if (toolManager.delineationToolboxForm)
  {
    toolManager.delineationToolboxForm->undoLastAction();
  }
}

void BrainSuiteWindow::on_action_Popout_Surface_View_triggered()
{
  popoutSurfaceView^=true;
  ui->action_Popout_Surface_View->setChecked(popoutSurfaceView);
  initializeSurfaceView();
}

void BrainSuiteWindow::on_actionSurface_Volume_Registration_triggered()
{
  showDialog(toolManager.svRegistrationDialog);
}

void BrainSuiteWindow::on_action_Reinstall_BrainSuite_Atlas_Files_triggered()
{
  QString currentInstallDir = BrainSuiteQtSettings::qtinstalldir.c_str();
  BrainSuiteInstallDialog installDialog(this,currentInstallDir);
  installDialog.setCheckInstallFlag(BrainSuiteQtSettings::enableCheckForUpdates);
  installDialog.setContinueText("Return to " PROGNAME);
  installDialog.setMainText(
        "Please select a directory for " PROGNAME ". "
        "The typical location for this would be in a subdirectory in your home directory."
        );
  int code=installDialog.exec();
  if (code==QDialog::Accepted)
  {
    QSettings settings("BrainSuite",BrainSuiteQtSettings::brainSuiteQtKey);
    QString install_directory = settings.value(BrainSuiteQtSettings::qtinstallkey.c_str()).toString(); // install_directory
    BrainSuiteQtSettings::qtinstalldir = install_directory.toStdString();
    initializeSettings();
  }
  ui->actionAllow_BrainSuite_to_Check_for_Updates->setChecked(BrainSuiteQtSettings::enableCheckForUpdates);
}

void BrainSuiteWindow::on_action_HARDI_Tool_triggered()
{
  showDiffusionToolbox();
  if (toolManager.diffusionToolboxForm)
  {
    toolManager.diffusionToolboxForm->updateDiffusionProperties();
    toolManager.diffusionToolboxForm->showHARDIPage();
  }
}

void BrainSuiteWindow::on_action_Diffusion_Display_Properties_triggered()
{
  showDiffusionToolbox();
  if (toolManager.diffusionToolboxForm)
  {
    toolManager.diffusionToolboxForm->updateDiffusionProperties();
    toolManager.diffusionToolboxForm->showDTIPage();
  }
}

void BrainSuiteWindow::setShowLighting(bool flag)
{
  BrainSuiteSettings::useLighting=flag;
  updateToolboxChecks();
  updateSurfaceView();
}

void BrainSuiteWindow::setShowSurfaces(bool flag)
{
  BrainSuiteSettings::showSurfaces=flag;
  updateToolboxChecks();
  updateSurfaceView();
}

void BrainSuiteWindow::setShowTensors(bool flag)
{
  BrainSuiteSettings::showTensors=flag;
  updateToolboxChecks();
  updateSurfaceView();
}

void BrainSuiteWindow::setShowFibers(bool flag)
{
  BrainSuiteSettings::showFibers = flag;
  updateToolboxChecks();
  updateSurfaceView();
}

void BrainSuiteWindow::setShowODFs(bool flag)
{
  BrainSuiteSettings::showODFs=flag;
  updateToolboxChecks();
  updateSurfaceView();
}

void BrainSuiteWindow::updateToolboxChecks()
{
  if (toolManager.surfaceDisplayForm) toolManager.surfaceDisplayForm->updateGeneralCheckboxes();
  if (toolManager.diffusionToolboxForm) toolManager.diffusionToolboxForm->updateDiffusionProperties();
  if (toolManager.imageDisplayPropertiesForm) toolManager.imageDisplayPropertiesForm->updateDisplay();
}

bool BrainSuiteWindow::setSVRegDirectory(std::string directory)
{
  QFileInfo info(directory.c_str());
  if (info.exists() && info.isDir())
  {
    BrainSuiteQtSettings::qtsvregdir = QDir::toNativeSeparators(QDir::cleanPath(info.filePath())).toStdString();
    if (svregGUITool.initializeTool(ces))
    {
      QSettings settings("BrainSuite",BrainSuiteQtSettings::brainSuiteQtKey);
      settings.setValue(BrainSuiteQtSettings::qtsvregkey.c_str(),QString(BrainSuiteQtSettings::qtsvregdir.c_str()));
      return true;
    }
    else
    {
      std::cerr<<"Could not configure SVReg using this directory."<<std::endl;
      std::cerr<<svregGUITool.svreg.errorString<<std::endl;
      std::cerr<<"SVReg is not configured to run. If you wish to perform surface/volume registration, please install it."<<std::endl;//<<std::endl;
    }
    if (toolManager.svRegistrationDialog)
      toolManager.svRegistrationDialog->updateSVRegPath();
  }
  return false;
}

void BrainSuiteWindow::on_actionSelect_SVReg_Directory_triggered()
{
  QString directory = QFileDialog::getExistingDirectory (this, "Select the location of the SVReg directory",
                                                         BrainSuiteQtSettings::qtsvregdir.c_str(),
                                                         QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks );
  if (directory.isEmpty()) return;
  setSVRegDirectory(directory.toStdString());
}

void BrainSuiteWindow::zoomSmallest()
{
  if (!brainSuiteDataManager.volume) return;
  float m=std::min(std::min(brainSuiteDataManager.volume->rx,brainSuiteDataManager.volume->ry),brainSuiteDataManager.volume->rz);
  imageState.zoomf = 1.0f;
  if (m>0) imageState.zoomf=1.0f/m;
  imageState.zoom  = (int)(log(imageState.zoomf)/log(ImageWidget::zoomMultiplier));
  for (size_t i=0;i<ports.size();i++)
  {
    if (ports[i])
    {
      ports[i]->image()->imageState.zoomf = imageState.zoomf;
      ports[i]->image()->imageState.zoom = imageState.zoom;
      ports[i]->image()->updateOffsets();
      ports[i]->image()->update();
      ports[i]->updateText();
    }
  }
}

void BrainSuiteWindow::on_actionDisable_Mask_Undo_System_triggered()
{
  BrainSuiteSettings::maskUndoDisabled = ui->actionDisable_Mask_Undo_System->isChecked();
}

void BrainSuiteWindow::on_actionUse_Native_File_Dialogs_triggered()
{
  BrainSuiteQtSettings::setUseNativeFileDialogs(ui->actionUse_Native_File_Dialogs->isChecked());
  fileDialogOptions = (BrainSuiteQtSettings::getUseNativeFileDialogs()) ? QFileDialog::Options() :  QFileDialog::DontUseNativeDialog;
}

void BrainSuiteWindow::on_actionAllow_BrainSuite_to_Check_for_Updates_triggered()
{
  BrainSuiteQtSettings::setEnableCheckForUpdates(ui->actionAllow_BrainSuite_to_Check_for_Updates->isChecked());
}

void BrainSuiteWindow::on_actionSingle_Frame_triggered()
{
  setSinglePort((activePort>=0) ? activePort : 0);
}

void BrainSuiteWindow::on_actionAxial_View_triggered()
{
  setSinglePort(2);
}

void BrainSuiteWindow::on_actionSagittal_View_triggered()
{
  setSinglePort(1);
}

void BrainSuiteWindow::on_actionCoronal_View_triggered()
{
  setSinglePort(0);
}

void BrainSuiteWindow::on_actionAutozoom_on_Resize_triggered()
{
  BrainSuiteSettings::zoomBestFitOnResize = ui->actionAutozoom_on_Resize->isChecked();
}

void BrainSuiteWindow::on_actionThree_color_mode_triggered()
{
  imageState.threeChannel^=true;
  if (ports.size())
  {
    for (size_t i=0;i<ports.size();i++)
    {
      if (ports[i]&&ports[i]->image())
        ports[i]->image()->imageState.threeChannel=imageState.threeChannel;
    }
  }
  if (toolManager.imageDisplayPropertiesForm) toolManager.imageDisplayPropertiesForm->updateDisplay();
  updateImages();
}

void printInfo(Vol3DBase *volume, std::string name)
{
  if (!volume) return;
  std::cout<<name<<'\n';
  std::cout<<"<b>"<<volume->filename<<"/b";
  std::cout<<volume->filename<<'\n'
          <<volume->cx<<'x'<<volume->cy<<'x'<<volume->cz<<'\n'
         <<volume->rx<<"mm x "<<volume->ry<<"mm x "<<volume->rz<<"mm"<<'\n'
        <<"data type: "<<volume->datatypeName()<<'\n'
       <<"description: "<<volume->description<<'\n'
      <<"position:"<<volume->origin<<'\n'//ox<<'\t'<<volume->oy<<'\t'<<volume->oz<<'\n'
     <<"file orientation:"<<'\n'
    <<volume->fileOrientation.row0()<<'\n'
   <<volume->fileOrientation.row1()<<'\n'
  <<volume->fileOrientation.row2()<<'\n'
  <<"working orientation:"<<'\n'
  <<volume->currentOrientation.row0()<<'\n'
  <<volume->currentOrientation.row1()<<'\n'
  <<volume->currentOrientation.row2()<<'\n'
    ;//<<std::ends;
}

void BrainSuiteWindow::on_actionVolume_Info_triggered()
{
  printInfo(brainSuiteDataManager.volume,"primary");
  printInfo(brainSuiteDataManager.imageOverlay1Volume,"overlay1");
  printInfo(brainSuiteDataManager.imageOverlay2Volume,"overlay2");
  printInfo(brainSuiteDataManager.vLabel,"label");
  if (brainSuiteDataManager.vMask.size()) printInfo(&brainSuiteDataManager.vMask,"mask");
}


void BrainSuiteWindow::on_actionShow_Labels_as_Outlines_triggered()
{
  setShowLabelOutline(imageState.viewLabelOutline!=true);
  ui->actionAutozoom_on_Resize->setChecked(imageState.viewLabelOutline);
}

bool BrainSuiteWindow::loadExtractionSettings(std::string ifname)
{
  auto origIndex=ces.svreg.atlasIndex;
  auto origName =ces.svreg.atlasName;
  bool okay=true;
  if (!ces.readFromPlainText(ifname))
  {
    QMessageBox msgBox(this);
    msgBox.setText(("Error reading settings from file "+ifname).c_str());
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
    okay=false;
  }
  bool indexChanged=(origIndex != ces.svreg.atlasIndex)&&(origIndex >=0 || origIndex < (int)svregGUITool.svreg.svregManifest.atlases.size());
  bool nameChanged=(ces.svreg.atlasName != origName);
  if (indexChanged)
  {
    if (nameChanged && !origName.empty())
    {
      auto newName = svregGUITool.svreg.svregManifest.atlases[ces.svreg.atlasIndex].name;
      if (newName != ces.svreg.atlasName)
      {
        std::cerr<<"warning: atlas name ("<<ces.svreg.atlasName<<") and index name ("<<newName<<") do not match"<<std::endl;
        std::cerr<<"reverting to "<<newName<<std::endl;
      }
      else
      {
        std::cout<<"names match"<<std::endl;
      }
    }
  }
  else if (nameChanged)
  {
    auto nAtlases=svregGUITool.svreg.svregManifest.atlases.size();
    int newIndex=-1;
    for (decltype(nAtlases) i=0;i<nAtlases;i++)
    {
      if (ces.svreg.atlasName==svregGUITool.svreg.svregManifest.atlases[i].name)
      {
        newIndex=i;
        break;
      }
    }
    if (newIndex>=0)
    {
      ces.svreg.atlasIndex=newIndex;
    }
    else
    {
      std::cerr<<"warning: could not find atlas named "<<ces.svreg.atlasName<<std::endl;
      if (ces.svreg.atlasIndex>=0 && ces.svreg.atlasIndex<(int)svregGUITool.svreg.svregManifest.atlases.size())
      {
        ces.svreg.atlasName = svregGUITool.svreg.svregManifest.atlases[ces.svreg.atlasIndex].name;
        std::cerr<<"reverting to "<<ces.svreg.atlasName<<std::endl;
      }
    }
  }
  else
  {
  }

  toolManager.updateFromCES();
  if (toolManager.corticalExtractionSequenceDialog)
  {
    toolManager.corticalExtractionSequenceDialog->updateAllfromCES();
  }
  return okay;
}

bool BrainSuiteWindow::saveExtractionSettings(std::string ofname)
{
  if (!ces.saveAsPlainText(ofname))
  {
    QMessageBox msgBox(this);
    msgBox.setText(("Error saving settings file "+ofname).c_str());
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
    return false;
  }
  return true;
}

void BrainSuiteWindow::on_actionSave_Extraction_Settings_triggered()
{
  std::string ofname=dsGetSaveFileName("Save Cortical Extraction Settings","Plain Text Files (*.txt)","");
  if (ofname.empty()) return;
  saveExtractionSettings(ofname);
}

void BrainSuiteWindow::on_actionExtraction_Settings_triggered()
{
  std::string ifname=getOpenFilenameDS("Load Cortical Extraction Settings","Plain Text Files (*.txt)");
  if (ifname.empty()) return;
  loadExtractionSettings(ifname);
}

void BrainSuiteWindow::on_actionDisable_Autoscaling_toggled(bool arg1)
{
  BrainSuiteSettings::dontAutoScale=arg1;
}
