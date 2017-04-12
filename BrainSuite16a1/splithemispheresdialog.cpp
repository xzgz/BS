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

#include "splithemispheresdialog.h"
#include "ui_splithemispheresdialog.h"
#include <brainsuitewindow.h>
#include <toolmanager.h>
#include <brainsplitter.h>
#include <DS/timer.h>
#include <waitcursor.h>
#include <surfacealpha.h>
#include <displaycode.h>
#include <brainsuiteqtsettings.h>
#include <svregistrationdialog.h>
#include <hemisplitguitool.h>
#include <corticalextractionsettings.h>
#include <brainsuitewindow_t.h>

HemisplitGUITool hemisplitGUITool;

SplitHemispheresDialog::SplitHemispheresDialog(BrainSuiteWindow *parent) :
  QDialog(parent),
  ui(new Ui::SplitHemispheresDialog), brainSuiteWindow(parent)
{
  ui->setupUi(this);
  ui->progressBar->setRange(0,4);
  checkStageInputs();
}

SplitHemispheresDialog::~SplitHemispheresDialog()
{
  delete ui;
}


std::string SplitHemispheresDialog::runningStepName()
{
  switch (splitState)
  {
    case Splitting: return "Splitting hemispheres";
    case Finished: return "Finished";
    default: return "";
  }
}

bool SplitHemispheresDialog::isFinished()
{
  return (splitState==Finished);
}

bool SplitHemispheresDialog::step()
{
  return splitHemispheres();
}

void SplitHemispheresDialog::setProgress(const int value, const int maxValue)
{
  ui->progressBar->setRange(0,maxValue);
  ui->progressBar->setValue(value);
}

void SplitHemispheresDialog::updateStepText(std::string stepname)
{
  ui->stepNameText->setText(stepname.c_str());
  ui->stepNameText->repaint();
}

void SplitHemispheresDialog::updateStatus()
{
  setStatusText(outputStream.str());
}

void SplitHemispheresDialog::setStatusText(std::string status)
{
  ui->statusBoxTextEdit->clear();
  ui->statusBoxTextEdit->insertPlainText(status.c_str());
  ui->statusBoxTextEdit->repaint();
  ui->statusBoxTextEdit->moveCursor (QTextCursor::End) ;
  ui->statusBoxTextEdit->ensureCursorVisible() ;
}

bool SplitHemispheresDialog::checkStageInputs()
{
  if (!brainSuiteWindow)
  {
    setStatusText("Dialog is disconnected.");
    return false;
  }
  bool status=true;
  std::ostringstream errstream;
  if (brainSuiteWindow->corticalExtractionData.vHemiLabels.size()<=0)
  {
    errstream<<"Hemisphere volume is not loaded.\n";
    status = false;
  }
  if (brainSuiteWindow->corticalExtractionData.innerCorticalSurface==0)
  {
    errstream<<"Inner cortical surface is not loaded.\n";
    status = false;
  }
  if (brainSuiteWindow->corticalExtractionData.pialSurface==0)
  {
    errstream<<"Pial cortical surface is not loaded.\n";
  }
  if (status)
    setStatusText("Press [Apply] to split surface meshes into hemispheres.");
  else
    setStatusText(errstream.str());
  return status;
}

bool SplitHemispheresDialog::splitHemispheres()
{
  outputStream.str(""); outputStream.clear();
  if (!checkStageInputs()) return false;
  return hemisplitGUITool.runStep(brainSuiteWindow);
}

void SplitHemispheresDialog::on_applyButton_clicked()
{
  splitHemispheres();
}

void SplitHemispheresDialog::on_runStepButton_clicked()
{

}

void SplitHemispheresDialog::on_previousStepButton_clicked()
{

}

void SplitHemispheresDialog::on_nextTool_clicked()
{
  brainSuiteWindow->swapDialogs(this,brainSuiteWindow->toolManager.svRegistrationDialog);
}

void SplitHemispheresDialog::on_restoreFromPreviousSessionButton_clicked()
{
  if (!brainSuiteWindow) return; // this should never happen
  if (brainSuiteWindow->loadExtractionStudy(CorticalExtractionData::PialSurface))
  {
    brainSuiteWindow->brainSuiteDataManager.vMask.copy(brainSuiteWindow->corticalExtractionData.vCortexDewispMask);
    brainSuiteWindow->updateView(DisplayCode::MaskOn|DisplayCode::Dirty);
  }
  checkStageInputs();
}
