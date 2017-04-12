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
#include "imagewidget.h"
#include "brainsuitewindow.h"
#include <rgb8.h>
#include <eigensystem3x3.h>
#include <sstream>
#include "qpainter.h"
#include "colorops.h"
#include <vol3d.h>
#include "qtrendertemplates.h"
#include <qevent.h>
#include <vol3d.h>
#include <qmenu.h>
#include <colormap.h>
#include <toolmanager.h>
#include <labelbrush.h>
#include <imageframe.h>
#include <protocolcurveset.h>
#include <brainsuitesettings.h>
#include <diffusiontoolboxform.h>
#include <delineationtoolboxform.h>
#include <curvetoolboxform.h>
#include <iomanip>
#include <qdebug.h>
#include <DS/timer.h>

void noRender(DSImage &targetBitmap, const Vol3DBase *volume, const Orientation::Code orientation)
{
  const float scale = std::min(volume->rx,std::min(volume->ry,volume->rz));
  switch (orientation)
  {
    case Orientation::XY :
      targetBitmap.resize(volume->cx,volume->cy);
      targetBitmap.setRes(volume->rx,volume->ry,scale);
      targetBitmap.blank();
      break;
    case Orientation::XZ:
      targetBitmap.resize(volume->cx,volume->cz);
      targetBitmap.setRes(volume->rx,volume->rz,scale);
      targetBitmap.blank();
      break;
    case Orientation::YZ:
      targetBitmap.resize(volume->cy,volume->cz);
      targetBitmap.setRes(volume->ry,volume->rz,scale);
      targetBitmap.blank();
      break;
    default:
      break;
  }
}

ImageState::ImageState() :
  brightness(0,255), overlay1Brightness(0,255), overlay2Brightness(0,255),
  overlayAlpha(128), overlay2Alpha(128), labelAlpha(64),
  labelBrightness(255),
  zoom(0), zoomf(1.0f),
  smoothImage(false),
  viewOverlay(true), viewOverlay2(true), viewMask(true), viewLabels(true), viewInfo(true), viewVolume(true),
  showCursor(true), labelAlphaMode(true), showSlider(true),
  viewLabelOutline(false), viewFixedOverlay(false), viewCurves(false),
  threeChannel(false), maskMode(Outline)
{
}

ImageWidget::ImageWidget(ImageFrame *parent, BrainSuiteWindow *brainSuiteWindow) :
  QWidget(parent),
  serial(serialPool++), dirty(true),
  imageFrame(parent), brainSuiteWindow(brainSuiteWindow),
  imageOverlay1Bitmap(true), imageOverlay2Bitmap(true), labelBitmap(true)
{
  volumeBitmap.resize(256,256);
  volumeBitmap.setRes(1,1,1);
  volumeBitmap.blank();
  setFocusPolicy(Qt::ClickFocus);
  update();
}

int ImageWidget::serialPool=0;
bool ImageWidget::blockPainting=false;
float ImageWidget::curveCutoff = 0.5f;

template <class RenderOp>
bool ImageWidget::render(DSImage &targetBitmap,
                         const Vol3DBase *volume, const IPoint3D position,
                         RenderOp &op, DSPoint scaleFactor)
{
  if (!volume) return false;
  switch (volume->typeID())
  {
    case SILT::Uint8  : renderVolumeT(targetBitmap,*(Vol3D<uint8> *)volume,position,op,scaleFactor,orientation);  break;
    case SILT::Sint16 : renderVolumeT(targetBitmap,*(Vol3D<sint16> *)volume,position,op,scaleFactor,orientation); break;
    case SILT::Uint16 : renderVolumeT(targetBitmap,*(Vol3D<uint16> *)volume,position,op,scaleFactor,orientation); break;
    case SILT::Sint32 :
    case SILT::Uint32 : renderVolumeT(targetBitmap,*(Vol3D<uint32> *)volume,position,op,scaleFactor,orientation); break;
    case SILT::Float32: renderVolumeT(targetBitmap,*(Vol3D<float32> *)volume,position,op,scaleFactor,orientation); break;
    case SILT::Float64: renderVolumeT(targetBitmap,*(Vol3D<float64> *)volume,position,op,scaleFactor,orientation); break;
    case SILT::RGB8   : renderVolumeT(targetBitmap,*(Vol3D<rgb8> *)volume,position,op,scaleFactor,orientation); break;
    case SILT::Eigensystem3x3f :
      renderVolumeT(targetBitmap,*(Vol3D<EigenSystem3x3f> *)volume,position,op,scaleFactor,orientation); break;
    default:
      noRender(targetBitmap,volume,orientation);
      update();
      break;
  }
  return true;
}

void ImageWidget::updateOffsets()
{
  int px=0;
  int py=0;
  bool invertY=false;
  switch(orientation)
  {
    case Orientation::XY :
      invertY=true;
      px = imageState.currentPosition.x;
      py = imageState.currentPosition.y;
      break;
    case Orientation::XZ :
      invertY=true;
      px = imageState.currentPosition.x;
      py = imageState.currentPosition.z;
      break;
    case Orientation::YZ :
      invertY=true;
      px = imageState.currentPosition.y;
      py = imageState.currentPosition.z;
      break;
    default:
      break;
  }
  float dx=0;
  float dy=0;
  // these are the zoomed image sizes
  float scaledBitmapWidth=volumeBitmap.size().width()*volumeBitmap.rx*imageState.zoomf;
  float scaledBitmapHeight=volumeBitmap.size().height()*volumeBitmap.ry*imageState.zoomf;
  const float factor = 0.5f;
  const int paneWidth = imageFrame->imagePaneSize.width();
  const int paneHeight = imageFrame->imagePaneSize.height();
  if (scaledBitmapWidth>paneWidth)
  {
    float scaledCursorBitmapPositionX = (px+0.49999999f) * volumeBitmap.rx*imageState.zoomf;
    float cursorScreenPositionX=scaledBitmapWidth-scaledCursorBitmapPositionX;
    if (cursorScreenPositionX>factor*paneWidth)
    {
      dx=factor*paneWidth-cursorScreenPositionX;
      if ((dx+scaledBitmapWidth)<paneWidth)
        dx = paneWidth - scaledBitmapWidth;
    }
  }
  if (scaledBitmapHeight>paneHeight)
  {
    float scaledCursorBitmapPositionY = (py+0.49999999f) * volumeBitmap.ry*imageState.zoomf;
    float cursorScreenPositionY=scaledBitmapHeight-scaledCursorBitmapPositionY;
    if (cursorScreenPositionY>factor*paneHeight)
    {
      dy=factor*paneHeight-cursorScreenPositionY;
      if ((dy+scaledBitmapHeight)<paneHeight)
        dy = paneHeight- scaledBitmapHeight;
    }
  }
  displayOffset=QPoint(dx,dy);
}

const float ImageWidget::zoomMultiplier = pow(2.0f,0.2f);

void ImageWidget::setZoom(int level)
{
  imageState.zoom = level;
  imageState.zoomf = (float)pow(zoomMultiplier,(float)imageState.zoom);
  update();
}

inline QSizeF computeTargetSize(const DSImage &volumeBitmap, const DSImage &overlayBitmap)
{
  const float scaleX = overlayBitmap.rx / volumeBitmap.rx;
  const float scaleY = overlayBitmap.ry / volumeBitmap.ry;
  return QSizeF ((overlayBitmap.size().width() * scaleX),(overlayBitmap.size().height() * scaleY));
}

inline QPointF computeLocalOffset(const DSImage &volumeBitmap, const DSImage &overlayBitmap)
{
  const float dx = volumeBitmap.size().width() * volumeBitmap.rx;
  const float dy = volumeBitmap.size().height() * volumeBitmap.ry;
  const float dx2 = overlayBitmap.size().width() * overlayBitmap.rx;
  const float dy2 = overlayBitmap.size().height() * overlayBitmap.ry;
  const int deltaX = ((dx - dx2) / (2.0f*volumeBitmap.rx));
  const int deltaY = ((dy - dy2) / (2.0f*volumeBitmap.ry));
  return QPointF ((deltaX),(deltaY));
}

bool orEq(QImage &dst, const QImage &src)
{
  if (dst.byteCount()!=src.byteCount()) return false;
  const uint32 *s=reinterpret_cast<const uint32*>(src.bits());
  uint32 *d=reinterpret_cast<uint32*>(dst.bits());
  const int n=dst.width()*dst.height();
  for (int i=0;i<n;i++) { d[i] |= s[i]; }
  return true;
}

void ImageWidget::blendLayers()
{
  blendBitmap.resize(volumeBitmap.size().width(),volumeBitmap.size().height());
  if (imageState.threeChannel)
  {
    channelBitmap.resize(blendBitmap.size().width(),blendBitmap.size().height());
    channelBitmap.setRes(blendBitmap.rx,blendBitmap.ry,blendBitmap.scale);
  }
  bool drawnYet=false;
  QPainter blendPainter(&blendBitmap.image);
  {
    if (imageState.viewVolume && brainSuiteWindow->brainSuiteDataManager.volume)
    {
      drawnYet=true;
      blendPainter.setCompositionMode(QPainter::CompositionMode_Source);
      blendPainter.drawImage(blendBitmap.image.rect(),volumeBitmap.image);
    }
    else
    {
      drawnYet=false;
      blendPainter.fillRect(blendBitmap.image.rect(),QColor(0,0,0));
    }
  }
  if (imageState.viewOverlay && brainSuiteWindow->brainSuiteDataManager.imageOverlay1Volume)
  {
    QSizeF targetSize = computeTargetSize(volumeBitmap,imageOverlay1Bitmap);
    QPointF localOffset = computeLocalOffset(volumeBitmap,imageOverlay1Bitmap);
    QRectF targetRect(localOffset,targetSize);
    if (imageState.threeChannel)
    {
      QImage image = blendBitmap.image.copy();
      blendPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
      imageOverlay1Bitmap.setAlpha(255);
      blendPainter.drawImage(targetRect,imageOverlay1Bitmap.image);
      orEq(blendBitmap.image,image);
    }
    else
    {
      imageOverlay1Bitmap.setAlpha(drawnYet ? imageState.overlayAlpha : 255);
      if (drawnYet) blendPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
      blendPainter.drawImage(targetRect,imageOverlay1Bitmap.image);
    }
    drawnYet=true;
  }
  if (imageState.viewOverlay2 && brainSuiteWindow->brainSuiteDataManager.imageOverlay2Volume)
  {
    QSizeF targetSize = computeTargetSize(volumeBitmap,imageOverlay2Bitmap);
    QPointF localOffset = computeLocalOffset(volumeBitmap,imageOverlay2Bitmap);
    QRectF targetRect(localOffset,targetSize.toSize());
    if (imageState.threeChannel)
    {
      QImage image = blendBitmap.image.copy();
      blendPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
      imageOverlay2Bitmap.setAlpha(255);
      blendPainter.drawImage(targetRect,imageOverlay2Bitmap.image);
      orEq(blendBitmap.image,image);
    }
    else
    {
      imageOverlay2Bitmap.setAlpha(drawnYet ? imageState.overlay2Alpha : 255);
      if (drawnYet) blendPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
      blendPainter.drawImage(targetRect,imageOverlay2Bitmap.image);
    }
    drawnYet=true;
  }
  if (imageState.viewLabels && brainSuiteWindow->brainSuiteDataManager.vLabel)
  {
    if (!imageState.viewLabelOutline)
    {
      labelBitmap.setAlpha(drawnYet ? imageState.labelAlpha : 255);
      blendPainter.setCompositionMode(drawnYet ? QPainter::CompositionMode_SourceOver : QPainter::CompositionMode_Source);
      blendPainter.drawImage(blendBitmap.image.rect(),labelBitmap.image);
    }
    else
    {
      blendPainter.setCompositionMode(drawnYet ? QPainter::CompositionMode_SourceOver : QPainter::CompositionMode_Source);
      blendPainter.drawImage(blendBitmap.image.rect(),labelOutlineBitmap.image);
    }
    drawnYet=true;
  }
  if (imageState.viewMask && brainSuiteWindow->brainSuiteDataManager.vMask.size())
  {
    switch (imageState.maskMode)
    {
      case ImageState::Off:
        break;
      case ImageState::Blend :
        maskBitmap.setAlpha(128);
        blendPainter.setCompositionMode(QPainter::CompositionMode_DestinationOut);
        blendPainter.drawImage(blendBitmap.image.rect(),maskBitmap.image);
        break;
      case ImageState::Stencil :
        blendPainter.setCompositionMode(QPainter::RasterOp_SourceAndDestination);
        blendPainter.drawImage(blendBitmap.image.rect(),maskBitmap.image);
        break;
      case ImageState::Outline:
        {
          maskBitmap.setAlpha(255);
          blendPainter.setCompositionMode(QPainter::CompositionMode_DestinationOut);
          blendPainter.drawImage(blendBitmap.image.rect(),maskStencilBitmap.image);
          blendPainter.setCompositionMode(QPainter::CompositionMode_Xor);
          blendPainter.drawImage(blendBitmap.image.rect(),maskStencilBitmap.image);
        }
        break;
      case ImageState::MaskOnly :
      default:
        blendPainter.setCompositionMode(QPainter::CompositionMode_Source);
        blendPainter.drawImage(blendBitmap.image.rect(),maskBitmap.image);
        break;
    }
  }
  blendBitmap.setAlpha(255);
  update();
}

void ImageWidget::drawCurveXY(QPainter *painter, std::vector<DSPoint> &points)
{
  if (!brainSuiteWindow) return;
  const size_t np = points.size();
  if (np>1)
  {
    int z = brainSuiteWindow->currentVolumePosition.z;
    float rz = (brainSuiteWindow->brainSuiteDataManager.volume) ? brainSuiteWindow->brainSuiteDataManager.volume->rz : 1;
    float wz = (z  - 0.5f) * rz;
    float uz = wz + curveCutoff * rz;
    float lz = wz - curveCutoff * rz;
    DSPoint p0 = points[0];
    const float sx = imageState.zoomf;
    const float sy = imageState.zoomf;
    for (size_t i=1;i<np;i++)
    {
      DSPoint p1 = points[i];
      if (((p0.z > lz) || (p1.z > lz))&&((p0.z<uz)||(p1.z<uz)))
      {
        painter->drawLine(QPoint(sx*p0.x,sy*p0.y),QPoint(sx*p1.x,sy*p1.y));
      }
      p0 = p1;
    }
  }
}

void ImageWidget::drawCurveXZ(QPainter *painter, std::vector<DSPoint> &points)
{
  if (!brainSuiteWindow) return;
  const size_t np = points.size();
  if (np>1)
  {
    int y = brainSuiteWindow->currentVolumePosition.y;
    float ry = (brainSuiteWindow->brainSuiteDataManager.volume) ? brainSuiteWindow->brainSuiteDataManager.volume->ry : 1;
    float wy = (y  - 0.5f) * ry;
    float uy = wy + curveCutoff * ry;
    float ly = wy - curveCutoff * ry;
    DSPoint p0 = points[0];
    const float sx = //rx*
        imageState.zoomf;
    const float sy = //rz*
        imageState.zoomf;
    for (size_t i=1;i<np;i++)
    {
      DSPoint p1 = points[i];
      if (((p0.y > ly) || (p1.y > ly))&&((p0.y<uy)||(p1.y<uy)))
      {
        painter->drawLine(QPoint(sx*p0.x,sy*p0.z),QPoint(sx*p1.x,sy*p1.z));
      }
      p0 = p1;
    }
  }
}

void ImageWidget::drawCurveYZ(QPainter *painter, std::vector<DSPoint> &points)
{
  if (!brainSuiteWindow) return;
  const size_t np = points.size();
  if (np>1)
  {
    int x = brainSuiteWindow->currentVolumePosition.x;
    // clip based on Y
    float rx = (brainSuiteWindow->brainSuiteDataManager.volume) ? brainSuiteWindow->brainSuiteDataManager.volume->rx : 1;
    float wx = (x  - 0.5f) * rx;
    float ux = wx + curveCutoff * rx;
    float lx = wx - curveCutoff * rx;
    DSPoint p0 = points[0];
    const float sx = imageState.zoomf;
    const float sy = imageState.zoomf;
    for (size_t i=1;i<np;i++)
    {
      DSPoint p1 = points[i];
      if (((p0.x > lx) || (p1.x > lx))&&((p0.x<ux)||(p1.x<ux)))
      {
        painter->drawLine(QPoint(sx*p0.y,sy*p0.z),QPoint(sx*p1.y,sy*p1.z));
      }
      p0 = p1;
    }
  }
}

// it is more optimal to have the loop inside of the orientation, as orientation and related scale parameters are fixed per widget
void ImageWidget::drawCurve(QPainter *painter, std::vector<DSPoint> &points)
{
  if (!painter) return;
  switch(orientation)
  {
    case Orientation::XY: drawCurveXY(painter,points); break;
    case Orientation::XZ: drawCurveXZ(painter,points); break;
    case Orientation::YZ: drawCurveYZ(painter,points); break;
    default: break;
  }
}

QPoint ImageWidget::voxelCoordinateToPort(const IPoint3D &voxel)
{
  int px=0;
  int py=0;
  switch(orientation)
  {
    case Orientation::XY :
      px = voxel.x;
      py = voxel.y;
      break;
    case Orientation::XZ :
      px = voxel.x;
      py = voxel.z;
      break;
    case Orientation::YZ :
      px = voxel.y;
      py = voxel.z;
      break;
    default:
      break;
  }
  // position of the cursor in the bitmap
  float scaledBitmapPositionX = (px+0.5f) * volumeBitmap.rx*imageState.zoomf;
  float scaledBitmapPositionY = (py+0.5f) * volumeBitmap.ry*imageState.zoomf;
  return QPoint (scaledBitmapPositionX,scaledBitmapPositionY);
}

bool ImageWidget::drawSphere(QPainter &painter, const IPoint3D &position, const DSPoint &sphereColor, const float R)
{
  QColor lineColor;
  lineColor.setRgbF(sphereColor.x,sphereColor.y,sphereColor.z);
  painter.setPen(lineColor);
  IPoint3D dP = position - brainSuiteWindow->currentVolumePosition;
  {
    float r2 = 0;
    switch (orientation)
    {
      case Orientation::XY : r2 = R*R - dP.z*dP.z; break;
      case Orientation::XZ : r2 = R*R - dP.y*dP.y; break;
      case Orientation::YZ : r2 = R*R - dP.x*dP.x; break;
      default: r2=0; return false; break;
    }
    if (r2>0)
    {
      float r =  std::sqrt(r2)* imageState.zoomf;
      QPoint p2=voxelCoordinateToPort(position);
      painter.drawEllipse(p2,(int)r,(int)r);
    }
    else
      return false;
  }
  return true;
}

void ImageWidget::paintIt(QPainter &painter)
{
  if (!brainSuiteWindow) return;
  bool active=(brainSuiteWindow->activeImageWidget()==this);
  updateImage();
  QSize sz=volumeBitmap.size();
  int scaledBitmapWidth=sz.width()*volumeBitmap.rx*imageState.zoomf;
  int scaledBitmapHeight=sz.height()*volumeBitmap.ry*imageState.zoomf;
  QPoint translation(-displayOffset.x()-scaledBitmapWidth,-displayOffset.y()-scaledBitmapHeight);
  int xyrot=180;
  int xzrot=180;
  int yzrot=180;
  if (!1010101)
  {
    translation=QPoint(0,0);
    xyrot=0;
    xzrot=0;
    yzrot=0;
  }
  switch (orientation)
  {
    case Orientation::XY : painter.rotate(xyrot); painter.translate(translation); break;
    case Orientation::XZ : painter.rotate(xzrot); painter.translate(translation); break;
    case Orientation::YZ : painter.rotate(yzrot); painter.translate(translation); break;
    default: break;
  }
  painter.setClipping(true);
  painter.setCompositionMode(QPainter::CompositionMode_Source);
  painter.drawImage(QRect(QPoint(0,0),QSize(scaledBitmapWidth,scaledBitmapHeight)), blendBitmap.image, blendBitmap.image.rect());

  int px=0;
  int py=0;
  switch(orientation)
  {
    case Orientation::XY :
      px = imageState.currentPosition.x;
      py = imageState.currentPosition.y;
      break;
    case Orientation::XZ :
      px = imageState.currentPosition.x;
      py = imageState.currentPosition.z;
      break;
    case Orientation::YZ :
      px = imageState.currentPosition.y;
      py = imageState.currentPosition.z;
      break;
    default:
      break;
  }
  // position of the cursor in the bitmap
  float scaledCursorBitmapPositionX = (px+0.49999999f) * volumeBitmap.rx*imageState.zoomf;
  float scaledCursorBitmapPositionY = (py+0.49999999f) * volumeBitmap.ry*imageState.zoomf;

  EditDSBitmap *editBitmap = 0;
  if (brainSuiteWindow)
  {
    if (brainSuiteWindow->toolManager.delineationToolboxForm)
      if (brainSuiteWindow->editingLabels()||brainSuiteWindow->editingMask())
        editBitmap = brainSuiteWindow->toolManager.delineationToolboxForm->editBitmap(orientation);
  }
  if (editBitmap)
  {
    QPoint pos(scaledCursorBitmapPositionX,scaledCursorBitmapPositionY);
    const QSize size = editBitmap->size();
    const int dx = (size.width() * volumeBitmap.rx * imageState.zoomf);
    const int dy = (size.height() * volumeBitmap.ry * imageState.zoomf);
    QPoint p0(pos.x()-dx/2.0f,pos.y()-dy/2.0f);
    QRect destRect(p0,QSize(dx,dy));
    QPainter::CompositionMode currentMode=painter.compositionMode();
    painter.setCompositionMode(QPainter::RasterOp_SourceXorDestination);
    painter.drawImage(destRect, editBitmap->image, editBitmap->image.rect());
    painter.setCompositionMode(currentMode);
  }
  if (imageState.showCursor)
  {
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    QColor lineColor = active ? 0xFFFF00 : 0x0000FF;
    painter.setPen(lineColor);
    int x0=0;
    int y0=0;
    int y1=scaledBitmapHeight;
    int x1=scaledBitmapWidth;
    painter.drawLine(QPoint(scaledCursorBitmapPositionX,y0),QPoint(scaledCursorBitmapPositionX,y1));
    painter.drawLine(QPoint(x0,scaledCursorBitmapPositionY),QPoint(x1,scaledCursorBitmapPositionY));
  }
  if (brainSuiteWindow->toolManager.diffusionToolboxForm &&
      brainSuiteWindow->toolManager.diffusionToolboxForm->showSphereROIsInSliceView())
  {
    auto &sphereROIs=brainSuiteWindow->toolManager.diffusionToolboxForm->sphereROIs;
    for (auto sphere=sphereROIs.begin();sphere!=sphereROIs.end();sphere++)
    {
      if (!sphere->active) continue;
      drawSphere(painter,
                 sphere->voxelPosition,
                 sphere->color,
                 sphere->radius);
    }
  }
  if (brainSuiteWindow->brainSuiteDataManager.protocolCurveSet)
  {
    if (BrainSuiteSettings::showCurves && BrainSuiteSettings::showCurvesOnImageSlices)
    {
      const int selectedCurve = (brainSuiteWindow->toolManager.curveToolboxForm) ? brainSuiteWindow->toolManager.curveToolboxForm->currentCurveIndex() : -1;
      ProtocolCurveSet *cp = brainSuiteWindow->brainSuiteDataManager.protocolCurveSet;
      const size_t nc = cp->curves.size();
      for (size_t i=0; i<nc;i++)
      {
        ProtocolCurve &c(cp->curves[i]);
        QColor color=QColor::fromRgbF(c.attributes.color.x,c.attributes.color.y,c.attributes.color.z);
        float width = (selectedCurve==(int)i) ? 2.0f*BrainSuiteSettings::curveProtocolBrushSize : BrainSuiteSettings::curveProtocolBrushSize;
        QPen pOld=painter.pen();
        QBrush brush(color);
        QPen pen(brush,width);
        painter.setPen(pen);
        drawCurve(&painter,c.points);
        painter.setPen(pOld);
      }
    }
  }
  if (BrainSuiteSettings::showOrientation)
  {
    const int textBoxWidth=20;
    const int textBoxHeight=20;
    painter.resetTransform();
    int mx=frameSize().width();
    if (scaledBitmapWidth<mx) mx = scaledBitmapWidth;
    int my=frameSize().height();
    if (scaledBitmapHeight<my) my = scaledBitmapHeight;

    int px = (mx - textBoxWidth) / 2;
    int py = (my - textBoxHeight) / 2;
    QFont font=painter.font();
    font.setBold(true);
    font.setPointSize(10);
    painter.setFont(font);
    QSize textBoxSize(textBoxWidth,textBoxHeight);
    QRect rectN(QPoint(px,0),textBoxSize);
    QRect rectS(QPoint(px,my-textBoxHeight),textBoxSize);
    QRect rectW(QPoint(0,py),textBoxSize);
    QRect rectE(QPoint(mx-textBoxWidth,py),textBoxSize);
    painter.setPen(QColor(0,255,0));
    std::string N,S,E,W;
    switch (orientation)
    {
      case Orientation::XY : N="A";S="P";W="R";E="L"; break;
      case Orientation::XZ : N="S";S="I";W="R";E="L"; break;
      case Orientation::YZ : N="S";S="I";W="A";E="P"; break;
      case Orientation::Oblique : break;
    }
    painter.drawText(rectN,Qt::AlignCenter,N.c_str());
    painter.drawText(rectE,Qt::AlignCenter,E.c_str());
    painter.drawText(rectW,Qt::AlignCenter,W.c_str());
    painter.drawText(rectS,Qt::AlignCenter,S.c_str());
  }
}

void ImageWidget::paintEvent(QPaintEvent *)
{
  if (blockPainting) return;
  QPainter painter(this);
  paintIt(painter);

}

IPoint3D scaledRenderPoint(Vol3DBase *vol, Vol3DBase *vOverlay, Orientation::Code orientation, IPoint3D p)
{              
  if (!vOverlay) return p;
  if (!vol) return p;
  switch (orientation)
  {
    case Orientation::XY :
      p.z = (int)(0.5f+(0.5f * vOverlay->cz) + (p.z - 0.5f * vol->cz) * (vol->rz / vOverlay->rz));
      break;
    case Orientation::XZ :
      p.y = (int)(0.5f+(p.y * vol->ry - (0.5f * vol->cy * vol->ry) + (0.5f * vOverlay->cy * vOverlay->ry)) / vOverlay->ry);
      break;
    case Orientation::YZ :
      p.x = (int)(0.5f+(p.x * vol->rx - (0.5f * vol->cx * vol->rx) + (0.5f * vOverlay->cx * vOverlay->rx)) / vOverlay->rx);
      break;
    case Orientation::Oblique : break;
  }
  return p;
}

bool ImageWidget::renderScaledVolume(DSImage &targetBitmap, const uint32 *LUT,
                                     const Vol3DBase *volume, const ImageScale &brightnessScale,
                                     const IPoint3D position)
{
  if (!volume) return false;
  if (brightnessScale.minv!=0)
  {
    RangeLUTOp op(LUT,brightnessScale.minv,brightnessScale.maxv);
    return render(targetBitmap,volume,position,op,DSPoint(1,1,1));
  }
  if (volume->typeID()==SILT::Eigensystem3x3f && BrainSuiteSettings::useColorFA)
  {
    ColorFAOp op((float)brightnessScale.maxv);
    return render(targetBitmap,volume,position,op,DSPoint(1,1,1));
  }
  else
  {
    LUTOp op(LUT ? LUT : ColorMap::greyLUT,(float)brightnessScale.maxv);
    return render(targetBitmap,volume,position,op,DSPoint(1,1,1));
  }
}

bool ImageWidget::renderVolume(const Vol3DBase *volume, const IPoint3D position)
{
  uint32 *volumeLUT=imageState.threeChannel ? ColorMap::redLUT : brainSuiteWindow->imageLUT;
  if (!volumeLUT) volumeLUT = ColorMap::greyLUT;
  return renderScaledVolume(volumeBitmap,volumeLUT,volume,imageState.brightness,position);
}

bool ImageWidget::markDirty()
{
  dirty = true;
  update();
  return true;
}

void ImageWidget::stencilOutline(DSImage &stencilBitmap, const DSImage &labelBitmap)
{
  const uint32 stencilColor = 0xFF00FF00; // include alpha channel
  const uint32 backgroundColor = 0x0;
  stencilBitmap.resize(labelBitmap.size());
  const uint32 *src = reinterpret_cast<const uint32 *>(labelBitmap.image.bits());
  uint32 *dst = reinterpret_cast<uint32 *>(stencilBitmap.image.bits());
  const int cx = stencilBitmap.size().width();
  const int cy = stencilBitmap.size().height();
  if (cy>0)
    for (int x=0;x<cx;x++) *dst++ = (*src++!=0) ? stencilColor : backgroundColor;
  for (int y=1;y<cy-1;y++)
  {
    *dst++ = (*src++!=0) ? stencilColor : backgroundColor;
    for (int x=1;x<cx-1;x++)
    {
      const uint32 v = *src;
      *dst++ = ((v!=0) && ((src[-1]!=v)||(src[+1]!=v)||(src[-cx]!=v)||(src[+cx]!=v))) ? stencilColor : backgroundColor;
      src++;
    }
    *dst++ = (*src++!=0) ? stencilColor : backgroundColor;
  }
  if (cy>0)
    for (int x=0;x<cx;x++) *dst++ = (*src++!=0) ? stencilColor : backgroundColor;
}

void ImageWidget::stencilLabelOutline(DSImage &labelOutlineBitmap, const DSImage &labelBitmap)
{
  labelOutlineBitmap=labelBitmap;
  const uint32 *src = reinterpret_cast<const uint32 *>(labelBitmap.image.bits());
  uint32 *dst = reinterpret_cast<uint32 *>(labelOutlineBitmap.image.bits());
  const int cx = labelOutlineBitmap.size().width();
  const int cy = labelOutlineBitmap.size().height();
  if (cy>0) for (int x=0;x<cx;x++) *dst++ = (*src++|0xFF000000);
  for (int y=1;y<cy-1;y++)
  {
    *dst++ = (*src++|0xFF000000);
    for (int x=1;x<cx-1;x++)
    {
      const uint32 v = *src;
      *dst++ = ((src[0]!=0) && ((src[-1]!=v)||(src[+1]!=v)||(src[-cx]!=v)||(src[+cx]!=v))) ? (*src|0xFF000000) : 0x0;
      src++;
    }
    *dst++ = (*src++|0xFF000000);
  }
  if (cy>0) for (int x=0;x<cx;x++) *dst++ = *src++;
}

void ImageWidget::updateImage()
{
  if (dirty)
  {
    if (brainSuiteWindow)
    {
      Vol3DBase *volume = brainSuiteWindow->brainSuiteDataManager.volume;
      Vol3DBase *overlay1 = brainSuiteWindow->brainSuiteDataManager.imageOverlay1Volume;
      Vol3DBase *overlay2 = brainSuiteWindow->brainSuiteDataManager.imageOverlay2Volume;
      renderVolume(brainSuiteWindow->brainSuiteDataManager.volume,brainSuiteWindow->currentVolumePosition);
      IPoint3D overlay1Point = scaledRenderPoint(volume,overlay1,orientation,brainSuiteWindow->currentVolumePosition);
      renderScaledVolume(imageOverlay1Bitmap,imageState.threeChannel ? ColorMap::greenLUT : brainSuiteWindow->imageOverlay1LUT,brainSuiteWindow->brainSuiteDataManager.imageOverlay1Volume,imageState.overlay1Brightness,overlay1Point);
      IPoint3D overlay2Point = scaledRenderPoint(volume,overlay2,orientation,brainSuiteWindow->currentVolumePosition);
      renderScaledVolume(imageOverlay2Bitmap,imageState.threeChannel ? ColorMap::blueLUT : brainSuiteWindow->imageOverlay2LUT,brainSuiteWindow->brainSuiteDataManager.imageOverlay2Volume,imageState.overlay2Brightness,overlay2Point);
      IPoint3D labelPoint = scaledRenderPoint(volume,brainSuiteWindow->brainSuiteDataManager.vLabel,orientation,brainSuiteWindow->currentVolumePosition);
      LabelOp labelOp;
      render(labelBitmap,brainSuiteWindow->brainSuiteDataManager.vLabel,labelPoint,labelOp,DSPoint(1,1,1));
      if (imageState.viewLabelOutline)
        stencilLabelOutline(labelOutlineBitmap,labelBitmap);
      if (brainSuiteWindow->brainSuiteDataManager.vMask.size())
      {
        IPoint3D maskPoint = scaledRenderPoint(volume,&brainSuiteWindow->brainSuiteDataManager.vMask,orientation,brainSuiteWindow->currentVolumePosition);
        RawOp nop;
        renderVolumeT(maskBitmap,brainSuiteWindow->brainSuiteDataManager.vMask,maskPoint,nop,DSPoint(1,1,1),orientation);
        if (imageState.maskMode==ImageState::Outline)
          stencilOutline(maskStencilBitmap,maskBitmap);
      }
      blendLayers();
      imageState.currentPosition = brainSuiteWindow->currentVolumePosition;
      dirty = false;
    }
  }
}

IPoint3D ImageWidget::getPosition(int x, int y)
{
  x -= displayOffset.x();
  y -= displayOffset.y();
  IPoint3D clickPosition(brainSuiteWindow->currentVolumePosition);
  float scale = 1.0f;// leaving this here for image scaling;
  const float rx_ = (scale>0) ? imageState.zoomf*volumeBitmap.rx/scale : imageState.zoomf;
  const float ry_ = (scale>0) ? imageState.zoomf*volumeBitmap.ry/scale : imageState.zoomf;
  int xScaled = (int)(volumeBitmap.size().width() - x/rx_);
  int yScaled = (int)(volumeBitmap.size().height() - y / (ry_));
  if (!1010101)
  {
    xScaled = (int)(x/rx_);
    yScaled = (int)(y/ry_);
  }
  switch (orientation)
  {
    case Orientation::XY :
      clickPosition.x = (int)xScaled;
      clickPosition.y = (int)yScaled;
      break;
    case Orientation::XZ :
      clickPosition.x = (int)xScaled;
      clickPosition.z = (int)yScaled;
      break;
    case Orientation::YZ :
      clickPosition.y = (int)xScaled;
      clickPosition.z = (int)yScaled;
      break;
    default:
      break;
  }
  return clickPosition;
}

void ImageWidget::wheelEvent ( QWheelEvent * event )
{
  int delta = event->delta()/120;
  switch (orientation)
  {
    case Orientation::XY:
      brainSuiteWindow->setVolumePosition(brainSuiteWindow->currentVolumePosition + IPoint3D(0,0,delta));
      break;
    case Orientation::XZ:
      brainSuiteWindow->setVolumePosition(brainSuiteWindow->currentVolumePosition + IPoint3D(0,delta,0));
      break;
    case Orientation::YZ:
      brainSuiteWindow->setVolumePosition(brainSuiteWindow->currentVolumePosition + IPoint3D(delta,0,0));
      break;
    default:
      break;
  }
}

void ImageWidget::mousePressEvent(QMouseEvent *event)
{
  if (!(brainSuiteWindow->editingMask() || brainSuiteWindow->editingLabels())&& (event->buttons() & Qt::RightButton))
  {
    LUTMenuSystem menuSystem(brainSuiteWindow);
    menuSystem.doPopup(event->globalPos());
  }
  else
  {
    brainSuiteWindow->mousePressed(this,getPosition(event->x(),event->y()),event);
    lastClick=event->pos();
    grabMouse();
  }
}

void ImageWidget::mouseMoveEvent(QMouseEvent *event)
{
  if (brainSuiteWindow)
  {
    IPoint3D ip=getPosition(event->x(),event->y());
    brainSuiteWindow->mouseMoveTo(this,ip,event,false);
  }
  lastClick=event->pos();
}

void ImageWidget::mouseReleaseEvent(QMouseEvent *event)
{
  if (brainSuiteWindow)
  {
    brainSuiteWindow->mouseReleased(this,getPosition(event->x(),event->y()),event);
  }
  lastClick=event->pos();
  releaseMouse();
}

void ImageWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
  if (brainSuiteWindow)
  {
    releaseMouse();
    brainSuiteWindow->mouseDoubleClicked(this,getPosition(event->x(),event->y()),event);
  }
}

void ImageWidget::validateOffsets()
{
  float scaledBitmapWidth=volumeBitmap.size().width()*volumeBitmap.rx*imageState.zoomf;
  float scaledBitmapHeight=volumeBitmap.size().height()*volumeBitmap.ry*imageState.zoomf;
  const int paneWidth = imageFrame->imagePaneSize.width();
  const int paneHeight = imageFrame->imagePaneSize.height();
  if (paneWidth>scaledBitmapWidth) displayOffset.setX(0);
  if (paneHeight>scaledBitmapHeight) displayOffset.setX(0);
  if (displayOffset.x()<(paneWidth-scaledBitmapWidth))
    displayOffset.setX(paneWidth-scaledBitmapWidth);
  if (displayOffset.y()<(paneHeight-scaledBitmapHeight))
    displayOffset.setY(paneHeight-scaledBitmapHeight);
  switch (orientation)
  {
    case Orientation::XY :
      if (displayOffset.x()>0) displayOffset.setX(0);
      if (displayOffset.y()>0) displayOffset.setY(0);
      break;
    case Orientation::XZ :
      if (displayOffset.x()>0) displayOffset.setX(0);
      if (displayOffset.y()>0) displayOffset.setY(0);
      break;
    case Orientation::YZ :
      if (displayOffset.y()>0) displayOffset.setY(0);
      if (displayOffset.x()>0) displayOffset.setX(0);
      break;
    case Orientation::Oblique : break;
  }
}

void ImageWidget::shiftOffsets(QPoint point)
{
  displayOffset+=point;
  validateOffsets();
  update();
}

void ImageWidget::resetOffsets()
{
  if (displayOffset!=QPoint(0,0))
  {
    displayOffset=QPoint(0,0);
    update();
  }
}

void ImageWidget::keyUp(const int step)
{
  if (!brainSuiteWindow) return;
  switch (orientation)
  {
    case Orientation::XY:
      brainSuiteWindow->setVolumePosition(brainSuiteWindow->currentVolumePosition + IPoint3D(0,0,step));
      break;
    case Orientation::XZ:
      brainSuiteWindow->setVolumePosition(brainSuiteWindow->currentVolumePosition + IPoint3D(0,step,0));
      break;
    case Orientation::YZ:
      brainSuiteWindow->setVolumePosition(brainSuiteWindow->currentVolumePosition + IPoint3D(step,0,0));
      break;
    default:
      break;
  }
}

void ImageWidget::keyDown(const int step)
{
  if (!brainSuiteWindow) return;
  switch (orientation)
  {
    case Orientation::XY:
      brainSuiteWindow->setVolumePosition(brainSuiteWindow->currentVolumePosition + IPoint3D(0,0,-step));
      break;
    case Orientation::XZ:
      brainSuiteWindow->setVolumePosition(brainSuiteWindow->currentVolumePosition + IPoint3D(0,-step,0));
      break;
    case Orientation::YZ:
      brainSuiteWindow->setVolumePosition(brainSuiteWindow->currentVolumePosition + IPoint3D(-step,0,0));
      break;
    default:
      break;
  }
}

float ImageWidget::zoomBestFit()
{
  if (!brainSuiteWindow) return 1.0f;
  float scale = 1.0f;// leaving this here for image scaling;
  float rx = (scale>0) ? volumeBitmap.rx : 1.0f;
  float ry = (scale>0) ? volumeBitmap.ry : 1.0f;
  QSize sz=volumeBitmap.size();
  float mx=sz.width()*rx;
  float my=sz.height()*ry;
  const int tx = imageFrame->imagePaneSize.width();
  const int ty = imageFrame->imagePaneSize.height();
  float zx = tx / mx;
  float zy = ty / my;
  return std::min(zx,zy);
}
