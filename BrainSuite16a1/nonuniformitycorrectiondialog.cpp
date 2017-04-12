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

#include "nonuniformitycorrectiondialog.h"
#include "ui_nonuniformitycorrectiondialog.h"
#include <brainsuitewindow.h>
#include <brainsuitedatamanager.h>
#include <toolmanager.h>
#include <tissueclassificationdialog.h>
#include <waitcursor.h>
#include <DS/timer.h>
#include <brainsuitesettings.h>
#include <corticalextractionsettings.h>
#include <corticalextractionsequencedialog.h>
#include <bfcguitool.h>
#include <displaycode.h>
#include <brainsuitewindow_t.h>

BFCGUITool bfcGUITool;

NonuniformityCorrectionDialog::NonuniformityCorrectionDialog(BrainSuiteWindow *parent) :
  QDialog(parent),
  ui(new Ui::NonuniformityCorrectionDialog), brainSuiteWindow(parent)
{
  ui->setupUi(this);
  setProgress(0,bfcGUITool.nSteps());
  ui->lowRangeButton->setVisible(false);
  ui->mediumRangeButton->setVisible(false);
  ui->highRangeButton->setVisible(false);
  ui->previousStepButton->setVisible(false);
  updateDialogFromCES();
}

NonuniformityCorrectionDialog::~NonuniformityCorrectionDialog()
{
  delete ui;
}

void NonuniformityCorrectionDialog::setProgress(const int value, const int maxValue)
{
  ui->progressBar->setRange(0,maxValue);
  ui->progressBar->setValue(value);
  ui->progressBar->repaint();
}

void NonuniformityCorrectionDialog::updateDialogFromCES()
{
  ui->iterativeModeCheckBox->setChecked(brainSuiteWindow->ces.bfc.iterative);
  ui->histogramRadiusEditBox->setText(QString::number(brainSuiteWindow->ces.bfc.histogramRadius));
  ui->sampleSpacingEditBox->setText(QString::number(brainSuiteWindow->ces.bfc.biasEstimateSampleSpacing));
  ui->controlPointSpacingEditBox->setText(QString::number(brainSuiteWindow->ces.bfc.controlPointSpacing));
  ui->splineStiffnessEditBox->setText(QString::number(brainSuiteWindow->ces.bfc.splineStiffness));
  ui->cuboidRadioButton->setChecked(brainSuiteWindow->ces.bfc.roiType==CorticalExtractionSettings::BFC::Block);
  ui->ellipsoidRadioButton->setChecked(brainSuiteWindow->ces.bfc.roiType==CorticalExtractionSettings::BFC::Ellipsoid);
  ui->biasRangeLowerEditBox->setText(QString::number(brainSuiteWindow->ces.bfc.biasRange.first));
  ui->biasRangeUpperEditBox->setText(QString::number(brainSuiteWindow->ces.bfc.biasRange.second));
  updateWidgetState();
}

void NonuniformityCorrectionDialog::updateCESfromDialog()
{
  brainSuiteWindow->ces.bfc.histogramRadius=ui->histogramRadiusEditBox->text().toInt();
  brainSuiteWindow->ces.bfc.biasEstimateSampleSpacing=ui->sampleSpacingEditBox->text().toInt();
  brainSuiteWindow->ces.bfc.controlPointSpacing=ui->controlPointSpacingEditBox->text().toInt();
  brainSuiteWindow->ces.bfc.splineStiffness=ui->splineStiffnessEditBox->text().toFloat();
  brainSuiteWindow->ces.bfc.roiType=(ui->cuboidRadioButton->isChecked()) ? CorticalExtractionSettings::BFC::Block : CorticalExtractionSettings::BFC::Ellipsoid;
  brainSuiteWindow->ces.bfc.biasRange.first=ui->biasRangeLowerEditBox->text().toFloat();
  brainSuiteWindow->ces.bfc.biasRange.second=ui->biasRangeUpperEditBox->text().toFloat();
  brainSuiteWindow->ces.bfc.iterative = ui->iterativeModeCheckBox->isChecked();
  if (brainSuiteWindow->toolManager.corticalExtractionSequenceDialog)
    brainSuiteWindow->toolManager.corticalExtractionSequenceDialog->updateBFCParameters();
}

void NonuniformityCorrectionDialog::on_applyButton_clicked()
{
  while (step()) {}
}

void NonuniformityCorrectionDialog::on_runStepButton_clicked()
{
  step();
}

std::string NonuniformityCorrectionDialog::runningStepName()
{
  return bfcGUITool.stepName();
}

bool NonuniformityCorrectionDialog::isFinished()
{
  return (bfcGUITool.isFinished());
}

void NonuniformityCorrectionDialog::updateStepText(std::string stepNameText)
{
  ui->stepNameText->setText(stepNameText.c_str());
  ui->stepNameText->repaint();
}

void NonuniformityCorrectionDialog::updateProgressBar()
{
  ui->progressBar->setValue(bfcGUITool.bfcTool.state);
  ui->progressBar->repaint();
}

bool NonuniformityCorrectionDialog::step()
{
  Timer timer;
  timer.start();
  if (!brainSuiteWindow->brainSuiteDataManager.volume) return false;
  bool retcode=false;
  WaitCursor waitCursor(this);
  retcode=bfcGUITool.runStep(brainSuiteWindow,this);
  timer.stop();
  return retcode;
}

void NonuniformityCorrectionDialog::on_previousStepButton_clicked()
{
}

void NonuniformityCorrectionDialog::on_nextTool_clicked()
{
  brainSuiteWindow->swapDialogs(this,brainSuiteWindow->toolManager.tissueClassificationDialog);
}

void NonuniformityCorrectionDialog::on_lowRangeButton_clicked()
{
  ui->biasRangeLowerEditBox->setText("0.95");
  ui->biasRangeUpperEditBox->setText("1.05");
}

void NonuniformityCorrectionDialog::on_mediumRangeButton_clicked()
{
  ui->biasRangeLowerEditBox->setText("0.90");
  ui->biasRangeUpperEditBox->setText("1.10");
}

void NonuniformityCorrectionDialog::on_highRangeButton_clicked()
{
  ui->biasRangeLowerEditBox->setText("0.80");
  ui->biasRangeUpperEditBox->setText("1.20");
}

void NonuniformityCorrectionDialog::updateAutosaveInfo(bool valid)
{
  if (valid && BrainSuiteSettings::autosaveCorticalExtraction && !BrainSuiteSettings::autosavePrefix.empty())
  {
    std::string output = BrainSuiteSettings::autosavePrefix + ".bfc.nii.gz";
  }
  else
  {
  }
}

void NonuniformityCorrectionDialog::on_iterativeModeCheckBox_clicked()
{    
  updateCESfromDialog();
  updateWidgetState();
}

void NonuniformityCorrectionDialog::updateWidgetState()
{
  bool parametersEnabled=brainSuiteWindow->ces.bfc.iterative==false;
  ui->histogramRadiusEditBox->setEnabled(parametersEnabled);
  ui->sampleSpacingEditBox->setEnabled(parametersEnabled);
  ui->controlPointSpacingEditBox->setEnabled(parametersEnabled);
  ui->splineStiffnessEditBox->setEnabled(parametersEnabled);
  ui->ellipsoidRadioButton->setEnabled(parametersEnabled);
  ui->cuboidRadioButton->setEnabled(parametersEnabled);
  ui->biasRangeLowerEditBox->setEnabled(parametersEnabled);
  ui->biasRangeUpperEditBox->setEnabled(parametersEnabled);
  ui->histogramRadiusLabel->setEnabled(parametersEnabled);
  ui->sampleSpacingLabel->setEnabled(parametersEnabled);
  ui->controlPointSpacingLabel->setEnabled(parametersEnabled);
  ui->splineStiffnessLabel->setEnabled(parametersEnabled);
  ui->biasEstimateRangeLabel->setEnabled(parametersEnabled);
  ui->roiShapeLabel->setEnabled(parametersEnabled);
}

void NonuniformityCorrectionDialog::on_histogramRadiusEditBox_textEdited(const QString & /* arg1 */)
{
  updateCESfromDialog();
}

void NonuniformityCorrectionDialog::on_sampleSpacingEditBox_textEdited(const QString & /* arg1 */)
{
  updateCESfromDialog();
}

void NonuniformityCorrectionDialog::on_controlPointSpacingEditBox_textEdited(const QString & /* arg1 */)
{
  updateCESfromDialog();
}

void NonuniformityCorrectionDialog::on_splineStiffnessEditBox_textEdited(const QString & /* arg1 */)
{
  updateCESfromDialog();
}

void NonuniformityCorrectionDialog::on_cuboidRadioButton_clicked()
{
  updateCESfromDialog();
}

void NonuniformityCorrectionDialog::on_ellipsoidRadioButton_clicked()
{
  updateCESfromDialog();
}

void NonuniformityCorrectionDialog::on_biasRangeLowerEditBox_textEdited(const QString & /* arg1 */)
{
  updateCESfromDialog();
}

void NonuniformityCorrectionDialog::on_biasRangeUpperEditBox_textEdited(const QString & /* arg1 */)
{
  updateCESfromDialog();
}

void NonuniformityCorrectionDialog::on_restoreFromPreviousSessionButton_clicked()
{
  if (!brainSuiteWindow) return; // this should never happen
  if (brainSuiteWindow->loadExtractionStudy(CorticalExtractionData::BSE))
  {
    brainSuiteWindow->brainSuiteDataManager.vMask.copy(brainSuiteWindow->corticalExtractionData.vBrainMask);
    brainSuiteWindow->updateView(DisplayCode::MaskOn|DisplayCode::Dirty);
  }
}
