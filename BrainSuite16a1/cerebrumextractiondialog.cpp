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

#include "cerebrumextractiondialog.h"
#include "ui_cerebrumextractiondialog.h"
#include <qdir.h>
#include <qfiledialog.h>
#include <qfileinfo.h>
#include <brainsuitewindow.h>
#include <toolmanager.h>
#include <cortexidentificationdialog.h>
#include <cerebrumlabelerqt.h>
#include <waitcursor.h>
#include <brainsuitewindow.h>
#include <brainsuitesettings.h>
#include <brainsuiteqtsettings.h>
#include <displaycode.h>
#include <DS/timer.h>
#include <corticalextractionsettings.h>
#include <corticalextractionsequencedialog.h>
#include <quietset.h>
#include <cerebroguitool.h>
#include <brainsuitewindow_t.h>

CerebroGUITool cerebroGUITool;

std::string cerebroRunningStepName(CerebrumLabelerQt::State state)
{
  switch(state)
  {
    case CerebrumLabelerQt::Initialize  : return "Initializing";
    case CerebrumLabelerQt::AlignLinear : return "Performing linear alignment";
    case CerebrumLabelerQt::AlignWarp  : return "Performing nonlinear alignment";
    case CerebrumLabelerQt::ResliceWarp    : return "Reslicing atlas";
    case CerebrumLabelerQt::Finished   : return "Finished labeling lobes";
    default: return "<error in step name>";
  }
}

std::string cerebroStepName(CerebrumLabelerQt::State state)
{
  switch(state)
  {
    case CerebrumLabelerQt::Initialize           : return "Next step: initialization";
    case CerebrumLabelerQt::AlignLinear : return "Next step: linear alignment";
    case CerebrumLabelerQt::AlignWarp  : return "Next step: linear alignment";
    case CerebrumLabelerQt::ResliceWarp    : return "Next step: warp alignment";
    case CerebrumLabelerQt::Finished   : return "Finished labeling lobes";
    default: return "<error in step name>";
  }
}

void CerebrumExtractionDialog::updateStepName()
{
  updateStepName(cerebroStepName(cerebroGUITool.cerebrumLabeler.state));
  ui->progressBar->setValue(cerebroGUITool.cerebrumLabeler.state);
  ui->progressBar->repaint();
}

void CerebrumExtractionDialog::updateRunningStepName()
{
  updateStepName(cerebroRunningStepName(cerebroGUITool.cerebrumLabeler.state));
  ui->progressBar->setValue(cerebroGUITool.cerebrumLabeler.state);
  ui->progressBar->repaint();
}

CerebrumExtractionDialog::CerebrumExtractionDialog(BrainSuiteWindow *parent) :
  QDialog(parent),
  ui(new Ui::CerebrumExtractionDialog), brainSuiteWindow(parent)
{
  cerebroGUITool.initialize(brainSuiteWindow);
  ui->setupUi(this);
  ui->tempPathPushButton->setVisible(false);
  ui->tempPathTextLabel->setVisible(false);
  ui->previousStepButton->setVisible(false);
  ui->costFunctionComboBox->blockSignals(true);
  ui->warpLevelSpinBox->blockSignals(true);

  ui->progressBar->setRange(0,CerebrumLabelerQt::Finished);

  ui->costFunctionComboBox->insertItem(0,"standard deviation");
  ui->costFunctionComboBox->insertItem(1,"least squares");
  ui->costFunctionComboBox->insertItem(2,"least squares w/intensity rescaling");
  ui->warpLevelSpinBox->setRange(2,8);
  ui->tempPathTextLabel->setText(cerebroGUITool.cerebrumLabeler.tempDirectoryBase.c_str());

  updateDialogFromCES();
  updateStepName();

  ui->costFunctionComboBox->blockSignals(false);
  ui->warpLevelSpinBox->blockSignals(false);
}

CerebrumExtractionDialog::~CerebrumExtractionDialog()
{
  delete ui;
}

void CerebrumExtractionDialog::setProgress(const int value, const int maxValue)
{
  ui->progressBar->setRange(0,maxValue);
  ui->progressBar->setValue(value);
  ui->progressBar->repaint();
}

void CerebrumExtractionDialog::updateDialogFromCES()
{
  ui->useCentroidsCheckBox->setChecked(brainSuiteWindow->ces.cbm.useCentroids);
  ui->linearConvergenceEditBox->setText(QString::number(brainSuiteWindow->ces.cbm.linearConvergence));
  ui->warpConvergenceEditBox->setText(QString::number(brainSuiteWindow->ces.cbm.warpConvergence));
  ::setValueQuiet(ui->warpLevelSpinBox,brainSuiteWindow->ces.cbm.warpLevel);
  ui->costFunctionComboBox->setCurrentIndex(brainSuiteWindow->ces.cbm.costFunction);
  ui->verboseCheckBox->setChecked(brainSuiteWindow->ces.cbm.verbose);
  ui->showAlignedAtlasAsOverlayCheckBox->setChecked(brainSuiteWindow->ces.cbm.showAtlasOverlay);
  ui->atlasFileTextLabel->setText(brainSuiteWindow->ces.cbm.atlasFilename.c_str());
  ui->labelFileTextLabel->setText(brainSuiteWindow->ces.cbm.labelFilename.c_str());
  ui->tempPathTextLabel->setText(brainSuiteWindow->ces.cbm.tempDirectory.c_str());
}

void CerebrumExtractionDialog::updateCESfromDialog()
{
  brainSuiteWindow->ces.cbm.useCentroids=ui->useCentroidsCheckBox->isChecked();
  brainSuiteWindow->ces.cbm.linearConvergence=ui->linearConvergenceEditBox->text().toFloat();
  brainSuiteWindow->ces.cbm.warpConvergence=ui->warpConvergenceEditBox->text().toFloat();
  brainSuiteWindow->ces.cbm.warpLevel=ui->warpLevelSpinBox->value();
  brainSuiteWindow->ces.cbm.costFunction=ui->costFunctionComboBox->currentIndex();
  brainSuiteWindow->ces.cbm.verbose=ui->verboseCheckBox->isChecked();
  brainSuiteWindow->ces.cbm.showAtlasOverlay=ui->showAlignedAtlasAsOverlayCheckBox->isChecked();

  if (brainSuiteWindow->toolManager.corticalExtractionSequenceDialog)
    brainSuiteWindow->toolManager.corticalExtractionSequenceDialog->updateCerebroParameters();
}

std::string CerebrumExtractionDialog::runningStepName()
{
  return cerebroRunningStepName(cerebroGUITool.cerebrumLabeler.state);
}

bool CerebrumExtractionDialog::isFinished()
{
  return (cerebroGUITool.isFinished());
}

void CerebrumExtractionDialog::on_applyButton_clicked()
{
  while (step());
}

void CerebrumExtractionDialog::on_runStepButton_clicked()
{
  step();
}

void CerebrumExtractionDialog::on_previousStepButton_clicked()
{
}

void CerebrumExtractionDialog::on_nextTool_clicked()
{
  brainSuiteWindow->swapDialogs(this,brainSuiteWindow->toolManager.cortexIdentificationDialog);
}

bool CerebrumExtractionDialog::step()
{
  return ::cerebroGUITool.runStep(brainSuiteWindow);

}

void CerebrumExtractionDialog::on_restoreFromPreviousSessionButton_clicked()
{
  if (!brainSuiteWindow) return; // this should never happen
  if (brainSuiteWindow->loadExtractionStudy(CorticalExtractionData::PVC))
  {
    brainSuiteWindow->brainSuiteDataManager.vMask.copy(brainSuiteWindow->corticalExtractionData.vBrainMask);
    brainSuiteWindow->updateView(DisplayCode::MaskOn|DisplayCode::Dirty);
  }
  checkStageInputs();
}

void CerebrumExtractionDialog::updateStepName(std::string stepname)
{
  ui->stepNameText->setText(stepname.c_str());
  ui->stepNameText->repaint();
}

bool CerebrumExtractionDialog::checkStageInputs()
{
  if (!brainSuiteWindow)
  {
    std::cerr<<"Dialog is disconnected."<<std::endl;
    return false;
  }
  bool status=true;
  if (brainSuiteWindow->brainSuiteDataManager.volume==0)
  {
    std::cerr<<"No image volume loaded.\n"<<std::endl;
    status = false;
  }
  return status;
}

void CerebrumExtractionDialog::on_atlasPushButton_clicked()
{
  if (brainSuiteWindow)
    brainSuiteWindow->setCerebrumAtlasFile();
}

void CerebrumExtractionDialog::on_labelsPushButton_clicked()
{
  if (brainSuiteWindow)
    brainSuiteWindow->setCerebrumAtlasLabelFile();
}

void CerebrumExtractionDialog::on_tempPathPushButton_clicked()
{
  if (brainSuiteWindow)
    brainSuiteWindow->setCerebrumTempDirectory();
}

void CerebrumExtractionDialog::on_linearConvergenceEditBox_textEdited(const QString & /* arg1 */)
{
  updateCESfromDialog();
}

void CerebrumExtractionDialog::on_warpConvergenceEditBox_textEdited(const QString & /* arg1 */)
{
  updateCESfromDialog();
}

void CerebrumExtractionDialog::on_warpLevelSpinBox_valueChanged(int /* arg1 */)
{
  updateCESfromDialog();
}

void CerebrumExtractionDialog::on_costFunctionComboBox_currentIndexChanged(int /* index */)
{
  updateCESfromDialog();
}

void CerebrumExtractionDialog::on_verboseCheckBox_clicked()
{
  updateCESfromDialog();
}

void CerebrumExtractionDialog::on_showAlignedAtlasAsOverlayCheckBox_clicked()
{
  updateCESfromDialog();
}

void CerebrumExtractionDialog::on_useCentroidsCheckBox_clicked()
{
  updateCESfromDialog();
}
