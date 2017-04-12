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

#include "dsimagerangecontrol.h"
#include <QPaintEvent>
#include <qpainter.h>
#include <colormap.h>
#include <lutmenusystem.h>
#include <imagewidget.h>
#include <brainsuitesettings.h>

const int DSImageRangeControl::triangleWidth = 6;

DSImageRangeControl::DSImageRangeControl(QWidget *parent) :
  QWidget(parent), symmetric(false), upperVal(255.0f), lowerVal(0.0f), minValue(0.0f), maxValue(0.0f)
{
  lutBox.resize(1,256);
  setLUT(ColorMap::jetLUT);
}

void DSImageRangeControl::setLUT(uint32 *LUT)
{
  QRgb* pixel = lutBox.pixels();
  for (int i=0;i<256;i++)
    pixel[i] = LUT[255-i];
  update();
}

void DSImageRangeControl::setRange(const float rMin, const float rMax)
{
  lowerVal=minValue = rMin;
  upperVal=maxValue = rMax;
  update();
}

void DSImageRangeControl::setRange(const ImageScale &scale)
{	
  setRange(scale.minv,scale.maxv);
}

ImageScale DSImageRangeControl::imageScale() const
{
  return ImageScale(lowerVal,upperVal);
}

void DSImageRangeControl::paintEvent(QPaintEvent *)
{
  QRect rec = rect();
  QPainter painter(this);
  int mx = rec.height() - 2 * triangleWidth;
  if (upperVal>maxValue) upperVal = maxValue;
  if (lowerVal>maxValue) lowerVal = maxValue;
  if (upperVal<minValue) upperVal = minValue;
  if (lowerVal<minValue) lowerVal = minValue;
  const float range = maxValue - minValue;
  const float low   = (lowerVal - minValue)/range;
  const float high = (upperVal - minValue)/range;
  const int lowPos = (int)(low * mx);
  const int highPos = (int)(high * mx);
  QPoint offset(triangleWidth,triangleWidth);
  int h = mx;
  int w = rec.width() - triangleWidth;
  int x0 = triangleWidth;
  int y0 = triangleWidth;
  QPoint top(x0,y0);
  QPoint bot(w-x0,h-2*triangleWidth);
  QPoint lowerMarker(triangleWidth,triangleWidth+(h) - lowPos);
  QPoint upperMarker(triangleWidth,triangleWidth+(h) - highPos);
  QRect colorRange(lowerMarker,upperMarker+QPoint(5,0));
  int rightEdge = rec.width()-triangleWidth-1;
  QRect colorbarActiveRect(upperMarker,QPoint(rightEdge,lowerMarker.y()-1));
  QRect colorbarUpperRect(QPoint(triangleWidth,y0),QPoint(rightEdge,upperMarker.y()-1));
  QRect colorbarLowerRect(lowerMarker,QPoint(rightEdge,rec.height()-triangleWidth));
  painter.drawImage(colorbarActiveRect,lutBox.image);
  painter.fillRect(colorbarUpperRect,lutBox.image.pixel(0,0));
  painter.fillRect(colorbarLowerRect,lutBox.image.pixel(0,255));
  painter.setPen(QColor(0,0,0));
  painter.setBrush(Qt::SolidPattern);
  std::vector<QPoint> triangle(4);
  triangle[0] = lowerMarker;
  triangle[1] = lowerMarker+QPoint(-triangleWidth,-triangleWidth);
  triangle[2] = lowerMarker+QPoint(-triangleWidth,+triangleWidth);
  triangle[3] = lowerMarker;
  painter.drawPolygon(&triangle[0],(int)triangle.size());
  triangle[0] = upperMarker;
  triangle[1] = upperMarker+QPoint(-triangleWidth,-triangleWidth);
  triangle[2] = upperMarker+QPoint(-triangleWidth,+triangleWidth);
  triangle[3] = upperMarker;
  painter.drawPolygon(&triangle[0],(int)triangle.size());
}

void DSImageRangeControl::mousePressEvent(QMouseEvent *event)
{
  if (event->buttons() & Qt::RightButton)
  {
    emit rightClick(event->globalPos());
  }
  else
  {
    grabMouse();
    mouseMoveEvent(event);
  }
}

void DSImageRangeControl::mouseReleaseEvent(QMouseEvent * event)
{
  mouseMoveEvent(event);
  releaseMouse();
}

void DSImageRangeControl::mouseMoveEvent(QMouseEvent *ev)
{
  lastPoint = ev->pos();
  QPoint point=ev->pos();
  int mx = rect().height() - 2 * triangleWidth - 1;
  int position = (rect().height() - triangleWidth) - point.y();
  float split = (lowerVal + upperVal)/2;
  const float range = maxValue - minValue;
  float scaledPosition = position*range/mx + minValue;
  if ((ev->buttons() & Qt::MiddleButton) || (ev->modifiers()&Qt::ControlModifier))
  {
    float delta = upperVal - lowerVal;
    upperVal = scaledPosition + delta/2.0f;
    if (upperVal < minValue) upperVal = minValue + delta;
    if (upperVal > maxValue) upperVal = maxValue;
    lowerVal = upperVal - delta;
    if (lowerVal < minValue)
    {
      lowerVal = minValue;
      upperVal = lowerVal + delta;
      if (upperVal > maxValue) upperVal = maxValue;
    }
  }
  else if (ev->buttons() & Qt::LeftButton)
  {
    if (scaledPosition<split)
    {
      lowerVal = scaledPosition;
      if (lowerVal < minValue) lowerVal = minValue;
      if (lowerVal > maxValue) lowerVal = maxValue;
      if (symmetric) upperVal=-lowerVal;
    }
    else
    {
      upperVal = scaledPosition;
      if (upperVal < minValue) upperVal = minValue;
      if (upperVal > maxValue) upperVal = maxValue;
      if (symmetric) lowerVal=-upperVal;
    }
  }
  else return;
  emit rangeChanged();
  update();
}
