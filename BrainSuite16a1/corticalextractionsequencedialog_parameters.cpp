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

#include <corticalextractionsequencedialog.h>
#include "ui_corticalextractionsequencedialog.h"
#include <corticalextractionsettings.h>
#include <brainsuitewindow.h>
#include <skullstrippingdialog.h>
#include <nonuniformitycorrectiondialog.h>
#include <tissueclassificationdialog.h>
#include <cerebrumextractiondialog.h>
#include <cortexidentificationdialog.h>
#include <scrubmaskdialog.h>
#include <topologicalcorrectiondialog.h>
#include <dewispdialog.h>
#include <innercorticalsurfacedialog.h>
#include <pialsurfacedialog.h>
#include <splithemispheresdialog.h>
#include <corticalextractionsequencedialog.h>
#include <quietset.h>
#include <skullandscalpdialog.h>
#include <svregistrationdialog.h>

void CorticalExtractionSequenceDialog::on_atlasPushButton_clicked()
{
  if (brainSuiteWindow)
    brainSuiteWindow->setCerebrumAtlasFile();
}

void CorticalExtractionSequenceDialog::on_labelsPushButton_clicked()
{
  if (brainSuiteWindow)
    brainSuiteWindow->setCerebrumAtlasLabelFile();
}

void CorticalExtractionSequenceDialog::on_tempPathPushButton_clicked()
{
  if (brainSuiteWindow)
    brainSuiteWindow->setCerebrumTempDirectory();
}

void CorticalExtractionSequenceDialog::on_parameterPageComboBox_currentIndexChanged(int index)
{
  ui->parametersStackedWidget->setCurrentIndex(index);
}

void CorticalExtractionSequenceDialog::on_skullStrippingCheckBox_pressed()
{
  ui->parameterPageComboBox->setCurrentIndex(ExtractionStages::SkullStripping);
}

void CorticalExtractionSequenceDialog::on_skullfinderCheckBox_pressed()
{
  ui->parameterPageComboBox->setCurrentIndex(ExtractionStages::SkullAndScalp);
}

void CorticalExtractionSequenceDialog::on_biasFieldCorrectionCheckBox_pressed()
{
  ui->parameterPageComboBox->setCurrentIndex(ExtractionStages::BiasFieldCorrection);
}

void CorticalExtractionSequenceDialog::on_tissueClassificationCorrectionCheckBox_pressed()
{
  ui->parameterPageComboBox->setCurrentIndex(ExtractionStages::TissueClassification);
}

void CorticalExtractionSequenceDialog::on_cerebrumLabelingCheckBox_pressed()
{
  ui->parameterPageComboBox->setCurrentIndex(ExtractionStages::CerebrumLabeling);
}

void CorticalExtractionSequenceDialog::on_innerCortexMaskCheckBox_pressed()
{
  ui->parameterPageComboBox->setCurrentIndex(ExtractionStages::InnerCorticalMask);
}

void CorticalExtractionSequenceDialog::on_scrubMaskCheckBox_pressed()
{
  ui->parameterPageComboBox->setCurrentIndex(ExtractionStages::ScrubMask);
}

void CorticalExtractionSequenceDialog::on_topologyCorrectionCheckBox_pressed()
{
  ui->parameterPageComboBox->setCurrentIndex(ExtractionStages::TopologyCorrection);
}

void CorticalExtractionSequenceDialog::on_dewispCheckBox_pressed()
{
  ui->parameterPageComboBox->setCurrentIndex(ExtractionStages::WispFilter);
}

void CorticalExtractionSequenceDialog::on_innerCorticalSurfaceCheckBox_pressed()
{
  ui->parameterPageComboBox->setCurrentIndex(ExtractionStages::InnerCorticalSurface);
}

void CorticalExtractionSequenceDialog::on_pialSurfaceCheckBox_pressed()
{
  ui->parameterPageComboBox->setCurrentIndex(ExtractionStages::PialSurface);
}

void CorticalExtractionSequenceDialog::on_hemisplitCheckBox_pressed()
{
  ui->parameterPageComboBox->setCurrentIndex(ExtractionStages::SplitHemispheres);
}

void CorticalExtractionSequenceDialog::on_registerLabelBrainCheckBox_pressed()
{
  ui->parameterPageComboBox->setCurrentIndex(ExtractionStages::RegisterAndLabel);
}

void CorticalExtractionSequenceDialog::updateBSEParameters()
{
  ui->diffusionConstantEditBox->setText(QString::number(brainSuiteWindow->ces.bse.diffusionConstant));
  ui->diffusionIterationsEditBox->setText(QString::number(brainSuiteWindow->ces.bse.diffusionIterations));
  ui->edgeConstantEditBox->setText(QString::number(brainSuiteWindow->ces.bse.edgeConstant));
  ::setValueQuiet(ui->erosionSizeSpinbox,brainSuiteWindow->ces.bse.erosionSize);
  ui->trimSpinalCordCheckBox->setChecked(brainSuiteWindow->ces.bse.trimSpinalCord);
  ui->dilateFinalMaskCheckBox->setChecked(brainSuiteWindow->ces.bse.dilateFinalMask);
}

void CorticalExtractionSequenceDialog::updateSkullfinderParameters()
{
  ui->skullComputeThresholdsCheckBox->setChecked(brainSuiteWindow->ces.skullfinder.computeThresholds);
  ui->skullThresholdEditBox->setText(QString::number(brainSuiteWindow->ces.skullfinder.skullThreshold));
  ui->scalpThresholdEditBox->setText(QString::number(brainSuiteWindow->ces.skullfinder.scalpThreshold));
}

void CorticalExtractionSequenceDialog::updateBFCParameters()
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
  updateBFCwidgets();
}

void CorticalExtractionSequenceDialog::updateBFCwidgets()
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

void CorticalExtractionSequenceDialog::updatePVCParameters()
{
  ui->spatialPriorWeightingEditBox->setText(QString::number(brainSuiteWindow->ces.pvc.spatialPrior));
}

void CorticalExtractionSequenceDialog::updateCerebroParameters()
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

void CorticalExtractionSequenceDialog::updateCortexParameters()
{
  ui->tissueThresholdEditBox->setText(QString::number(brainSuiteWindow->ces.ctx.tissueThreshold));
  ui->includeAllSubcorticalLabelsCheckBox->setChecked(brainSuiteWindow->ces.ctx.includeAllSubcorticalAreas);

}

void CorticalExtractionSequenceDialog::updateScrubMaskParameters()
{
  ui->foregroundThresholdEditBox->setText(QString::number(brainSuiteWindow->ces.scrubMask.foregroundThreshold));
  ui->backgroundThresholdEditBox->setText(QString::number(brainSuiteWindow->ces.scrubMask.backgroundThreshold));
}

void CorticalExtractionSequenceDialog::updateTCAParameters()
{
  ui->minimumCorrectionEditBox->setText(QString::number(brainSuiteWindow->ces.tca.minimumCorrectionSize));
  ui->maximumCorrectionEditBox->setText(QString::number(brainSuiteWindow->ces.tca.maximumCorrectionSize));
  ui->fillOffsetEditBox->setText(QString::number(brainSuiteWindow->ces.tca.fillOffset));
}

void CorticalExtractionSequenceDialog::updateDewispParameters()
{
  ui->dewispMaximumIterationsEditBox->setText(QString::number(brainSuiteWindow->ces.dewisp.maximumIterations));
  ui->dewispThresholdEditBox->setText(QString::number(brainSuiteWindow->ces.dewisp.threshold));
}

void CorticalExtractionSequenceDialog::updateICSParameters()
{
  ui->smoothingIterationsEditBox->setText(QString::number(brainSuiteWindow->ces.ics.smoothingIterations));
  ui->smoothingConstantEditBox->setText(QString::number(brainSuiteWindow->ces.ics.smoothingConstant));
  ui->curvatureWeightEditBox->setText(QString::number(brainSuiteWindow->ces.ics.curvatureWeight));
}

void CorticalExtractionSequenceDialog::updatePialParameters()
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

void CorticalExtractionSequenceDialog::updateSVRegParameters()
{
  ui->surfaceRegistrationOnlyCheckBox->setChecked(brainSuiteWindow->ces.svreg.surfaceOnly);
  ui->refineSulcalCurvesCheckBox->setChecked(brainSuiteWindow->ces.svreg.refineSulci);
  ui->labeledAtlasComboBox->setCurrentIndex(brainSuiteWindow->ces.svreg.atlasIndex);
  ui->extendLabelsCheckBox->setChecked(brainSuiteWindow->ces.svreg.extendLabels);
}
void CorticalExtractionSequenceDialog::updateAllfromCES()
{
  updateBFCwidgets();
  updateBSEParameters();
  updateSkullfinderParameters();
  updateBFCParameters();
  updatePVCParameters();
  updateCerebroParameters();
  updateCortexParameters();
  updateScrubMaskParameters();
  updateTCAParameters();
  updateDewispParameters();
  updateICSParameters();
  updatePialParameters();
  updateSVRegParameters();
}

template <class Dialog>
void updateDialogFromCES(std::unique_ptr<Dialog> &dlg)
{
  if (dlg)
    dlg->updateDialogFromCES();
}

void CorticalExtractionSequenceDialog::updateCES_BSEParameters()
{
  brainSuiteWindow->ces.bse.diffusionIterations = ui->diffusionIterationsEditBox->text().toInt();
  brainSuiteWindow->ces.bse.diffusionConstant = ui->diffusionConstantEditBox->text().toFloat();
  brainSuiteWindow->ces.bse.edgeConstant = ui->edgeConstantEditBox->text().toFloat();
  brainSuiteWindow->ces.bse.erosionSize = ui->erosionSizeSpinbox->value();
  brainSuiteWindow->ces.bse.trimSpinalCord = ui->trimSpinalCordCheckBox->isChecked();
  brainSuiteWindow->ces.bse.dilateFinalMask = ui->dilateFinalMaskCheckBox->isChecked();
  if (brainSuiteWindow && brainSuiteWindow->toolManager.skullStrippingDialog)
    brainSuiteWindow->toolManager.skullStrippingDialog->updateDialogFromCES();
}

void CorticalExtractionSequenceDialog::updateCES_SkullfinderParameters()
{
  brainSuiteWindow->ces.skullfinder.computeThresholds = ui->skullComputeThresholdsCheckBox->isChecked();
  brainSuiteWindow->ces.skullfinder.skullThreshold = ui->skullThresholdEditBox->text().toInt();
  brainSuiteWindow->ces.skullfinder.scalpThreshold = ui->scalpThresholdEditBox->text().toInt();
  if (brainSuiteWindow && brainSuiteWindow->toolManager.skullfinderDialog)
    brainSuiteWindow->toolManager.skullfinderDialog->updateDialogFromCES();
}

void CorticalExtractionSequenceDialog::updateCES_BFCParameters()
{
  brainSuiteWindow->ces.bfc.histogramRadius=ui->histogramRadiusEditBox->text().toInt();
  brainSuiteWindow->ces.bfc.biasEstimateSampleSpacing=ui->sampleSpacingEditBox->text().toInt();
  brainSuiteWindow->ces.bfc.controlPointSpacing=ui->controlPointSpacingEditBox->text().toInt();
  brainSuiteWindow->ces.bfc.splineStiffness=ui->splineStiffnessEditBox->text().toFloat();
  brainSuiteWindow->ces.bfc.roiType=(ui->cuboidRadioButton->isChecked()) ? CorticalExtractionSettings::BFC::Block : CorticalExtractionSettings::BFC::Ellipsoid;
  brainSuiteWindow->ces.bfc.biasRange.first=ui->biasRangeLowerEditBox->text().toFloat();
  brainSuiteWindow->ces.bfc.biasRange.second=ui->biasRangeUpperEditBox->text().toFloat();
  brainSuiteWindow->ces.bfc.iterative = ui->iterativeModeCheckBox->isChecked();
  if (brainSuiteWindow && brainSuiteWindow->toolManager.nonuniformityCorrectionDialog)
    brainSuiteWindow->toolManager.nonuniformityCorrectionDialog->updateDialogFromCES();
}

void CorticalExtractionSequenceDialog::updateCES_PVCParameters()
{
  brainSuiteWindow->ces.pvc.spatialPrior=ui->spatialPriorWeightingEditBox->text().toFloat();
  if (brainSuiteWindow)
    updateDialogFromCES(brainSuiteWindow->toolManager.tissueClassificationDialog);
}

void CorticalExtractionSequenceDialog::updateCES_CerebroParameters()
{
  brainSuiteWindow->ces.cbm.useCentroids=ui->useCentroidsCheckBox->isChecked();
  brainSuiteWindow->ces.cbm.costFunction=ui->costFunctionComboBox->currentIndex();
  brainSuiteWindow->ces.cbm.linearConvergence=ui->linearConvergenceEditBox->text().toFloat();
  brainSuiteWindow->ces.cbm.warpConvergence=ui->warpConvergenceEditBox->text().toFloat();
  brainSuiteWindow->ces.cbm.warpLevel=ui->warpLevelSpinBox->value();
  brainSuiteWindow->ces.cbm.verbose=ui->verboseCheckBox->isChecked();
  brainSuiteWindow->ces.cbm.showAtlasOverlay=ui->showAlignedAtlasAsOverlayCheckBox->isChecked();
  if (brainSuiteWindow)
    updateDialogFromCES(brainSuiteWindow->toolManager.cerebrumExtractionDialog);
}

void CorticalExtractionSequenceDialog::updateCES_CortexParameters()
{
  brainSuiteWindow->ces.ctx.tissueThreshold = ui->tissueThresholdEditBox->text().toFloat();
  brainSuiteWindow->ces.ctx.includeAllSubcorticalAreas = ui->includeAllSubcorticalLabelsCheckBox->isChecked();
  if (brainSuiteWindow)
    updateDialogFromCES(brainSuiteWindow->toolManager.cortexIdentificationDialog);
}

void CorticalExtractionSequenceDialog::updateCES_ScrubMaskParameters()
{
  brainSuiteWindow->ces.scrubMask.foregroundThreshold = ui->foregroundThresholdEditBox->text().toFloat();
  brainSuiteWindow->ces.scrubMask.backgroundThreshold = ui->backgroundThresholdEditBox->text().toFloat();
  if (brainSuiteWindow)
    updateDialogFromCES(brainSuiteWindow->toolManager.scrubMaskDialog);
}

void CorticalExtractionSequenceDialog::updateCES_TCAParameters()
{
  brainSuiteWindow->ces.tca.minimumCorrectionSize = ui->minimumCorrectionEditBox->text().toInt();
  brainSuiteWindow->ces.tca.maximumCorrectionSize = ui->maximumCorrectionEditBox->text().toInt();
  brainSuiteWindow->ces.tca.fillOffset = ui->fillOffsetEditBox->text().toInt();
  if (brainSuiteWindow)
    updateDialogFromCES(brainSuiteWindow->toolManager.topologicalCorrectionDialog);
}

void CorticalExtractionSequenceDialog::updateCES_DewispParameters()
{
  brainSuiteWindow->ces.dewisp.maximumIterations=ui->dewispMaximumIterationsEditBox->text().toInt();
  brainSuiteWindow->ces.dewisp.threshold=ui->dewispThresholdEditBox->text().toInt();
  if (brainSuiteWindow)
    updateDialogFromCES(brainSuiteWindow->toolManager.dewispDialog);
}

void CorticalExtractionSequenceDialog::updateCES_ICSParameters()
{
  brainSuiteWindow->ces.ics.smoothingIterations=ui->smoothingIterationsEditBox->text().toInt();
  brainSuiteWindow->ces.ics.smoothingConstant=ui->smoothingConstantEditBox->text().toFloat();
  brainSuiteWindow->ces.ics.curvatureWeight=ui->curvatureWeightEditBox->text().toFloat();
  if (brainSuiteWindow)
    updateDialogFromCES(brainSuiteWindow->toolManager.innerCorticalSurfaceDialog);
}

void CorticalExtractionSequenceDialog::updateCES_PialParameters()
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
  if (brainSuiteWindow)
    updateDialogFromCES(brainSuiteWindow->toolManager.pialSurfaceDialog);
}

void CorticalExtractionSequenceDialog::updateCES_SVRegParameters()
{
  brainSuiteWindow->ces.svreg.refineSulci=ui->refineSulcalCurvesCheckBox->isChecked();
  brainSuiteWindow->ces.svreg.surfaceOnly=ui->surfaceRegistrationOnlyCheckBox->isChecked();
  brainSuiteWindow->ces.svreg.extendLabels=ui->extendLabelsCheckBox->isChecked();
  brainSuiteWindow->ces.svreg.atlasIndex=ui->labeledAtlasComboBox->currentIndex();
  brainSuiteWindow->ces.svreg.atlasName=ui->labeledAtlasComboBox->currentText().toStdString();
  if (brainSuiteWindow)
    updateDialogFromCES(brainSuiteWindow->toolManager.svRegistrationDialog);
}

// callbacks
void CorticalExtractionSequenceDialog::on_diffusionIterationsEditBox_textEdited(const QString & /* arg1 */)
{
  updateCES_BSEParameters();
}

void CorticalExtractionSequenceDialog::on_diffusionConstantEditBox_textEdited(const QString & /* arg1 */)
{
  updateCES_BSEParameters();
}

void CorticalExtractionSequenceDialog::on_edgeConstantEditBox_textEdited(const QString & /* arg1 */)
{
  updateCES_BSEParameters();
}

void CorticalExtractionSequenceDialog::on_dilateFinalMaskCheckBox_clicked()
{
  updateCES_BSEParameters();
}

void CorticalExtractionSequenceDialog::on_trimSpinalCordCheckBox_clicked()
{
  updateCES_BSEParameters();
}

void CorticalExtractionSequenceDialog::on_erosionSizeSpinbox_valueChanged(int /* arg1 */)
{
  updateCES_BSEParameters();
}

void CorticalExtractionSequenceDialog::on_skullComputeThresholdsCheckBox_clicked()
{
  updateCES_SkullfinderParameters();
}

void CorticalExtractionSequenceDialog::on_skullThresholdEditBox_textEdited(const QString & /* arg1 */)
{
  updateCES_SkullfinderParameters();
}

void CorticalExtractionSequenceDialog::on_scalpThresholdEditBox_textEdited(const QString & /* arg1 */)
{
  updateCES_SkullfinderParameters();
}

void CorticalExtractionSequenceDialog::on_iterativeModeCheckBox_clicked()
{
  updateCES_BFCParameters();
  updateBFCwidgets();
}

void CorticalExtractionSequenceDialog::on_histogramRadiusEditBox_textEdited(const QString & /* arg1 */)
{
  updateCES_BFCParameters();
}

void CorticalExtractionSequenceDialog::on_sampleSpacingEditBox_textEdited(const QString & /* arg1 */)
{
  updateCES_BFCParameters();
}

void CorticalExtractionSequenceDialog::on_controlPointSpacingEditBox_textEdited(const QString & /* arg1 */)
{
  updateCES_BFCParameters();
}

void CorticalExtractionSequenceDialog::on_splineStiffnessEditBox_textEdited(const QString & /* arg1 */)
{
  updateCES_BFCParameters();
}

void CorticalExtractionSequenceDialog::on_cuboidRadioButton_clicked()
{
  updateCES_BFCParameters();
}

void CorticalExtractionSequenceDialog::on_ellipsoidRadioButton_clicked()
{
  updateCES_BFCParameters();
}

void CorticalExtractionSequenceDialog::on_biasRangeLowerEditBox_textEdited(const QString & /* arg1 */)
{
  updateCES_BFCParameters();
}

void CorticalExtractionSequenceDialog::on_biasRangeUpperEditBox_textEdited(const QString & /* arg1 */)
{
  updateCES_BFCParameters();
}

void CorticalExtractionSequenceDialog::on_spatialPriorWeightingEditBox_textEdited(const QString & /* arg1 */)
{
  updateCES_PVCParameters();
}

void CorticalExtractionSequenceDialog::on_useCentroidsCheckBox_clicked()
{
  updateCES_CerebroParameters();
}

void CorticalExtractionSequenceDialog::on_linearConvergenceEditBox_textEdited(const QString & /* arg1 */)
{
  updateCES_CerebroParameters();
}

void CorticalExtractionSequenceDialog::on_warpConvergenceEditBox_textEdited(const QString & /* arg1 */)
{
  updateCES_CerebroParameters();
}

void CorticalExtractionSequenceDialog::on_warpLevelSpinBox_valueChanged(int /* arg1 */)
{
  updateCES_CerebroParameters();
}

void CorticalExtractionSequenceDialog::on_costFunctionComboBox_currentIndexChanged(int /* index */)
{
  updateCES_CerebroParameters();
}

void CorticalExtractionSequenceDialog::on_verboseCheckBox_clicked()
{
  updateCES_CerebroParameters();
}

void CorticalExtractionSequenceDialog::on_showAlignedAtlasAsOverlayCheckBox_clicked()
{
  updateCES_CerebroParameters();
}

void CorticalExtractionSequenceDialog::on_tissueThresholdEditBox_textEdited(const QString & /* arg1 */)
{
  updateCES_CortexParameters();
}

void CorticalExtractionSequenceDialog::on_includeAllSubcorticalLabelsCheckBox_clicked()
{
  updateCES_CortexParameters();
}

void CorticalExtractionSequenceDialog::on_foregroundThresholdEditBox_textEdited(const QString & /* arg1 */)
{
  updateCES_ScrubMaskParameters();
}

void CorticalExtractionSequenceDialog::on_backgroundThresholdEditBox_textEdited(const QString & /* arg1 */)
{
  updateCES_ScrubMaskParameters();
}

void CorticalExtractionSequenceDialog::on_minimumCorrectionEditBox_textEdited(const QString & /* arg1 */)
{
  updateCES_TCAParameters();
}

void CorticalExtractionSequenceDialog::on_maximumCorrectionEditBox_textEdited(const QString & /* arg1 */)
{
  updateCES_TCAParameters();
}

void CorticalExtractionSequenceDialog::on_fillOffsetEditBox_textEdited(const QString & /* arg1 */)
{
  updateCES_TCAParameters();
}

void CorticalExtractionSequenceDialog::on_dewispThresholdEditBox_textEdited(const QString & /* arg1 */)
{
  updateCES_DewispParameters();
}

void CorticalExtractionSequenceDialog::on_dewispMaximumIterationsEditBox_textEdited(const QString & /* arg1 */)
{
  updateCES_DewispParameters();
}

void CorticalExtractionSequenceDialog::on_smoothingIterationsEditBox_textEdited(const QString & /* arg1 */)
{
  updateCES_ICSParameters();
}

void CorticalExtractionSequenceDialog::on_smoothingConstantEditBox_textEdited(const QString & /* arg1 */)
{
  updateCES_ICSParameters();
}

void CorticalExtractionSequenceDialog::on_curvatureWeightEditBox_textEdited(const QString & /* arg1 */)
{
  updateCES_ICSParameters();
}

void CorticalExtractionSequenceDialog::on_pialPresmoothIterationsEditBox_textEdited(const QString & /* arg1 */)
{
  updateCES_PialParameters();
}

void CorticalExtractionSequenceDialog::on_pialIterationsEditBox_textEdited(const QString & /* arg1 */)
{
  updateCES_PialParameters();
}

void CorticalExtractionSequenceDialog::on_pialCollisionDetectionRadiusEditBox_textEdited(const QString & /* arg1 */)
{
  updateCES_PialParameters();
}

void CorticalExtractionSequenceDialog::on_pialThicknessLimitEditBox_textEdited(const QString & /* arg1 */)
{
  updateCES_PialParameters();
}

void CorticalExtractionSequenceDialog::on_pialStepSizeEditBox_textEdited(const QString & /* arg1 */)
{
  updateCES_PialParameters();
}

void CorticalExtractionSequenceDialog::on_pialTissueThresholdEditBox_textEdited(const QString & /* arg1 */)
{
  updateCES_PialParameters();
}

void CorticalExtractionSequenceDialog::on_pialSmoothingConstantEditBox_textEdited(const QString & /* arg1 */)
{
  updateCES_PialParameters();
}

void CorticalExtractionSequenceDialog::on_pialRadialConstantEditBox_textEdited(const QString & /* arg1 */)
{
  updateCES_PialParameters();
}

void CorticalExtractionSequenceDialog::on_pialUpdateSurfaceIntervalEditBox_textEdited(const QString & /* arg1 */)
{
  updateCES_PialParameters();
}

void CorticalExtractionSequenceDialog::on_pialConstrainWithCerebrumMaskCheckBox_clicked()
{
  updateCES_PialParameters();
}


void CorticalExtractionSequenceDialog::on_surfaceRegistrationOnlyCheckBox_clicked()
{
  updateCES_SVRegParameters();
}

void CorticalExtractionSequenceDialog::on_refineSulcalCurvesCheckBox_clicked()
{
  updateCES_SVRegParameters();
}

void CorticalExtractionSequenceDialog::on_labeledAtlasComboBox_currentIndexChanged(int /*index*/)
{
  updateCES_SVRegParameters();
}

void CorticalExtractionSequenceDialog::on_extendLabelsCheckBox_clicked()
{
  updateCES_SVRegParameters();
}
