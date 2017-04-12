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

#include "tissueclassificationdialog.h"
#include "ui_tissueclassificationdialog.h"
#include <brainsuitewindow.h>
#include <PVC/pvctool.h>
#include <displaycode.h>
#include <cerebrumextractiondialog.h>
#include <brainsuiteqtsettings.h>
#include <corticalextractionsettings.h>
#include <corticalextractionsequencedialog.h>
#include <pvcguitool.h>
#include <brainsuitewindow_t.h>

PVCGUITool pvcGUITool;

TissueClassificationDialog::TissueClassificationDialog(BrainSuiteWindow *parent) :
  QDialog(parent),
  ui(new Ui::TissueClassificationDialog), brainSuiteWindow(parent)
{
  ui->setupUi(this);
  ui->progressBar->setRange(0,pvcGUITool.nSteps());
  updateDialogFromCES();
  updateProgressBar();
}

TissueClassificationDialog::~TissueClassificationDialog()
{
  delete ui;
}

void TissueClassificationDialog::updateDialogFromCES()
{
  ui->spatialPriorWeightingEditBox->setText(QString::number(brainSuiteWindow->ces.pvc.spatialPrior));
}

void TissueClassificationDialog::updateCESfromDialog()
{
  brainSuiteWindow->ces.pvc.spatialPrior=ui->spatialPriorWeightingEditBox->text().toFloat();
  if (brainSuiteWindow && brainSuiteWindow->toolManager.corticalExtractionSequenceDialog)
    brainSuiteWindow->toolManager.corticalExtractionSequenceDialog->updatePVCParameters();
}

bool TissueClassificationDialog::step()
{
  return pvcGUITool.runStep(brainSuiteWindow);
}

std::string TissueClassificationDialog::runningStepName()
{
  return pvcGUITool.stepName();
}

bool TissueClassificationDialog::isFinished()
{
  return pvcGUITool.isFinished();
}

void TissueClassificationDialog::setProgress(const int value, const int maxValue)
{
  ui->progressBar->setRange(0,maxValue);
  ui->progressBar->setValue(value);
  ui->progressBar->repaint();
}

void TissueClassificationDialog::updateStepText(std::string s)
{
  ui->stepNameText->setText(s.c_str());
  ui->stepNameText->repaint();
}

void TissueClassificationDialog::updateProgressBar()
{
  updateStepText(pvcGUITool.stepName());
  ui->progressBar->setValue(pvcGUITool.currentStep());
  ui->progressBar->repaint();
}

void TissueClassificationDialog::on_applyButton_clicked()
{
  while (step());
}

void TissueClassificationDialog::on_runStepButton_clicked()
{
  step();
}

void TissueClassificationDialog::on_previousStepButton_clicked()
{
  pvcGUITool.reset(brainSuiteWindow);
}

void TissueClassificationDialog::on_nextTool_clicked()
{
  brainSuiteWindow->swapDialogs(this,brainSuiteWindow->toolManager.cerebrumExtractionDialog);
}

void TissueClassificationDialog::on_spatialPriorWeightingEditBox_textEdited(const QString & /*arg1*/)
{
  updateCESfromDialog();
}

void TissueClassificationDialog::on_restoreFromPreviousSessionButton_clicked()
{
  if (!brainSuiteWindow) return; // this should never happen
  if (brainSuiteWindow->loadExtractionStudy(CorticalExtractionData::BFC))
  {
    brainSuiteWindow->brainSuiteDataManager.vMask.copy(brainSuiteWindow->corticalExtractionData.vBrainMask);
    brainSuiteWindow->updateView(DisplayCode::MaskOn|DisplayCode::Dirty);
  }
}
