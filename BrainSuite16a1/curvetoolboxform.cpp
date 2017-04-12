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

#include <qdebug.h>
#include <qdir.h>
#include <qdesktopservices.h>
#include <qurl.h>
#include "curvetoolboxform.h"
#include "ui_curvetoolboxform.h"
#include "brainsuiteqtsettings.h"
#include <brainsuitesettings.h>
#include <brainsuitewindow.h>
#include <protocolcurveset.h>
#include <qcolordialog.h>
#include <displaycode.h>
#include <curvetoolboxform.h>
#include <protocolcurveset.h>
#include <glglyphs.h>
#include <surfacealpha.h>
#include <DS/timer.h>
#include <floatslider.h>

const float CurveToolboxForm::curveTubeSizeScale=1000.0f;

CurveToolboxForm::CurveToolboxForm(BrainSuiteWindow *brainSuiteWindow, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::CurveToolboxForm), brainSuiteWindow(brainSuiteWindow)
{
  ui->setupUi(this);
  ui->saveCurveSetButton->setText("");
  ui->saveCurveSetButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogSaveButton));
  ui->loadCurvesetPushButton->setText("");
  ui->loadCurvesetPushButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogOpenButton));
  {
    std::ostringstream ostr;
    ostr<<BrainSuiteSettings::curveProtocolBrushSize;
    ui->imageCurveWidthLineEdit->setText(ostr.str().c_str());
  }
  initialize();
}

CurveToolboxForm::~CurveToolboxForm()
{
  delete ui;
}

void CurveToolboxForm::initialize()
{
  ui->showProtocolCheckBox->setChecked(true);
  ui->showCurvesCheckBox->setChecked(BrainSuiteSettings::showCurves);
  ui->renderAsTubesCheckBox->setChecked(BrainSuiteSettings::showCurvesAsTubes);
  ui->tubeRadiusSlider->setTracking(false);
  float n=BrainSuiteSettings::curveTubeSize;
  ui->tubeRadiusSlider->setRange(100,3000);
  BrainSuiteSettings::curveTubeSize=n;
  ui->tubeRadiusSlider->setTracking(true);
  ui->tubeRadiusLineEdit->setText(QString::number(BrainSuiteSettings::curveTubeSize));
  ui->showCurvesOnImageSlicesCheckBox->setChecked(BrainSuiteSettings::showCurvesOnImageSlices);
  ui->lineWidthLineEdit->setText(QString::number(BrainSuiteSettings::defaultCurveWidth));
  ui->showCurvePointsCheckBox->setChecked(false);
  ui->pointListWidget->setVisible(false);
  updateCurveList();
}

void CurveToolboxForm::on_renderAsTubesCheckBox_clicked()
{
  bool flag = ui->renderAsTubesCheckBox->isChecked();
  BrainSuiteSettings::showCurvesAsTubes = flag;
  if (brainSuiteWindow)
    brainSuiteWindow->updateSurfaceView();
}

void CurveToolboxForm::on_tubeRadiusLineEdit_textEdited(const QString &arg1)
{
  BrainSuiteSettings::curveTubeSize = arg1.toFloat();
  ui->tubeRadiusSlider->setTracking(false);
  ui->tubeRadiusSlider->setValue(BrainSuiteSettings::curveTubeSize*curveTubeSizeScale);
  ui->tubeRadiusSlider->setTracking(true);
  if (brainSuiteWindow)
    brainSuiteWindow->updateSurfaceView();
}

void CurveToolboxForm::on_tubeRadiusSlider_valueChanged(int value)
{
  BrainSuiteSettings::curveTubeSize = value/curveTubeSizeScale;
  ui->tubeRadiusLineEdit->setText(QString::number(BrainSuiteSettings::curveTubeSize));
  if (brainSuiteWindow)
    brainSuiteWindow->updateSurfaceView();
}

void CurveToolboxForm::on_showCurvesCheckBox_clicked()
{
  BrainSuiteSettings::showCurves = ui->showCurvesCheckBox->isChecked();
  if (brainSuiteWindow)
    brainSuiteWindow->updateSurfaceView();
}

void CurveToolboxForm::updateCurveList()
{
  if (brainSuiteWindow && brainSuiteWindow->brainSuiteDataManager.protocolCurveSet)
  {
    ui->curveListWidget->clear();
    ProtocolCurveSet &curveset = *brainSuiteWindow->brainSuiteDataManager.protocolCurveSet;
    ui->protocolNameTextLabel->setText(curveset.name.c_str());
    for (size_t i=0;i<curveset.curves.size(); i++)
    {
      ProtocolCurve &curve=curveset.curves[i];
      if (curve.attributes.name.empty())
      {
        std::ostringstream ostr;
        ostr<<"Curve "<<i+1;
        curve.attributes.name = ostr.str();
      }
      QListWidgetItem *item = new QListWidgetItem(curve.attributes.name.c_str(),ui->curveListWidget);
      item->setData(Qt::UserRole,QVariant((int)i));
    }
  }
}

void CurveToolboxForm::updatePointList()
{
  ui->pointListWidget->clear();
  if (ui->showCurvePointsCheckBox->isChecked())
  {
    QListWidgetItem *current=ui->curveListWidget->currentItem();
    if (current && brainSuiteWindow && brainSuiteWindow->brainSuiteDataManager.protocolCurveSet)
    {
      int idx=current->data(Qt::UserRole).toInt();
      ProtocolCurveSet &curveset = *brainSuiteWindow->brainSuiteDataManager.protocolCurveSet;
      if (idx>=0 && idx < (int)curveset.curves.size())
      {
        ProtocolCurve &curve=curveset.curves[idx];
        for (size_t i=0;i<curve.points.size();i++)
        {
          std::ostringstream ostr;
          ostr<<curve.points[i];
          ui->pointListWidget->insertItem((int)i,ostr.str().c_str());
        }
      }
    }
  }
  else
  {
    ui->pointListWidget->setVisible(false);
  }
}


void CurveToolboxForm::updateProtocol()
{
  ui->protocolNameTextLabel->setText(brainSuiteWindow->brainSuiteDataManager.protocolCurveSet->name.c_str());
  if (ui->showProtocolCheckBox->isChecked())
  {
    ProtocolCurve *curve=currentCurve();
    if (curve)
    {
      ui->curveIDLineEdit->setEnabled(true);
      ui->curveIDLineEdit->setText(curve->protocol.tag.c_str());
      ui->curveNameLineEdit->setEnabled(true);
      ui->curveNameLineEdit->setText(curve->protocol.name.c_str());
      std::ostringstream ostr;
      ostr<<"<b>start:</b> "<<curve->protocol.start<<"<br/>";
      ostr<<"<b>stop:</b> "<<curve->protocol.stop<<"<br/>";
      ostr<<"<b>direction:</b> "<<curve->protocol.direction<<"<br/>";
      ostr<<"<b>notes:</b> "<<curve->protocol.notes<<"<br/>";
      ostr<<"<b>required:</b> "<<(curve->protocol.required ? "yes" : "no")<<"<br/>";
      ui->protocolInfoTextEdit->setText(ostr.str().c_str());
      ui->protocolInfoTextEdit->setEnabled(curve->protocol.href.empty()==false);
    }
    else
    {
      ui->protocolInfoTextEdit->setEnabled(false);
      ui->curveIDLineEdit->setText("");
      ui->curveIDLineEdit->setEnabled(false);
      ui->curveNameLineEdit->setText("");
      ui->curveNameLineEdit->setEnabled(false);
    }
  }
  else
  {
    ui->protocolInfoTextEdit->setVisible(false);
  }
}

void CurveToolboxForm::updateCurveInfo()
{
  if (!brainSuiteWindow) return;
  ProtocolCurveSet *curveset = brainSuiteWindow->brainSuiteDataManager.protocolCurveSet;
  std::ostringstream info;
  if (curveset)
  {
    info<<"Curveset has "<<curveset->curves.size()<<" curves\n";
    ProtocolCurve *curve=currentCurve();
    if (curve)
    {
      info<<"Curve has "<<curve->points.size()<<" points";
      updatePointList();
    }
  }
  ui->curveInfoTextEdit->setText(info.str().c_str());
}

void CurveToolboxForm::on_curveListWidget_currentItemChanged(QListWidgetItem *current, QListWidgetItem * /*previous*/)
{
  if (current && brainSuiteWindow && brainSuiteWindow->brainSuiteDataManager.protocolCurveSet)
  {
    int idx=current->data(Qt::UserRole).toInt();
    ProtocolCurveSet &curveset = *brainSuiteWindow->brainSuiteDataManager.protocolCurveSet;
    if (idx>=0 && idx < (int)curveset.curves.size())
    {
      ProtocolCurve &curve=curveset.curves[idx];
      ui->curveColorSelectionButton->setColor(QColor::fromRgbF(curve.attributes.color.x,curve.attributes.color.y,curve.attributes.color.z));
      updateCurveInfo();
      updatePointList();
      updateProtocol();
      brainSuiteWindow->updateView(DisplayCode::RedrawView);
    }
  }
}

int CurveToolboxForm::currentCurveIndex() // returns -1 if none selected
{
  QListWidgetItem *currentItem=ui->curveListWidget->currentItem();
  if (currentItem)
  {
    int idx=currentItem->data(Qt::UserRole).toInt();
    ProtocolCurveSet &curveset = *brainSuiteWindow->brainSuiteDataManager.protocolCurveSet;
    if (idx>=0 && idx < (int)curveset.curves.size())
      return idx;
  }
  return -1;
}

ProtocolCurve *CurveToolboxForm::currentCurve()
{
  QListWidgetItem *currentItem=ui->curveListWidget->currentItem();
  if (currentItem)
  {
    int idx=currentItem->data(Qt::UserRole).toInt();
    ProtocolCurveSet &curveset = *brainSuiteWindow->brainSuiteDataManager.protocolCurveSet;
    if (idx>=0 && idx < (int)curveset.curves.size())
    {
      ProtocolCurve &curve=curveset.curves[idx];
      return &curve;
    }
  }
  return 0;
}

void CurveToolboxForm::on_curveColorSelectionButton_clicked()
{
  QListWidgetItem *currentItem=ui->curveListWidget->currentItem();
  if (currentItem)
  {
    int idx=currentItem->data(Qt::UserRole).toInt();
    ProtocolCurveSet &curveset = *brainSuiteWindow->brainSuiteDataManager.protocolCurveSet;
    if (idx>=0 && idx < (int)curveset.curves.size())
    {
      ProtocolCurve &curve=curveset.curves[idx];
      QColor c=QColorDialog::getColor(ui->curveColorSelectionButton->getColor(),this,"New Surface Color");
      if (!c.isValid()) return;
      ui->curveColorSelectionButton->setColor(c);
      curve.attributes.color = DSPoint(c.redF(),c.greenF(),c.blueF());
      if (brainSuiteWindow)
        brainSuiteWindow->updateSurfaceView();
    }
  }
}

void CurveToolboxForm::on_showCurvesOnImageSlicesCheckBox_clicked()
{
  BrainSuiteSettings::showCurvesOnImageSlices = ui->showCurvesOnImageSlicesCheckBox->isChecked();
  if (brainSuiteWindow)
  {
    brainSuiteWindow->updateView(DisplayCode::Dirty);
  }
}

void CurveToolboxForm::on_lineWidthLineEdit_textEdited(const QString &arg1)
{
  BrainSuiteSettings::defaultCurveWidth=arg1.toFloat();
  if (brainSuiteWindow)
  {
    brainSuiteWindow->updateView(DisplayCode::Dirty);
  }
}

inline bool operator==(const DSPoint &a, const DSPoint &b)
{
  return (a.x==b.x)&&(a.y==b.y)&&(a.z==b.z);
}

bool CurveToolboxForm::addCurve()
{
  return false;
}

void CurveToolboxForm::on_showCurvePointsCheckBox_clicked()
{
  if (ui->showCurvePointsCheckBox->isChecked())
  {
    ui->pointListWidget->show();
    updatePointList();
  }
  else
    ui->pointListWidget->hide();
}

void CurveToolboxForm::on_loadProtocolPushButton_clicked()
{
  if (brainSuiteWindow)
    brainSuiteWindow->loadCurveProtocol();
}

void CurveToolboxForm::on_loadCurvesetPushButton_clicked()
{
  if (brainSuiteWindow)
    brainSuiteWindow->loadCurveSet();
}

void CurveToolboxForm::on_saveCurveSetButton_clicked()
{
  if (brainSuiteWindow)
    brainSuiteWindow->saveCurveSet();
}

void CurveToolboxForm::on_showProtocolCheckBox_clicked()
{
  if (ui->showProtocolCheckBox->isChecked())
  {
    ui->protocolInfoTextEdit->show();
  }
  else
    ui->protocolInfoTextEdit->hide();
  updateProtocol();
}

void CurveToolboxForm::on_protocolLinkPushButton_clicked()
{

  ProtocolCurve *curve=currentCurve();
  if (curve && !curve->protocol.href.empty())
  {
    std::cout<<"launching "<<curve->protocol.href<<std::endl;
    QDesktopServices::openUrl(QUrl(curve->protocol.href.c_str()));
  }
}

void CurveToolboxForm::on_protocolNameTextLabel_fileDropped(const QString &filename)
{
  if (brainSuiteWindow)
    brainSuiteWindow->loadCurveProtocol(filename.toStdString());
}

void CurveToolboxForm::on_imageCurveWidthLineEdit_textEdited(const QString &arg1)
{
  BrainSuiteSettings::curveProtocolBrushSize=arg1.toFloat();
  if (brainSuiteWindow)
  {
    brainSuiteWindow->updateView(DisplayCode::Dirty);
  }
}
