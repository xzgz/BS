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

#include "svregistrationdialog.h"
#include "ui_svregistrationdialog.h"
#include <qprocess.h>
#include <brainsuiteqtsettings.h>
#include <brainsuitewindow.h>
#include <iostream>
#include <qdir.h>
#include <waitcursor.h>
#include <qapplication.h>
#include <surfacelabelprocess.h>
#include <dualregistrationprocessdialog.h>
#include <registrationprocessdialog.h>
#include <qmessagebox.h>
#include <svregistrationtool.h>
#include <DS/timer.h>
#include <toolmanager.h>
#include <svregguitool.h>
#include <corticalextractionsettings.h>
#include <corticalextractionsequencedialog.h>
#include <svregguitool.h>
#include <corticalextractionsequencedialog.h>

extern SVRegGUITool svregGUITool;
SVRegGUITool svregGUITool;

void SVRegistrationDialog::updateSVRegPath()
{
  if (BrainSuiteQtSettings::qtsvregdir.empty()==false)
    ui->svregDirectoryLabel->setText(("SVReg directory: "+BrainSuiteQtSettings::qtsvregdir).c_str());
  else
    ui->svregDirectoryLabel->setText("SVReg directory is not specified");
}

SVRegistrationDialog::SVRegistrationDialog(BrainSuiteWindow *parent) :
  QDialog(parent),
  ui(new Ui::SVRegistrationDialog), brainSuiteWindow(parent)
{
  ui->setupUi(this);
  updateSVRegPath();
  ui->stepNameText->setText((std::string("Next Step: ") + svregGUITool.svreg.stateName(svregGUITool.svreg.state)).c_str());
  updateProgressBar();
  {
    ui->labeledAtlasComboBox->blockSignals(true);
    ui->labeledAtlasComboBox->clear();
    for (auto atlas=svregGUITool.svreg.svregManifest.atlases.begin(); atlas != svregGUITool.svreg.svregManifest.atlases.end(); atlas++)
    {
      ui->labeledAtlasComboBox->addItem(atlas->name.c_str());
    }
    ui->labeledAtlasComboBox->blockSignals(false);
  }
  updateDialogFromCES(); // need to update AFTER adding atlases so that selection matches.
  ui->previousStepButton->setVisible(false);
}

SVRegistrationDialog::~SVRegistrationDialog()
{
  delete ui;
}

void SVRegistrationDialog::updateDialogFromCES()
{
  ui->refineSulcalCurvesCheckBox->setChecked(brainSuiteWindow->ces.svreg.refineSulci);
  ui->surfaceRegistrationOnlyCheckBox->setChecked(brainSuiteWindow->ces.svreg.surfaceOnly);
  ui->labeledAtlasComboBox->setCurrentIndex(brainSuiteWindow->ces.svreg.atlasIndex);
  ui->extendLabelsCheckBox->setChecked(brainSuiteWindow->ces.svreg.extendLabels);
}

void SVRegistrationDialog::updateCESfromDialog()
{
  brainSuiteWindow->ces.svreg.surfaceOnly=ui->surfaceRegistrationOnlyCheckBox->isChecked();
  brainSuiteWindow->ces.svreg.refineSulci=ui->refineSulcalCurvesCheckBox->isChecked();
  brainSuiteWindow->ces.svreg.extendLabels=ui->extendLabelsCheckBox->isChecked();
  brainSuiteWindow->ces.svreg.atlasIndex=ui->labeledAtlasComboBox->currentIndex();
  brainSuiteWindow->ces.svreg.atlasName=ui->labeledAtlasComboBox->currentText().toStdString();
  if (brainSuiteWindow->toolManager.corticalExtractionSequenceDialog)
    brainSuiteWindow->toolManager.corticalExtractionSequenceDialog->updateSVRegParameters();
}

void SVRegistrationDialog::updateProgressBar()
{
  ui->progressBar->setRange(0,SVRegistrationTool::Finished);
  ui->progressBar->setValue(svregGUITool.svreg.state);
  ui->progressBar->repaint();
}

void SVRegistrationDialog::setProgress(const int value, const int maxValue)
{
  ui->progressBar->setRange(0,maxValue);
  ui->progressBar->setValue(value);
  ui->progressBar->repaint();

}

void SVRegistrationDialog::on_applyButton_clicked()
{
  while (step()) {  }
}

bool SVRegistrationDialog::isFinished()
{
  return (svregGUITool.svreg.state==SVRegistrationTool::Finished);
}

std::string SVRegistrationDialog::runningStepName()
{
  return svregGUITool.svreg.runningStepName(svregGUITool.svreg.state);
}

void SVRegistrationDialog::updateStepText(std::string stepname)
{
  ui->stepNameText->setText(stepname.c_str());
  ui->stepNameText->repaint();
}

bool SVRegistrationDialog::step()
{
  WaitCursor wc(brainSuiteWindow);
  WaitCursor wc2(this);
  std::string activeStep="running "+svregGUITool.stepName();
  updateStepText(activeStep);
  bool retcode=svregGUITool.runStep(brainSuiteWindow);
  updateStepText(svregGUITool.nextStepText());
  return retcode;
}

void SVRegistrationDialog::on_runStepButton_clicked()
{
  step();
}

void SVRegistrationDialog::on_surfaceRegistrationOnlyCheckBox_clicked()
{
  updateCESfromDialog();
}

void SVRegistrationDialog::on_refineSulcalCurvesCheckBox_clicked()
{
  updateCESfromDialog();
}

void SVRegistrationDialog::on_labeledAtlasComboBox_currentIndexChanged(int /*index*/)
{
  updateCESfromDialog();
}

void SVRegistrationDialog::on_extendLabelsCheckBox_clicked()
{
  updateCESfromDialog();
}
