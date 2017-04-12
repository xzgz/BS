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

#include "skullandscalpdialog.h"
#include "ui_skullandscalpdialog.h"
#include <skullfinderguitool.h>
#include <brainsuitewindow.h>
#include <waitcursor.h>
#include <displaycode.h>
#include <toolmanager.h>
#include <brainsuiteqtsettings.h>
#include <qdir.h>
#include <toolmanager.h>
#include <corticalextractionsequencedialog.h>
#include <corticalextractionsettings.h>
#include <nonuniformitycorrectiondialog.h>
#include <brainsuitewindow_t.h>

extern SkullfinderGUITool skullfinderGUITool;

SkullAndScalpDialog::SkullAndScalpDialog(BrainSuiteWindow *brainSuiteWindow) :
  QDialog(brainSuiteWindow),
  ui(new Ui::SkullAndScalpDialog),
  brainSuiteWindow(brainSuiteWindow)
{
  ui->setupUi(this);
  updateDialogFromCES();
  updateProgress();
}

SkullAndScalpDialog::~SkullAndScalpDialog()
{
  delete ui;
}

void SkullAndScalpDialog::setProgress(const int value, const int maxValue)
{
  ui->progressBar->setRange(0,maxValue);
  ui->progressBar->setValue(value);
}

void SkullAndScalpDialog::updateStepText(std::string stepname)
{
  ui->stepNameText->setText(stepname.c_str());
  ui->stepNameText->repaint();
}

void SkullAndScalpDialog::updateProgress()
{
  ui->progressBar->setRange(SkullAndScalpTool::Initialize,SkullAndScalpTool::Finished);
  ui->progressBar->setValue(skullfinderGUITool.currentStep());
  ui->progressBar->repaint();
  if (!skullfinderGUITool.isFinished())
    ui->stepNameText->setText((std::string("next step: ")+skullfinderGUITool.stepName()).c_str());
  else
    ui->stepNameText->setText(skullfinderGUITool.stepName().c_str());
}

void SkullAndScalpDialog::updateDialogFromCES()
{
  ui->skullComputeThresholdsCheckBox->setChecked(brainSuiteWindow->ces.skullfinder.computeThresholds);
  ui->skullThresholdEditBox->setText(QString::number(brainSuiteWindow->ces.skullfinder.skullThreshold));
  ui->scalpThresholdEditBox->setText(QString::number(brainSuiteWindow->ces.skullfinder.scalpThreshold));
}

void SkullAndScalpDialog::updateCESfromDialog()
{
  brainSuiteWindow->ces.skullfinder.computeThresholds = ui->skullComputeThresholdsCheckBox->isChecked();
  brainSuiteWindow->ces.skullfinder.skullThreshold = ui->skullThresholdEditBox->text().toInt();
  brainSuiteWindow->ces.skullfinder.scalpThreshold = ui->scalpThresholdEditBox->text().toInt();
}

void SkullAndScalpDialog::on_skullComputeThresholdsCheckBox_clicked()
{
  updateCESfromDialog();
}

void SkullAndScalpDialog::on_skullThresholdEditBox_textEdited(const QString &/* arg1 */)
{
  updateCESfromDialog();
}

void SkullAndScalpDialog::on_scalpThresholdEditBox_textEdited(const QString &/* arg1 */)
{
  updateCESfromDialog();
}

void SkullAndScalpDialog::on_applyButton_clicked()
{
  while (skullfinderGUITool.runStep(brainSuiteWindow));
}

void SkullAndScalpDialog::on_runStepButton_clicked()
{
  skullfinderGUITool.runStep(brainSuiteWindow);
}

void SkullAndScalpDialog::on_bfcButton_clicked()
{
  brainSuiteWindow->swapDialogs(this,brainSuiteWindow->toolManager.nonuniformityCorrectionDialog);
}

void SkullAndScalpDialog::on_previousStepButton_clicked()
{
  skullfinderGUITool.stepBack(brainSuiteWindow);
}

void SkullAndScalpDialog::on_restoreFromPreviousSessionButton_clicked()
{
  if (!brainSuiteWindow) return; // this should never happen
  if (brainSuiteWindow->loadExtractionStudy(CorticalExtractionData::BSE))
  {
    brainSuiteWindow->brainSuiteDataManager.vMask.copy(brainSuiteWindow->corticalExtractionData.vBrainMask);
    brainSuiteWindow->updateView(DisplayCode::MaskOn|DisplayCode::Dirty);
  }
}
