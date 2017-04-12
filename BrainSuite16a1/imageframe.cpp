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

#include "imageframe.h"
#include "ui_imageframe.h"
#include <imagewidget.h>
#include <brainsuitewindow.h>
#include <qevent.h>

ImageFrame::ImageFrame(BrainSuiteWindow *parent) :
  QFrame(parent),
  brainSuiteWindow(parent),
  ui(new Ui::ImageFrame)
{
  ui->setupUi(this);
  imageWidget = new ImageWidget(this,parent);
  ui->imagePositionText->setText("");
  ui->imagePositionText->setFocusPolicy(Qt::ClickFocus);
}

ImageFrame::~ImageFrame()
{
  delete ui;
  delete imageWidget;
}

void ImageFrame::setSizes()
{
  const int sliderWidth=16;
  const int textHeight=16;
  const int border=2;
  const int imageX = sliderWidth + border;
  const int imageY = textHeight + border;
  const int imageWidth = rect().width() - imageX;
  const int imageHeight = rect().height() - imageY;
  imagePaneOrigin = QPoint(imageX,imageY);
  imagePaneSize = QSize(imageWidth,imageHeight);
  ui->verticalSlider->setGeometry(QRect(0,textHeight,sliderWidth,imageHeight));
  ui->imagePositionText->setGeometry(QRect(imageX,0,imageWidth,textHeight));
  imageWidget->setGeometry(QRect(imagePaneOrigin,imagePaneSize));
  std::ostringstream ostr;
  if (brainSuiteWindow->brainSuiteDataManager.volume)
  {
    ostr<<brainSuiteWindow->currentVolumePosition<<'\t'<<imageWidget->imageState.zoomf;
    ui->imagePositionText->setText(ostr.str().c_str());
  }
}

void ImageFrame::resizeEvent(QResizeEvent *)
{
  setSizes();
}

void ImageFrame::updateSlider()
{
  Vol3DBase *vol = brainSuiteWindow->brainSuiteDataManager.volume;
  if (!vol) return;
  int max=0;
  int pos=0;
  if (image())
    switch (image()->orientation)
    {
      case Orientation::XY : max = vol->cz; pos=brainSuiteWindow->currentVolumePosition.z; break;
      case Orientation::XZ : max = vol->cy; pos=brainSuiteWindow->currentVolumePosition.y; break;
      case Orientation::YZ : max = vol->cx; pos=brainSuiteWindow->currentVolumePosition.x; break;
      default: break;
    }
  ui->verticalSlider->setRange(0,max-1);
  ui->verticalSlider->setValue(pos);
}

void ImageFrame::updateText()
{
  std::ostringstream ostr;
  ostr<<brainSuiteWindow->currentVolumePosition<<'\t'<<imageWidget->imageState.zoomf;
  ui->imagePositionText->setText(ostr.str().c_str());
}

void ImageFrame::on_verticalSlider_valueChanged(int value)
{
  if (image())
  {
    IPoint3D newPosition(brainSuiteWindow->currentVolumePosition);
    switch (image()->orientation)
    {
      case Orientation::XY : newPosition.z=value; break;
      case Orientation::XZ : newPosition.y=value; break;
      case Orientation::YZ : newPosition.x=value; break;
      default: break;
    }
    brainSuiteWindow->setVolumePosition(newPosition);
  }
}
