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

#include "scrubmaskdialog.h"
#include "ui_scrubmaskdialog.h"
#include <brainsuitewindow.h>
#include <toolmanager.h>
#include <topologicalcorrectiondialog.h>
#include <maskscrubber.h>
#include <DS/timer.h>
#include <displaycode.h>
#include <brainsuiteqtsettings.h>
#include <corticalextractionsettings.h>
#include <corticalextractionsequencedialog.h>
#include <quietset.h>
#include <scrubmaskguitool.h>
#include <brainsuitewindow_t.h>

ScrubMaskGUITool scrubMaskGUITool;

ScrubMaskDialog::ScrubMaskDialog(BrainSuiteWindow *parent) :
  QDialog(parent), ui(new Ui::ScrubMaskDialog), brainSuiteWindow(parent)
{
  ui->setupUi(this);
  updateDialogFromCES();
  ui->progressBar->setRange(0,brainSuiteWindow->ces.scrubMask.nScrubIterations);
  ui->progressBar->setValue(0);
  checkStageInputs();
}

ScrubMaskDialog::~ScrubMaskDialog()
{
  delete ui;
}

void ScrubMaskDialog::updateStepName(std::string s)
{
  ui->stepNameText->setText(s.c_str());
  ui->stepNameText->repaint();
}

void ScrubMaskDialog::setProgress(const int value, const int maxValue)
{
  ui->progressBar->setRange(0,maxValue);
  ui->progressBar->setValue(value);
}

void ScrubMaskDialog::updateDialogFromCES()
{
  ui->foregroundThresholdEditBox->setText(QString::number(brainSuiteWindow->ces.scrubMask.foregroundThreshold));
  ui->backgroundThresholdEditBox->setText(QString::number(brainSuiteWindow->ces.scrubMask.backgroundThreshold));
}

void ScrubMaskDialog::updateCESfromDialog()
{
  brainSuiteWindow->ces.scrubMask.foregroundThreshold = ui->foregroundThresholdEditBox->text().toFloat();
  brainSuiteWindow->ces.scrubMask.backgroundThreshold = ui->backgroundThresholdEditBox->text().toFloat();
  if (brainSuiteWindow->toolManager.corticalExtractionSequenceDialog)
    brainSuiteWindow->toolManager.corticalExtractionSequenceDialog->updateScrubMaskParameters();
}

void ScrubMaskDialog::setStatusText(std::string status)
{
  ui->statusBoxTextEdit->clear();
  ui->statusBoxTextEdit->insertPlainText(status.c_str());
  ui->statusBoxTextEdit->repaint();
  ui->statusBoxTextEdit->moveCursor (QTextCursor::End) ;
  ui->statusBoxTextEdit->ensureCursorVisible() ;
}

bool ScrubMaskDialog::checkStageInputs()
{
  if (!brainSuiteWindow)
  {
    setStatusText("dialog is disconnected.");
    return false;
  }
  bool status=true;
  std::ostringstream errstream;
  if (brainSuiteWindow->brainSuiteDataManager.vMask.size()<=0)
  {
    errstream<<"No mask volume loaded.\n";
    status = false;
  }
  if (status)
    setStatusText("Press apply to perform mask scrubbing.");
  else
    setStatusText(errstream.str());
  return status;
}

std::string ScrubMaskDialog::runningStepName()
{
  return scrubMaskGUITool.stepName();
}

bool ScrubMaskDialog::isFinished()
{
  return (scrubMaskGUITool.state==ScrubMaskGUITool::Finished);
}

bool ScrubMaskDialog::step()
{
  return scrubMaskGUITool.runStep(brainSuiteWindow);
  return true;
}

void ScrubMaskDialog::on_applyButton_clicked()
{
  step();
}

void ScrubMaskDialog::on_nextTool_clicked()
{
  brainSuiteWindow->swapDialogs(this,brainSuiteWindow->toolManager.topologicalCorrectionDialog);
}

void ScrubMaskDialog::on_restoreFromPreviousSessionButton_clicked()
{
  if (!brainSuiteWindow) return; // this should never happen
  if (brainSuiteWindow->loadExtractionStudy(CorticalExtractionData::CTX))
  {
    brainSuiteWindow->brainSuiteDataManager.vMask.copy(brainSuiteWindow->corticalExtractionData.vCortexInitMask);
    brainSuiteWindow->updateView(DisplayCode::MaskOn|DisplayCode::Dirty);
  }
  checkStageInputs();
}
