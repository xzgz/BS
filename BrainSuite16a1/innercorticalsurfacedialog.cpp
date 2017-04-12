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

#include "innercorticalsurfacedialog.h"
#include "ui_innercorticalsurfacedialog.h"
#include <brainsuitewindow.h>
#include <toolmanager.h>
#include <pialsurfacedialog.h>
#include <waitcursor.h>
#include <marchingcubes.h>
#include <surfacealpha.h>
#include <surflib.h>
#include <brainsuiteqtsettings.h>
#include <corticalextractionsettings.h>
#include <corticalextractionsequencedialog.h>
#include <innercorticalsurfaceguitool.h>
#include <displaycode.h>
#include <brainsuitewindow_t.h>

InnerCorticalSurfaceGUITool innerCorticalSurfaceGUITool;

InnerCorticalSurfaceDialog::InnerCorticalSurfaceDialog(BrainSuiteWindow *parent) :
  QDialog(parent),
  ui(new Ui::InnerCorticalSurfaceDialog), brainSuiteWindow(parent), surfaceState(Generating)
{
  ui->setupUi(this);
  updateDialogFromCES();
  ui->progressBar->setRange(0,2);
  ui->progressBar->setValue(0);
  checkStageInputs();
}

InnerCorticalSurfaceDialog::~InnerCorticalSurfaceDialog()
{
  delete ui;
}

void InnerCorticalSurfaceDialog::updateDialogFromCES()
{
  ui->smoothingIterationsEditBox->setText(QString::number(brainSuiteWindow->ces.ics.smoothingIterations));
  ui->smoothingConstantEditBox->setText(QString::number(brainSuiteWindow->ces.ics.smoothingConstant));
  ui->curvatureWeightEditBox->setText(QString::number(brainSuiteWindow->ces.ics.curvatureWeight));
}

void InnerCorticalSurfaceDialog::updateCESfromDialog()
{
  brainSuiteWindow->ces.ics.smoothingIterations=ui->smoothingIterationsEditBox->text().toInt();
  brainSuiteWindow->ces.ics.smoothingConstant=ui->smoothingConstantEditBox->text().toFloat();
  brainSuiteWindow->ces.ics.curvatureWeight=ui->curvatureWeightEditBox->text().toFloat();
  if (brainSuiteWindow->toolManager.corticalExtractionSequenceDialog)
    brainSuiteWindow->toolManager.corticalExtractionSequenceDialog->updateICSParameters();
}

std::string InnerCorticalSurfaceDialog::runningStepName()
{
  switch (surfaceState)
  {
    case Generating: return "Generating inner cortical surface";
    case Finished: return "Finished";
    default: return "";
  }
}

bool InnerCorticalSurfaceDialog::isFinished()
{
  return (surfaceState==Finished);
}

bool InnerCorticalSurfaceDialog::step()
{
  return generateSurface();
}

void InnerCorticalSurfaceDialog::updateStepText(std::string stepname)
{
  ui->stepNameText->setText(stepname.c_str());
  ui->stepNameText->repaint();
}

void InnerCorticalSurfaceDialog::setProgress(const int value, const int maxValue)
{
  ui->progressBar->setRange(0,maxValue);
  ui->progressBar->setValue(value);
}

void InnerCorticalSurfaceDialog::setStatusText(std::string status)
{
  ui->statusBoxTextEdit->clear();
  ui->statusBoxTextEdit->insertPlainText(status.c_str());
  ui->statusBoxTextEdit->repaint();
  ui->statusBoxTextEdit->moveCursor (QTextCursor::End) ;
  ui->statusBoxTextEdit->ensureCursorVisible() ;
}

bool InnerCorticalSurfaceDialog::checkStageInputs()
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
    setStatusText("Press [Apply] to generate inner cortical surface mesh.");
  else
    setStatusText(errstream.str());
  return status;
}

bool InnerCorticalSurfaceDialog::generateSurface()
{
  if (!checkStageInputs()) return false;
  innerCorticalSurfaceGUITool.runStep(brainSuiteWindow);
  return true;
}

void InnerCorticalSurfaceDialog::on_applyButton_clicked()
{
  generateSurface();
}

void InnerCorticalSurfaceDialog::on_nextTool_clicked()
{
  brainSuiteWindow->swapDialogs(this,brainSuiteWindow->toolManager.pialSurfaceDialog);
}

void InnerCorticalSurfaceDialog::on_restoreFromPreviousSessionButton_clicked()
{
  if (!brainSuiteWindow) return; // this should never happen
  if (brainSuiteWindow->loadExtractionStudy(CorticalExtractionData::Dewisp))
  {
    brainSuiteWindow->brainSuiteDataManager.vMask.copy(brainSuiteWindow->corticalExtractionData.vCortexDewispMask);
    brainSuiteWindow->updateView(DisplayCode::MaskOn|DisplayCode::Dirty);
  }
  checkStageInputs();
}

void InnerCorticalSurfaceDialog::on_smoothingIterationsEditBox_textEdited(const QString & /* arg1 */)
{
  updateCESfromDialog();
}

void InnerCorticalSurfaceDialog::on_smoothingConstantEditBox_textEdited(const QString & /* arg1 */)
{
  updateCESfromDialog();
}

void InnerCorticalSurfaceDialog::on_curvatureWeightEditBox_textEdited(const QString & /* arg1 */)
{
  updateCESfromDialog();
}
