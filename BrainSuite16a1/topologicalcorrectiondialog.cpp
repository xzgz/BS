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

#include "topologicalcorrectiondialog.h"
#include "ui_topologicalcorrectiondialog.h"
#include <brainsuitewindow.h>
#include <toolmanager.h>
#include <dewispdialog.h>
#include <displaycode.h>
#include <waitcursor.h>
#include <DS/timer.h>
#include <corticalextractionsettings.h>
#include <corticalextractionsequencedialog.h>
#include <quietset.h>
#include <topologycorrectionguitool.h>
#include <brainsuitewindow_t.h>

TopologyCorrectionGUITool topologyCorrectionGUITool;

TopologicalCorrectionDialog::TopologicalCorrectionDialog(BrainSuiteWindow *parent) :
  QDialog(parent),
  ui(new Ui::TopologicalCorrectionDialog), brainSuiteWindow(parent), tcaState(Correction)
{
  ui->setupUi(this);
  updateDialogFromCES();
  ui->progressBar->setValue(0);
  ui->progressBar->setRange(0,100);
  checkStageInputs();
}

TopologicalCorrectionDialog::~TopologicalCorrectionDialog()
{
  delete ui;
}

void TopologicalCorrectionDialog::updateDialogFromCES()
{
  ui->minimumCorrectionEditBox->setText(QString::number(brainSuiteWindow->ces.tca.minimumCorrectionSize));
  ui->maximumCorrectionEditBox->setText(QString::number(brainSuiteWindow->ces.tca.maximumCorrectionSize));
  ui->fillOffsetEditBox->setText(QString::number(brainSuiteWindow->ces.tca.fillOffset));
}

void TopologicalCorrectionDialog::updateCESfromDialog()
{
  brainSuiteWindow->ces.tca.minimumCorrectionSize = ui->minimumCorrectionEditBox->text().toInt();
  brainSuiteWindow->ces.tca.maximumCorrectionSize = ui->maximumCorrectionEditBox->text().toInt();
  brainSuiteWindow->ces.tca.fillOffset = ui->fillOffsetEditBox->text().toInt();
  if (brainSuiteWindow->toolManager.corticalExtractionSequenceDialog)
    brainSuiteWindow->toolManager.corticalExtractionSequenceDialog->updateTCAParameters();
}

void TopologicalCorrectionDialog::updateStepText(std::string status)
{
  ui->stepNameText->setText(status.c_str());
  ui->stepNameText->repaint();
}

void TopologicalCorrectionDialog::setStatusText(std::string status)
{
  ui->statusBoxTextEdit->clear();
  ui->statusBoxTextEdit->insertPlainText(status.c_str());
  ui->statusBoxTextEdit->moveCursor (QTextCursor::End) ;
  ui->statusBoxTextEdit->ensureCursorVisible();
  ui->statusBoxTextEdit->repaint();
}

bool TopologicalCorrectionDialog::checkStageInputs()
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
    setStatusText("Press ["+ui->applyButton->text().toStdString()+"] to perform topology correction.");
  else
    setStatusText(errstream.str());
  return status;
}

bool TopologicalCorrectionDialog::fixTopology()
{
  if (!checkStageInputs()) return false;
  return topologyCorrectionGUITool.runStep(brainSuiteWindow);
}

void TopologicalCorrectionDialog::on_applyButton_clicked()
{
  fixTopology();
}

void TopologicalCorrectionDialog::on_nextTool_clicked()
{
  brainSuiteWindow->swapDialogs(this,brainSuiteWindow->toolManager.dewispDialog);
}

void TopologicalCorrectionDialog::on_restoreFromPreviousSessionButton_clicked()
{
  if (!brainSuiteWindow) return; // this should never happen
  if (brainSuiteWindow->loadExtractionStudy(CorticalExtractionData::ScrubMask))
  {
    brainSuiteWindow->brainSuiteDataManager.vMask.copy(brainSuiteWindow->corticalExtractionData.vCortexScrubbedMask);
    brainSuiteWindow->updateView(DisplayCode::MaskOn|DisplayCode::Dirty);
  }
  checkStageInputs();
}

void TopologicalCorrectionDialog::setProgress(const int value, const int maxValue)
{
  ui->progressBar->setRange(0,maxValue);
  ui->progressBar->setValue(value);
  ui->progressBar->repaint();
}

bool TopologicalCorrectionDialog::step()
{
  return fixTopology();
}

std::string TopologicalCorrectionDialog::runningStepName()
{
  switch (tcaState)
  {
    case Correction: return "Correcting topological defects";
    case Finished: return "Finished";
    default: return "";
  }
}

bool TopologicalCorrectionDialog::isFinished()
{
  return (tcaState==Finished);
}

void TopologicalCorrectionDialog::on_minimumCorrectionEditBox_textEdited(const QString & /* arg1 */)
{
  updateCESfromDialog();
}

void TopologicalCorrectionDialog::on_maximumCorrectionEditBox_textEdited(const QString & /* arg1 */)
{
  updateCESfromDialog();
}

void TopologicalCorrectionDialog::on_fillOffsetEditBox_textEdited(const QString & /* arg1 */)
{
  updateCESfromDialog();
}
