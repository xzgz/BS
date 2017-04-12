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

#include "dewispdialog.h"
#include "ui_dewispdialog.h"
#include <brainsuitewindow.h>
#include <toolmanager.h>
#include <innercorticalsurfacedialog.h>
#include <DS/timer.h>
#include <displaycode.h>
#include <dewisp.h>
#include <waitcursor.h>
#include <brainsuiteqtsettings.h>
#include <corticalextractionsettings.h>
#include <corticalextractionsequencedialog.h>
#include "dewispguitool.h"
#include <brainsuitewindow_t.h>

DewispGUITool dewispGUITool;

// TODO: move default settings to class
DewispDialog::DewispDialog(BrainSuiteWindow *parent) :
  QDialog(parent),
  ui(new Ui::DewispDialog), brainSuiteWindow(parent)
{
  ui->setupUi(this);
  checkStageInputs();
  updateDialogFromCES();
  ui->progressBar->setRange(0,10);
  ui->progressBar->setValue(0);
}

void DewispDialog::setProgress(const int value, const int maxValue)
{
  ui->progressBar->setRange(0,maxValue);
  ui->progressBar->setValue(value);
}

void DewispDialog::updateStepText(std::string status)
{
  ui->stepNameText->setText(status.c_str());
  ui->stepNameText->repaint();
}

void DewispDialog::updateDialogFromCES()
{
  ui->dewispMaximumIterationsEditBox->setText(QString::number(brainSuiteWindow->ces.dewisp.maximumIterations));
  ui->dewispThresholdEditBox->setText(QString::number(brainSuiteWindow->ces.dewisp.threshold));
}

void DewispDialog::updateCESfromDialog()
{
  brainSuiteWindow->ces.dewisp.maximumIterations=ui->dewispMaximumIterationsEditBox->text().toInt();
  brainSuiteWindow->ces.dewisp.threshold=ui->dewispThresholdEditBox->text().toInt();
  if (brainSuiteWindow->toolManager.corticalExtractionSequenceDialog)
    brainSuiteWindow->toolManager.corticalExtractionSequenceDialog->updateDewispParameters();
}

DewispDialog::~DewispDialog()
{
  delete ui;
}

std::string DewispDialog::runningStepName()
{
  return dewispGUITool.stepName();
}

bool DewispDialog::isFinished()
{
  return dewispGUITool.isFinished();
}

void DewispDialog::setStatusText(std::string status)
{
  ui->statusBoxTextEdit->clear();
  ui->statusBoxTextEdit->insertPlainText(status.c_str());
  ui->statusBoxTextEdit->repaint();
  ui->statusBoxTextEdit->moveCursor (QTextCursor::End) ;
  ui->statusBoxTextEdit->ensureCursorVisible() ;
}

bool DewispDialog::checkStageInputs()
{
  if (!brainSuiteWindow)
  {
    setStatusText("Dialog is disconnected.");
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
    setStatusText("Press [Apply] to perform mask dewisping.");
  else
    setStatusText(errstream.str());
  return status;
}

void DewispDialog::on_applyButton_clicked()
{
  applyDewispFilter();
}

bool DewispDialog::step()
{
  return applyDewispFilter();
}


bool DewispDialog::applyDewispFilter()
{
  if (!checkStageInputs()) return false;
  return dewispGUITool.runStep(brainSuiteWindow);
}

void DewispDialog::on_nextTool_clicked()
{
  brainSuiteWindow->swapDialogs(this,brainSuiteWindow->toolManager.innerCorticalSurfaceDialog);
}

void DewispDialog::on_restoreFromPreviousSessionButton_clicked()
{
  if (!brainSuiteWindow) return; // this should never happen
  if (brainSuiteWindow->loadExtractionStudy(CorticalExtractionData::TCA))
  {
    brainSuiteWindow->brainSuiteDataManager.vMask.copy(brainSuiteWindow->corticalExtractionData.vCortexTCAMask);
    brainSuiteWindow->updateView(DisplayCode::MaskOn|DisplayCode::Dirty);
  }
  checkStageInputs();
}

void DewispDialog::on_dewispThresholdEditBox_textEdited(const QString & /* arg1 */)
{
  updateCESfromDialog();
}

void DewispDialog::on_dewispMaximumIterationsEditBox_textEdited(const QString & /* arg1 */)
{
  updateCESfromDialog();
}
