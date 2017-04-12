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

#include "dsrangeslider.h"
#include <QPaintEvent>
#include <qpainter.h>
#include <dsimage.h>
#include <colormap.h>
#include <qmimedata.h>

DSRangeSlider::DSRangeSlider(QWidget *parent) :
  QSlider(parent)
{
	lutColor.resize(1,256);
  setLUT(ColorMap::jetLUT);
}

void DSRangeSlider::setLUT(uint32 *LUT)
{
	QRgb* pixel = lutColor.pixels();
	for (int i=0;i<256;i++)
		pixel[i] = LUT[i];
}

void DSRangeSlider::paintEvent(QPaintEvent * /*ev*/)
{
	QRect r = rect();
	QPainter painter(this);
	painter.drawImage(r,lutColor.image);
}

void DSRangeSlider::dragEnterEvent(QDragEnterEvent *event)
{
  if (event->mimeData()->hasFormat("text/uri-list"))
  {
    event->acceptProposedAction();
  }
}

void DSRangeSlider::dropEvent(QDropEvent */*event*/)
{
}
