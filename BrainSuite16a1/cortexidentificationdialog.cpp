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

#include "cortexidentificationdialog.h"
#include "ui_cortexidentificationdialog.h"
#include <brainsuitewindow.h>
#include <toolmanager.h>
#include <scrubmaskdialog.h>
#include <strutil.h>
#include <qmessagebox.h>
#include "cortexmodeler.h"
#include <brainsuiteqtsettings.h>
#include <corticalextractionsettings.h>
#include <corticalextractionsequencedialog.h>
#include <quietset.h>
#include <cortexmodelguitool.h>
#include <brainsuitewindow_t.h>
#include <cerebrumlabelerqt.h>

extern CerebrumLabelerQt cerebro;

CortexIdentificationDialog::CortexIdentificationDialog(BrainSuiteWindow *parent) :
  QDialog(parent),
  ui(new Ui::CortexIdentificationDialog), brainSuiteWindow(parent), state(Computing)
{
  ui->setupUi(this);
  ui->progressBar->setRange(0,1);
  ui->progressBar->setValue(0);
  updateDialogFromCES();
  checkStageInputs();
}

CortexIdentificationDialog::~CortexIdentificationDialog()
{
  delete ui;
}

void CortexIdentificationDialog::setProgress(const int value, const int maxValue)
{
  ui->progressBar->setRange(0,maxValue);
  ui->progressBar->setValue(value);
}

void CortexIdentificationDialog::updateDialogFromCES()
{
  ui->tissueThresholdEditBox->setText(QString::number(brainSuiteWindow->ces.ctx.tissueThreshold));
  ui->includeAllSubcorticalLabelsCheckBox->setChecked(brainSuiteWindow->ces.ctx.includeAllSubcorticalAreas);
}

void CortexIdentificationDialog::updateCESfromDialog()
{
  brainSuiteWindow->ces.ctx.tissueThreshold = ui->tissueThresholdEditBox->text().toFloat();
  brainSuiteWindow->ces.ctx.includeAllSubcorticalAreas = ui->includeAllSubcorticalLabelsCheckBox->isChecked();

  if (brainSuiteWindow->toolManager.corticalExtractionSequenceDialog)
    brainSuiteWindow->toolManager.corticalExtractionSequenceDialog->updateCortexParameters();
}

void CortexIdentificationDialog::setStatusText(std::string status)
{
  ui->statusBoxTextEdit->clear();
  ui->statusBoxTextEdit->insertPlainText(status.c_str());
  ui->statusBoxTextEdit->repaint();
}

bool CortexIdentificationDialog::checkStageInputs()
{
  if (!brainSuiteWindow)
  {
    setStatusText("dialog is disconnected.");
    return false;
  }
  bool status=true;
  std::ostringstream errstream;
  if (brainSuiteWindow->corticalExtractionData.vHemiLabels.size()<=0)
  {
    errstream<<"Hemisphere labels have not been computed.\n";
    status = false;
  }
  if (brainSuiteWindow->corticalExtractionData.vPVCFractions.size()<=0)
  {
    errstream<<"Tissue fractions have not been computed.\n";
    status = false;
  }
  else if (brainSuiteWindow->corticalExtractionData.vPVCFractions.isCompatible(brainSuiteWindow->corticalExtractionData.vHemiLabels)==false)
  {
    errstream<<"Tissue fraction volume is not compatible with labeled lobes.\n";
    status = false;
  }
  if (status)
    setStatusText("Press apply to compute initial cortex mask.");
  else
    setStatusText(errstream.str());
  return status;
}
#include <cortexmodelguitool.h>
extern CortexModelGUITool cortexModelGUITool;
bool CortexIdentificationDialog::computeInnerCortexMask()
{
  if (!brainSuiteWindow) { setStatusText("dialog is disconnected."); return false; }
  if (checkStageInputs()==false) return false;
  state = Computing;
  return cortexModelGUITool.runStep(brainSuiteWindow,this);
}

void CortexIdentificationDialog::on_applyButton_clicked()
{
  if (computeInnerCortexMask())
    state = Finished;
}

void CortexIdentificationDialog::on_nextTool_clicked()
{
  brainSuiteWindow->swapDialogs(this,brainSuiteWindow->toolManager.scrubMaskDialog);
}

void CortexIdentificationDialog::on_restoreFromPreviousSessionButton_clicked()
{
  if (!brainSuiteWindow) return; // this should never happen
  brainSuiteWindow->loadExtractionStudy(CorticalExtractionData::CB);
  checkStageInputs();
}

bool CortexIdentificationDialog::step()
{
  if (state == Finished) return false;
  if (computeInnerCortexMask())
  {
    state = Finished;
    return true;
  }
  return false;
}

std::string CortexIdentificationDialog::runningStepName()
{
  if (state==Computing) return "Computing inner cortex mask";
  return "finished";
}

bool CortexIdentificationDialog::isFinished()
{
  return (state==Finished);
}


void CortexIdentificationDialog::on_tissueThresholdEditBox_textEdited(const QString & /* arg1 */)
{
  updateCESfromDialog();
}

void CortexIdentificationDialog::on_includeAllSubcorticalLabelsCheckBox_clicked()
{
  updateCESfromDialog();
}
