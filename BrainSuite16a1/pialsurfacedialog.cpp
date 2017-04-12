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

#include "pialsurfacedialog.h"
#include "ui_pialsurfacedialog.h"
#include <brainsuitewindow.h>
#include <toolmanager.h>
#include <splithemispheresdialog.h>
#include <displaycode.h>
#include <pialsurfacetool.h>
#include <surfacealpha.h>
#include <DS/timer.h>
#include <waitcursor.h>
#include <brainsuiteqtsettings.h>
#include <corticalextractionsettings.h>
#include <corticalextractionsequencedialog.h>
#include <pialsurfaceguitool.h>
#include <brainsuitewindow_t.h>

PialSurfaceGUITool pialSurfaceGUITool;

bool writeOrFail(SILT::Surface *surface, std::string ofname);
bool writeOrFail(SILT::Surface &surface, std::string ofname);

void copySurface(Surface &destinationSurface, const Surface *sourceSurface)
{
  destinationSurface.vertices=sourceSurface->vertices;
  destinationSurface.triangles=sourceSurface->triangles;
  destinationSurface.vertexColor=sourceSurface->vertexColor;
  destinationSurface.computeConnectivity();
  destinationSurface.computeNormals();
}

void copySurface(Surface &destinationSurface, const Surface &sourceSurface)
{
  destinationSurface.vertices=sourceSurface.vertices;
  destinationSurface.triangles=sourceSurface.triangles;
  destinationSurface.vertexColor=sourceSurface.vertexColor;
  destinationSurface.computeConnectivity();
  destinationSurface.computeNormals();
}

PialSurfaceDialog::PialSurfaceDialog(BrainSuiteWindow *parent) :
  QDialog(parent),
  ui(new Ui::PialSurfaceDialog), brainSuiteWindow(parent)
{
  ui->setupUi(this);
  checkStageInputs();
  updateDialogFromCES();
  ui->progressBar->setValue(0);
  ui->progressBar->setRange(0,100);
}

PialSurfaceDialog::~PialSurfaceDialog()
{
  delete ui;
}

void PialSurfaceDialog::updateDialogFromCES()
{
  ui->pialPresmoothIterationsEditBox->setText(QString::number(brainSuiteWindow->ces.pial.pialPresmoothIterations));
  ui->pialIterationsEditBox->setText(QString::number(brainSuiteWindow->ces.pial.pialIterations));
  ui->pialCollisionDetectionRadiusEditBox->setText(QString::number(brainSuiteWindow->ces.pial.pialCollisionDetectionRadius));
  ui->pialThicknessLimitEditBox->setText(QString::number(brainSuiteWindow->ces.pial.pialThicknessLimit));
  ui->pialStepSizeEditBox->setText(QString::number(brainSuiteWindow->ces.pial.pialStepSize));
  ui->pialTissueThresholdEditBox->setText(QString::number(brainSuiteWindow->ces.pial.pialTissueThreshold));
  ui->pialSmoothingConstantEditBox->setText(QString::number(brainSuiteWindow->ces.pial.pialSmoothingConstant));
  ui->pialRadialConstantEditBox->setText(QString::number(brainSuiteWindow->ces.pial.pialRadialConstant));
  ui->pialUpdateSurfaceIntervalEditBox->setText(QString::number(brainSuiteWindow->ces.pial.pialUpdateSurfaceInterval));
  ui->pialConstrainWithCerebrumMaskCheckBox->setChecked(brainSuiteWindow->ces.pial.pialConstrainWithCerebrumMask);
}

void PialSurfaceDialog::updateCESfromDialog()
{
  brainSuiteWindow->ces.pial.pialPresmoothIterations=	ui->pialPresmoothIterationsEditBox->text().toInt();
  brainSuiteWindow->ces.pial.pialIterations=ui->pialIterationsEditBox->text().toInt();
  brainSuiteWindow->ces.pial.pialCollisionDetectionRadius=ui->pialCollisionDetectionRadiusEditBox->text().toFloat();
  brainSuiteWindow->ces.pial.pialThicknessLimit=ui->pialThicknessLimitEditBox->text().toFloat();
  brainSuiteWindow->ces.pial.pialStepSize=ui->pialStepSizeEditBox->text().toFloat();
  brainSuiteWindow->ces.pial.pialTissueThreshold=ui->pialTissueThresholdEditBox->text().toFloat();
  brainSuiteWindow->ces.pial.pialSmoothingConstant=ui->pialSmoothingConstantEditBox->text().toFloat();
  brainSuiteWindow->ces.pial.pialRadialConstant=ui->pialRadialConstantEditBox->text().toFloat();
  brainSuiteWindow->ces.pial.pialUpdateSurfaceInterval=ui->pialUpdateSurfaceIntervalEditBox->text().toInt();
  brainSuiteWindow->ces.pial.pialConstrainWithCerebrumMask=ui->pialConstrainWithCerebrumMaskCheckBox->isChecked();
  if (brainSuiteWindow->toolManager.corticalExtractionSequenceDialog)
    brainSuiteWindow->toolManager.corticalExtractionSequenceDialog->updatePialParameters();
}

std::string PialSurfaceDialog::runningStepName()
{
  return pialSurfaceGUITool.stepName();
}

bool PialSurfaceDialog::isFinished()
{
  return pialSurfaceGUITool.isFinished();
}

bool PialSurfaceDialog::step()
{
  return ::pialSurfaceGUITool.runStep(brainSuiteWindow,this);
}

void PialSurfaceDialog::updateStepText(std::string stepname)
{
  ui->stepNameText->setText(stepname.c_str());
  ui->stepNameText->repaint();
}

void PialSurfaceDialog::setProgress(const int value, const int maxValue)
{
  ui->progressBar->setRange(0,maxValue);
  ui->progressBar->setValue(value);
}

// would be good in a base class...
void PialSurfaceDialog::setStatusText(std::string status)
{
  ui->statusBoxTextEdit->clear();
  ui->statusBoxTextEdit->insertPlainText(status.c_str());
  ui->statusBoxTextEdit->moveCursor (QTextCursor::End) ;
  ui->statusBoxTextEdit->ensureCursorVisible();
  ui->statusBoxTextEdit->repaint();
}

void PialSurfaceDialog::updateStatusText()
{
  setStatusText(outputStream.str());
}

bool PialSurfaceDialog::checkStageInputs()
{
  if (!brainSuiteWindow)
  {
    setStatusText("Dialog is disconnected.");
    return false;
  }
  bool status=true;
  std::ostringstream errstream;
  if (ui->pialConstrainWithCerebrumMaskCheckBox->isChecked() &&
      brainSuiteWindow->corticalExtractionData.vCerebrumMask.size()<=0)
  {
    errstream<<"Cerebrum mask volume is not loaded.\n";
    status = false;
  }
  if (brainSuiteWindow->corticalExtractionData.vPVCFractions.size()<=0)
  {
    errstream<<"Tissue fraction volume is not loaded.\n";
    status = false;
  }
  if (brainSuiteWindow->corticalExtractionData.innerCorticalSurface==0)
  {
    errstream<<"Inner cortical surface is not loaded.\n";
    status = false;
  }
  if (status)
    setStatusText("Press [Apply] to generate pial cortical surface mesh.");
  else
    setStatusText(errstream.str());
  return status;
}

void PialSurfaceDialog::on_applyButton_clicked()
{
  while (step());
}

void PialSurfaceDialog::on_runStepButton_clicked()
{
  step();
}

void PialSurfaceDialog::on_previousStepButton_clicked()
{

}

void PialSurfaceDialog::on_nextTool_clicked()
{
  brainSuiteWindow->swapDialogs(this,brainSuiteWindow->toolManager.splitHemispheresDialog);
}

void PialSurfaceDialog::on_restoreFromPreviousSessionButton_clicked()
{
  if (!brainSuiteWindow) return; // this should never happen
  if (brainSuiteWindow->loadExtractionStudy(CorticalExtractionData::InnerCorticalSurface))
  {
    brainSuiteWindow->brainSuiteDataManager.vMask.copy(brainSuiteWindow->corticalExtractionData.vCortexDewispMask);
    brainSuiteWindow->updateView(DisplayCode::MaskOn|DisplayCode::Dirty);
  }
  checkStageInputs();
}

void PialSurfaceDialog::on_pialPresmoothIterationsEditBox_textEdited(const QString & /* arg1 */)
{
  updateCESfromDialog();
}

void PialSurfaceDialog::on_pialIterationsEditBox_textEdited(const QString & /* arg1 */)
{
  updateCESfromDialog();
}

void PialSurfaceDialog::on_pialCollisionDetectionRadiusEditBox_textEdited(const QString & /* arg1 */)
{
  updateCESfromDialog();
}

void PialSurfaceDialog::on_pialThicknessLimitEditBox_textEdited(const QString & /* arg1 */)
{
  updateCESfromDialog();
}

void PialSurfaceDialog::on_pialStepSizeEditBox_textEdited(const QString & /* arg1 */)
{
  updateCESfromDialog();
}

void PialSurfaceDialog::on_pialTissueThresholdEditBox_textEdited(const QString & /* arg1 */)
{
  updateCESfromDialog();
}

void PialSurfaceDialog::on_pialSmoothingConstantEditBox_textEdited(const QString & /* arg1 */)
{
  updateCESfromDialog();
}

void PialSurfaceDialog::on_pialRadialConstantEditBox_textEdited(const QString & /* arg1 */)
{
  updateCESfromDialog();
}

void PialSurfaceDialog::on_pialUpdateSurfaceIntervalEditBox_textEdited(const QString & /* arg1 */)
{
  updateCESfromDialog();
}

void PialSurfaceDialog::on_pialConstrainWithCerebrumMaskCheckBox_clicked()
{
  updateCESfromDialog();
}

void PialSurfaceDialog::on_pialLoadCustomMaskButton_clicked()
{
  pialSurfaceGUITool.loadCustomCerebrumMask(brainSuiteWindow);
}
