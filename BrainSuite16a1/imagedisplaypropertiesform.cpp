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

#include "imagedisplaypropertiesform.h"
#include "ui_imagedisplaypropertiesform.h"
#include <brainsuitewindow.h>
#include <qmessagebox.h>
#include <qdir.h>
#include <colormap.h>

void setWidgetText(DSFileLabel *dsFileLabel, Vol3DBase *volume)
{
  if (!dsFileLabel) return;
  if (!volume)
  {
    dsFileLabel->setText("");
    dsFileLabel->setToolTip("");
  }
  else
  {
    dsFileLabel->setText(StrUtil::shortName(volume->filename).c_str());
  }
}

ImageDisplayPropertiesForm::ImageDisplayPropertiesForm(BrainSuiteWindow *brainSuiteWindow, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::ImageDisplayPropertiesForm), brainSuiteWindow(brainSuiteWindow)
{
  ui->setupUi(this);
  ui->closeOverlay1PushButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogCloseButton));
  ui->closeOverlay2PushButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogCloseButton));
  ui->closeLabelVolumePushButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogCloseButton));
  ui->closeMaskVolumePushButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogCloseButton));
  updateDisplay();
  updateNames();
  setSliderRanges();
}

bool ImageDisplayPropertiesForm::setLUT1symmetric(bool state)
{
  ui->lutSymmetricPushButton->setChecked(state);
  imageSlider()->symmetric=state;
  return true;
}

bool ImageDisplayPropertiesForm::setLUT2symmetric(bool state)
{
  ui->lut2SymmetricPushButton->setChecked(state);
  imageOverlay1Slider()->symmetric=state;
  return true;
}

bool ImageDisplayPropertiesForm::setLUT3symmetric(bool state)
{
  ui->lut3SymmetricPushButton->setChecked( state);
  imageOverlay2Slider()->symmetric= state;
  return true;
}

QSize ImageDisplayPropertiesForm::sizeHint() const
{
  return QSize(280,480);
}

ImageDisplayPropertiesForm::~ImageDisplayPropertiesForm()
{
  delete ui;
}

DSImageRangeControl *ImageDisplayPropertiesForm::imageSlider() { return ui->imageVolumeBrightnessSlider; }
DSImageRangeControl *ImageDisplayPropertiesForm::imageOverlay1Slider()  { return ui->imageOverlay1BrightnessSlider; }
DSImageRangeControl *ImageDisplayPropertiesForm::imageOverlay2Slider()  { return ui->imageOverlay2BrightnessSlider; }


void ImageDisplayPropertiesForm::setImageVolumeSliderRange()
{
  ui->imageVolumeBrightnessSlider->setRange(brainSuiteWindow->imageState.brightness);

}

void ImageDisplayPropertiesForm::setImageOverlay1SliderRange()
{
  ui->imageOverlay1BrightnessSlider->setRange(brainSuiteWindow->imageState.overlay1Brightness);

}

void ImageDisplayPropertiesForm::setImageOverlay2SliderRange()
{
  ui->imageOverlay2BrightnessSlider->setRange(brainSuiteWindow->imageState.overlay2Brightness);
}

void ImageDisplayPropertiesForm::setSliderRanges()
{
  setImageVolumeSliderRange();
  setImageOverlay1SliderRange();
  setImageOverlay2SliderRange();
}

void ImageDisplayPropertiesForm::updateDisplay()
{
  updateNames();
  if (brainSuiteWindow)
  {
    ui->showVolumeCheckBox->setChecked(brainSuiteWindow->imageState.viewVolume);
    const ImageState &imageState(brainSuiteWindow->imageState);
    ui->imageVolumeMinText->setText(QString::number(brainSuiteWindow->imageState.brightness.minv));
    ui->imageVolumeMaxText->setText(QString::number(brainSuiteWindow->imageState.brightness.maxv));
    ui->imageVolumeBrightnessSlider->setLUT(brainSuiteWindow->imageState.threeChannel ? ColorMap::redLUT : brainSuiteWindow->imageLUT);

    ui->showOverlay1VolumeCheckBox->setChecked(brainSuiteWindow->imageState.viewOverlay);
    ui->imageOverlay1Alpha->setText(QString::number(imageState.overlayAlpha));
    ui->imageOverlay1AlphaSlider->setValue(imageState.overlayAlpha);
    ui->imageOverlay1MinText->setText(QString::number(brainSuiteWindow->imageState.overlay1Brightness.minv));
    ui->imageOverlay1MaxText->setText(QString::number(brainSuiteWindow->imageState.overlay1Brightness.maxv));
    ui->imageOverlay1BrightnessSlider->setLUT(brainSuiteWindow->imageState.threeChannel ? ColorMap::greenLUT : brainSuiteWindow->imageOverlay1LUT);

    ui->showOverlay2VolumeCheckBox->setChecked(brainSuiteWindow->imageState.viewOverlay2);ui->imageOverlay2Alpha->setText(QString::number(imageState.overlay2Alpha));
    ui->imageOverlay2AlphaSlider->setValue(imageState.overlay2Alpha);
    ui->imageOverlay2MinText->setText(QString::number(brainSuiteWindow->imageState.overlay2Brightness.minv));
    ui->imageOverlay2MaxText->setText(QString::number(brainSuiteWindow->imageState.overlay2Brightness.maxv));
    ui->imageOverlay2BrightnessSlider->setLUT(brainSuiteWindow->imageState.threeChannel ? ColorMap::blueLUT : brainSuiteWindow->imageOverlay2LUT);

    ui->showLabelVolumeCheckBox->setChecked(brainSuiteWindow->imageState.viewLabels);
    ui->labelAlphaText->setText(QString::number(brainSuiteWindow->imageState.labelAlpha));
    ui->labelAlphaSlider->setValue(brainSuiteWindow->imageState.labelAlpha);
    update();
  }
}

void ImageDisplayPropertiesForm::updateNames()
{
  if (brainSuiteWindow)
  {
    setWidgetText(ui->imageVolumeFilename,brainSuiteWindow->brainSuiteDataManager.volume);
    setWidgetText(ui->imageOverlay1Filename,brainSuiteWindow->brainSuiteDataManager.imageOverlay1Volume);
    setWidgetText(ui->imageOverlay2Filename,brainSuiteWindow->brainSuiteDataManager.imageOverlay2Volume);
    setWidgetText(ui->labelVolumeFilename,brainSuiteWindow->brainSuiteDataManager.vLabel);
    if (brainSuiteWindow->brainSuiteDataManager.vMask.size())
    {
      setWidgetText(ui->maskVolumeFilename,&brainSuiteWindow->brainSuiteDataManager.vMask);
    }
    else
      ui->maskVolumeFilename->setText("");
  }
  else
  {
    ui->imageVolumeFilename->setText("");
    ui->imageOverlay1Filename->setText("");
    ui->imageOverlay2Filename->setText("");
    ui->labelVolumeFilename->setText("");
    ui->maskVolumeFilename->setText("");
  }
}

void ImageDisplayPropertiesForm::on_imageVolumeButton_clicked()
{
  if (brainSuiteWindow)
  {
    brainSuiteWindow->loadVolume();
  }
}

void ImageDisplayPropertiesForm::on_imageOverlay1Button_clicked()
{
  if (brainSuiteWindow)
  {
    brainSuiteWindow->loadOverlay1Volume();
  }
}

void ImageDisplayPropertiesForm::on_imageOverlay2Button_clicked()
{
  if (brainSuiteWindow)
  {
    brainSuiteWindow->loadOverlay2Volume();
  }
}

void ImageDisplayPropertiesForm::on_labelVolumeButton_clicked()
{
  if (brainSuiteWindow)
  {
    brainSuiteWindow->loadLabelVolume();
  }
}

void ImageDisplayPropertiesForm::on_maskVolumeButton_clicked()
{
  if (brainSuiteWindow)
  {
    brainSuiteWindow->loadMaskVolume();
  }
}

void ImageDisplayPropertiesForm::on_imageOverlay1AlphaSlider_valueChanged(int value)
{
  brainSuiteWindow->setOverlay1Alpha(value);
  ui->imageOverlay1Alpha->setText(QString::number(brainSuiteWindow->imageState.overlayAlpha));
}

void ImageDisplayPropertiesForm::on_imageOverlay2AlphaSlider_valueChanged(int value)
{
  brainSuiteWindow->setOverlay2Alpha(value);
  ui->imageOverlay2Alpha->setText(QString::number(brainSuiteWindow->imageState.overlay2Alpha));
}

void ImageDisplayPropertiesForm::on_labelAlphaSlider_valueChanged(int value)
{
  brainSuiteWindow->setLabelAlpha(value);
  ui->labelAlphaText->setText(QString::number(brainSuiteWindow->imageState.labelAlpha));
}

void ImageDisplayPropertiesForm::on_imageVolumeFilename_fileDropped(const QString &filename)
{
  brainSuiteWindow->loadVolume(filename.toStdString());
}

void ImageDisplayPropertiesForm::on_imageOverlay1Filename_fileDropped(const QString &filename)
{
  brainSuiteWindow->loadOverlay1Volume(filename.toStdString());
}

void ImageDisplayPropertiesForm::on_imageOverlay2Filename_fileDropped(const QString &filename)
{
  brainSuiteWindow->loadOverlay2Volume(filename.toStdString());
}

void ImageDisplayPropertiesForm::on_labelVolumeFilename_fileDropped(const QString &filename)
{
  brainSuiteWindow->loadLabelVolume(filename.toStdString());
}

void ImageDisplayPropertiesForm::on_maskVolumeFilename_fileDropped(const QString &filename)
{
  brainSuiteWindow->loadMaskVolume(filename.toStdString());
}

void ImageDisplayPropertiesForm::on_imageVolumeMaxText_textEdited(const QString &upperValue)
{
  float f=upperValue.toFloat();
  ui->imageVolumeBrightnessSlider->setUpperValue(f);
  if (ui->imageVolumeBrightnessSlider->symmetric && f>0)
  {
    ui->imageVolumeBrightnessSlider->setLowerValue(-f);
    ui->imageVolumeBrightnessSlider->imageScale();
    ui->imageVolumeMinText->setText(QString::number((float)-f,'g',3));
  }
  brainSuiteWindow->setVolumeBrightness(ui->imageVolumeBrightnessSlider->imageScale());
  ui->imageVolumeBrightnessSlider->setRange(brainSuiteWindow->imageState.brightness);
}

void ImageDisplayPropertiesForm::on_imageOverlay1MaxText_textEdited(const QString &upperValue)
{
  float f=upperValue.toFloat();
  ui->imageOverlay1BrightnessSlider->setUpperValue(upperValue.toFloat());
  if (ui->imageOverlay1BrightnessSlider->symmetric && f>0)
  {
    ui->imageOverlay1BrightnessSlider->setLowerValue(-f);
    ui->imageOverlay1BrightnessSlider->imageScale();
    ui->imageOverlay1MinText->setText(QString::number((float)-f,'g',3));
  }
  brainSuiteWindow->setOverlay1Brightness(ui->imageOverlay1BrightnessSlider->imageScale());
  ui->imageOverlay1BrightnessSlider->setRange(brainSuiteWindow->imageState.overlay1Brightness);
}

void ImageDisplayPropertiesForm::on_imageOverlay2MaxText_textEdited(const QString &upperValue)
{
  float f=upperValue.toFloat();
  ui->imageOverlay2BrightnessSlider->setUpperValue(upperValue.toFloat());
  if (ui->imageOverlay2BrightnessSlider->symmetric && f>0)
  {
    ui->imageOverlay2BrightnessSlider->setLowerValue(-f);
    ui->imageOverlay2BrightnessSlider->imageScale();
    ui->imageOverlay2MinText->setText(QString::number((float)-f,'g',3));
  }
  brainSuiteWindow->setOverlay2Brightness(ui->imageOverlay2BrightnessSlider->imageScale());
  ui->imageOverlay2BrightnessSlider->setRange(brainSuiteWindow->imageState.overlay2Brightness);
}

void ImageDisplayPropertiesForm::on_imageVolumeBrightnessSlider_rangeChanged()
{
  brainSuiteWindow->setVolumeBrightness(ui->imageVolumeBrightnessSlider->imageScale());
  ui->imageVolumeMinText->setText(QString::number(brainSuiteWindow->imageState.brightness.minv));
  ui->imageVolumeMaxText->setText(QString::number(brainSuiteWindow->imageState.brightness.maxv));
}

void ImageDisplayPropertiesForm::on_imageOverlay1BrightnessSlider_rangeChanged()
{
  brainSuiteWindow->setOverlay1Brightness(ui->imageOverlay1BrightnessSlider->imageScale());
  ui->imageOverlay1MinText->setText(QString::number(brainSuiteWindow->imageState.overlay1Brightness.minv));
  ui->imageOverlay1MaxText->setText(QString::number(brainSuiteWindow->imageState.overlay1Brightness.maxv));
}

void ImageDisplayPropertiesForm::on_imageOverlay2BrightnessSlider_rangeChanged()
{
  brainSuiteWindow->setOverlay2Brightness(ui->imageOverlay2BrightnessSlider->imageScale());
  ui->imageOverlay2MinText->setText(QString::number(brainSuiteWindow->imageState.overlay2Brightness.minv));
  ui->imageOverlay2MaxText->setText(QString::number(brainSuiteWindow->imageState.overlay2Brightness.maxv));
}

void ImageDisplayPropertiesForm::on_imageVolumeBrightnessSlider_rightClick(const QPoint &point)
{
  brainSuiteWindow->makeLUTPopupMenu(point,brainSuiteWindow->imageLUT);
}

void ImageDisplayPropertiesForm::on_imageOverlay1BrightnessSlider_rightClick(const QPoint &point)
{
  brainSuiteWindow->makeLUTPopupMenu(point,brainSuiteWindow->imageOverlay1LUT);
}

void ImageDisplayPropertiesForm::on_imageOverlay2BrightnessSlider_rightClick(const QPoint &point)
{
  brainSuiteWindow->makeLUTPopupMenu(point,brainSuiteWindow->imageOverlay2LUT);
}

void ImageDisplayPropertiesForm::on_imageOverlay2Alpha_textEdited(const QString &arg1)
{
  int alpha = arg1.toInt();
  brainSuiteWindow->setLabelAlpha(alpha);
  ui->imageOverlay2Alpha->setText(QString::number(brainSuiteWindow->imageState.overlayAlpha));
  ui->imageOverlay2AlphaSlider->setTracking(false);
  ui->imageOverlay2AlphaSlider->setValue(alpha);
  ui->imageOverlay2AlphaSlider->setTracking(true);
}

void ImageDisplayPropertiesForm::on_imageOverlay1Alpha_textEdited(const QString &arg1)
{
  int alpha = arg1.toInt();
  brainSuiteWindow->setLabelAlpha(alpha);
  ui->imageOverlay1Alpha->setText(QString::number(brainSuiteWindow->imageState.overlayAlpha));
  ui->imageOverlay1AlphaSlider->setTracking(false);
  ui->imageOverlay1AlphaSlider->setValue(alpha);
  ui->imageOverlay1AlphaSlider->setTracking(true);
}

void ImageDisplayPropertiesForm::on_imageVolumeMinText_textEdited(const QString &lowerValue)
{
  float f=lowerValue.toFloat();
  ui->imageVolumeBrightnessSlider->setLowerValue(f);
  if (ui->imageVolumeBrightnessSlider->symmetric && f<0)
  {
    ui->imageVolumeBrightnessSlider->setUpperValue(-f);
    ui->imageVolumeMaxText->setText(QString::number(-f,'g',3));
  }
  brainSuiteWindow->setVolumeBrightness(ui->imageVolumeBrightnessSlider->imageScale());
  ui->imageVolumeBrightnessSlider->setRange(brainSuiteWindow->imageState.brightness);
}

void ImageDisplayPropertiesForm::on_imageOverlay1MinText_textEdited(const QString &lowerValue)
{
  ui->imageOverlay1BrightnessSlider->setLowerValue(lowerValue.toFloat());
  brainSuiteWindow->setOverlay1Brightness(ui->imageOverlay1BrightnessSlider->imageScale());
  ui->imageOverlay1BrightnessSlider->setRange(brainSuiteWindow->imageState.overlay1Brightness);
}

void ImageDisplayPropertiesForm::on_imageOverlay2MinText_textEdited(const QString &lowerValue)
{
  ui->imageOverlay2BrightnessSlider->setLowerValue(lowerValue.toFloat());
  brainSuiteWindow->setOverlay2Brightness(ui->imageOverlay2BrightnessSlider->imageScale());
  ui->imageOverlay2BrightnessSlider->setRange(brainSuiteWindow->imageState.overlay2Brightness);
}

void ImageDisplayPropertiesForm::on_labelAlphaText_textEdited(const QString &arg1)
{
  int alpha = arg1.toInt();
  brainSuiteWindow->setLabelAlpha(alpha);
  ui->labelAlphaText->setText(QString::number(brainSuiteWindow->imageState.labelAlpha));
  ui->labelAlphaSlider->setTracking(false);
  ui->labelAlphaSlider->setValue(alpha);
  ui->labelAlphaSlider->setTracking(true);
}

bool doubleCheck(QWidget *w)
{
  QMessageBox msgBox(w);
  msgBox.setText("Are you sure?");
  msgBox.setIcon(QMessageBox::Question);
  msgBox.setInformativeText("Do you really want to close this volume?");
  msgBox.addButton(QMessageBox::Yes);
  msgBox.addButton(QMessageBox::No);
  int value=msgBox.exec();
  return value==QMessageBox::Yes;
}

void ImageDisplayPropertiesForm::on_closeOverlay1PushButton_clicked()
{
  if (brainSuiteWindow)
  {
    if (brainSuiteWindow->brainSuiteDataManager.imageOverlay1Volume)
      if (doubleCheck(this))
        brainSuiteWindow->closeOverlay1Volume();
  }
}
void ImageDisplayPropertiesForm::on_closeOverlay2PushButton_clicked()
{
  if (brainSuiteWindow)
  {
    if (brainSuiteWindow->brainSuiteDataManager.imageOverlay2Volume)
      if (doubleCheck(this))
        brainSuiteWindow->closeOverlay2Volume();
  }
}

void ImageDisplayPropertiesForm::on_closeLabelVolumePushButton_clicked()
{
  if (brainSuiteWindow)
  {
    if (brainSuiteWindow->brainSuiteDataManager.vLabel)
      if (doubleCheck(this))
        brainSuiteWindow->closeLabelVolume();
  }
}

void ImageDisplayPropertiesForm::on_closeMaskVolumePushButton_clicked()
{
  if (brainSuiteWindow)
  {
    if (brainSuiteWindow->brainSuiteDataManager.vMask.size())
      if (doubleCheck(this))
        brainSuiteWindow->closeMaskVolume();
  }
}

void ImageDisplayPropertiesForm::on_showVolumeCheckBox_clicked()
{
  brainSuiteWindow->setShowVolume(ui->showVolumeCheckBox->isChecked());
}

void ImageDisplayPropertiesForm::on_showOverlay1VolumeCheckBox_clicked()
{
  brainSuiteWindow->setShowOverlay1(ui->showOverlay1VolumeCheckBox->isChecked());
}

void ImageDisplayPropertiesForm::on_showOverlay2VolumeCheckBox_clicked()
{
  if (brainSuiteWindow)
  {
    brainSuiteWindow->setShowOverlay2(ui->showOverlay2VolumeCheckBox->isChecked());
  }
}

void ImageDisplayPropertiesForm::on_showLabelVolumeCheckBox_clicked()
{
  if (brainSuiteWindow)
  {
    brainSuiteWindow->setShowLabels(ui->showLabelVolumeCheckBox->isChecked());
  }
}

void ImageDisplayPropertiesForm::on_lutSymmetricPushButton_clicked(bool checked)
{
  if (brainSuiteWindow)
  {
    imageSlider()->symmetric=checked;
    if (checked)
    {
      auto f=std::max(std::abs(brainSuiteWindow->imageState.brightness.minv),std::abs(brainSuiteWindow->imageState.brightness.maxv));
      brainSuiteWindow->imageState.brightness.maxv=f;
      brainSuiteWindow->imageState.brightness.minv=-f;
      brainSuiteWindow->setVolumeBrightness(brainSuiteWindow->imageState.brightness);
      updateDisplay();
      setImageVolumeSliderRange();
    }
  }
}

void ImageDisplayPropertiesForm::on_adjustRange1PushButton_clicked()
{
  setImageVolumeSliderRange();
}

void ImageDisplayPropertiesForm::on_lut2SymmetricPushButton_clicked(bool checked)
{
  if (brainSuiteWindow)
  {
    imageOverlay1Slider()->symmetric=checked;
    if (checked)
    {
      auto f=std::max(std::abs(brainSuiteWindow->imageState.overlay1Brightness.minv),std::abs(brainSuiteWindow->imageState.overlay1Brightness.maxv));
      brainSuiteWindow->imageState.overlay1Brightness.maxv=f;
      brainSuiteWindow->imageState.overlay1Brightness.minv=-f;
      brainSuiteWindow->setOverlay1Brightness(brainSuiteWindow->imageState.overlay1Brightness); // redundant
      updateDisplay();
      setImageOverlay1SliderRange();
    }
  }
}

void ImageDisplayPropertiesForm::on_lut3SymmetricPushButton_clicked(bool checked)
{
  if (brainSuiteWindow)
  {
    imageOverlay2Slider()->symmetric=checked;
    if (checked)
    {
      auto f=std::max(std::abs(brainSuiteWindow->imageState.overlay2Brightness.minv),std::abs(brainSuiteWindow->imageState.overlay2Brightness.maxv));
      brainSuiteWindow->imageState.overlay2Brightness.maxv=f;
      brainSuiteWindow->imageState.overlay2Brightness.minv=-f;
      brainSuiteWindow->setOverlay2Brightness(brainSuiteWindow->imageState.overlay2Brightness); // redundant
      updateDisplay();
      setImageOverlay2SliderRange();
    }
  }
}

void ImageDisplayPropertiesForm::on_adjustRange2PushButton_clicked()
{
  setImageOverlay1SliderRange();
}

void ImageDisplayPropertiesForm::on_adjustRange3PushButton_clicked()
{
  setImageOverlay2SliderRange();
}
