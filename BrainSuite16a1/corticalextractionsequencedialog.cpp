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

#include "corticalextractionsequencedialog.h"
#include "ui_corticalextractionsequencedialog.h"
#include <brainsuitesettings.h>
#include <toolmanager.h>
#include <brainsuitewindow.h>
#include <strutil.h>
#include <qfileinfo.h>
#include <qdir.h>
#include <skullstrippingdialog.h>
#include <nonuniformitycorrectiondialog.h>
#include <tissueclassificationdialog.h>
#include <cerebrumextractiondialog.h>
#include <cortexidentificationdialog.h>
#include <topologicalcorrectiondialog.h>
#include <dewispdialog.h>
#include <innercorticalsurfacedialog.h>
#include <pialsurfacedialog.h>
#include <splithemispheresdialog.h>
#include <corticalextractionsequencedialog.h>
#include <waitcursor.h>
#include <svregistrationdialog.h>
#include <corticalextractionsettings.h>
#include <qfiledialog.h>
#include <DS/timer.h>
#include <bseguitool.h>
#include <bfcguitool.h>
#include <pvcguitool.h>
#include <cerebroguitool.h>
#include <cortexmodelguitool.h>
#include <scrubmaskguitool.h>
#include <topologycorrectionguitool.h>
#include <dewispguitool.h>
#include <innercorticalsurfaceguitool.h>
#include <hemisplitguitool.h>
#include <pialsurfaceguitool.h>
#include <svregguitool.h>
#include <skullfinderguitool.h>

extern BSEGUITool bseGUITool;
SkullfinderGUITool skullfinderGUITool;
extern BFCGUITool bfcGUITool;
extern PVCGUITool pvcGUITool;
extern CerebroGUITool cerebroGUITool;
extern CortexModelGUITool cortexModelGUITool;
extern ScrubMaskGUITool scrubMaskGUITool;
extern TopologyCorrectionGUITool topologyCorrectionGUITool;
extern DewispGUITool dewispGUITool;
extern InnerCorticalSurfaceGUITool innerCorticalSurfaceGUITool;
extern HemisplitGUITool hemisplitGUITool;
extern PialSurfaceGUITool pialSurfaceGUITool;
extern SVRegGUITool svregGUITool;

CorticalExtractionSequenceDialog::CorticalExtractionSequenceDialog(BrainSuiteWindow *parent) :
  QDialog(parent),
  ui(new Ui::CorticalExtractionSequenceDialog), brainSuiteWindow(parent), currentStage(ExtractionStages::SkullStripping)
{
  cerebroGUITool.initialize(brainSuiteWindow);
  ui->setupUi(this);
  ui->tempPathPushButton->setVisible(false);
  ui->tempPathTextLabel->setVisible(false);
  ui->erosionSizeSpinbox->blockSignals(true);
  ui->costFunctionComboBox->blockSignals(true);
  ui->warpLevelSpinBox->blockSignals(true);
  for (int i=ExtractionStages::SkullStripping; i < ExtractionStages::Finished; i++)
  {
    ExtractionStages::Stages stage = static_cast<ExtractionStages::Stages>(i);
    ui->parameterPageComboBox->insertItem(stage,ExtractionStages::stageName(stage).c_str());
  }
  ui->parameterPageComboBox->insertItem(ExtractionStages::Finished,"");
  ui->erosionSizeSpinbox->setRange(1,4);
  ui->autosaveOutputCheckBox->setChecked(BrainSuiteSettings::autosaveCorticalExtraction);
  ui->costFunctionComboBox->insertItem(0,"standard deviation");
  ui->costFunctionComboBox->insertItem(1,"least squares");
  ui->costFunctionComboBox->insertItem(2,"least squares w/intensity rescaling");
  ui->warpLevelSpinBox->setRange(2,8);
  ui->labeledAtlasComboBox->blockSignals(true);
  ui->labeledAtlasComboBox->clear();
  int i=0;
  for (auto atlas=svregGUITool.svreg.svregManifest.atlases.begin(); atlas != svregGUITool.svreg.svregManifest.atlases.end(); atlas++)
  {
    ui->labeledAtlasComboBox->insertItem(i,atlas->name.c_str());
    i++;
  }
  highlightPresentStage();

  ui->skullStrippingCheckBox->setChecked(true); ui->skullStrippingCheckBox->repaint();
  ui->skullfinderCheckBox->setChecked(true); ui->skullfinderCheckBox->repaint();
  ui->biasFieldCorrectionCheckBox->setChecked(true); ui->biasFieldCorrectionCheckBox->repaint();
  ui->tissueClassificationCorrectionCheckBox->setChecked(true); ui->tissueClassificationCorrectionCheckBox->repaint();
  ui->cerebrumLabelingCheckBox->setChecked(true); ui->cerebrumLabelingCheckBox->repaint();
  ui->innerCortexMaskCheckBox->setChecked(true); ui->cerebrumLabelingCheckBox->repaint();
  ui->scrubMaskCheckBox->setChecked(true); ui->cerebrumLabelingCheckBox->repaint();
  ui->topologyCorrectionCheckBox->setChecked(true); ui->cerebrumLabelingCheckBox->repaint();
  ui->dewispCheckBox->setChecked(true); ui->cerebrumLabelingCheckBox->repaint();
  ui->innerCorticalSurfaceCheckBox->setChecked(true); ui->cerebrumLabelingCheckBox->repaint();
  ui->pialSurfaceCheckBox->setChecked(true); ui->cerebrumLabelingCheckBox->repaint();
  ui->hemisplitCheckBox->setChecked(true); ui->cerebrumLabelingCheckBox->repaint();
  ui->registerLabelBrainCheckBox->setChecked(false); ui->cerebrumLabelingCheckBox->repaint();
  ui->lowRangeButton->setVisible(false);
  ui->mediumRangeButton->setVisible(false);
  ui->highRangeButton->setVisible(false);
  ui->labeledAtlasComboBox->blockSignals(false);
  on_suggestOutputsPushButton_clicked();
  updateAutosaveText();
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
  updateProgress();
  ui->erosionSizeSpinbox->blockSignals(false);
  ui->costFunctionComboBox->blockSignals(false);
  ui->warpLevelSpinBox->blockSignals(false);
  ui->resetPushButton->setVisible(false);
  ui->previousStagePushButton->setVisible(false);
  ui->stepBackPushButton->setVisible(false);
}

void CorticalExtractionSequenceDialog::setProgress(const int value, const int maxValue)
{
  ui->progressBar->setRange(0,maxValue);
  ui->progressBar->setValue(value);
  ui->progressBar->repaint();
}

bool CorticalExtractionSequenceDialog::updateStepText(std::string s)
{
  ui->extractionStepLabel->setText(s.c_str());
  ui->extractionStepLabel->repaint();
  return true;
}
ExtractionStages::Stages CorticalExtractionSequenceDialog::lastCheckedStage()
{
  if (ui->registerLabelBrainCheckBox->isChecked()) return ExtractionStages::RegisterAndLabel ;
  if (ui->hemisplitCheckBox->isChecked()) return ExtractionStages::SplitHemispheres;
  if (ui->pialSurfaceCheckBox->isChecked()) return ExtractionStages::PialSurface;
  if (ui->innerCorticalSurfaceCheckBox->isChecked()) return ExtractionStages::InnerCorticalSurface;
  if (ui->dewispCheckBox->isChecked()) return ExtractionStages::WispFilter;
  if (ui->topologyCorrectionCheckBox->isChecked()) return ExtractionStages::TopologyCorrection;
  if (ui->scrubMaskCheckBox->isChecked()) return ExtractionStages::ScrubMask;
  if (ui->innerCortexMaskCheckBox->isChecked()) return ExtractionStages::InnerCorticalMask;
  if (ui->cerebrumLabelingCheckBox->isChecked()) return ExtractionStages::CerebrumLabeling;
  if (ui->tissueClassificationCorrectionCheckBox->isChecked()) return ExtractionStages::TissueClassification;
  if (ui->biasFieldCorrectionCheckBox->isChecked()) return ExtractionStages::BiasFieldCorrection;
  if (ui->skullfinderCheckBox->isChecked()) return ExtractionStages::SkullAndScalp;
  if (ui->skullStrippingCheckBox->isChecked()) return ExtractionStages::SkullStripping;
  return ExtractionStages::SkullStripping;
}

ExtractionStages::Stages CorticalExtractionSequenceDialog::nextStage(ExtractionStages::Stages stage)
{
  switch (stage)
  {
    case ExtractionStages::SkullStripping : if (ui->skullfinderCheckBox->isChecked()) return ExtractionStages::SkullAndScalp;
    case ExtractionStages::SkullAndScalp : if (ui->biasFieldCorrectionCheckBox->isChecked()) return ExtractionStages::BiasFieldCorrection;
    case ExtractionStages::BiasFieldCorrection : if (ui->tissueClassificationCorrectionCheckBox->isChecked()) return ExtractionStages::TissueClassification;
    case ExtractionStages::TissueClassification : if (ui->cerebrumLabelingCheckBox->isChecked()) return ExtractionStages::CerebrumLabeling;
    case ExtractionStages::CerebrumLabeling :  if (ui->innerCortexMaskCheckBox->isChecked()) return ExtractionStages::InnerCorticalMask;
    case ExtractionStages::InnerCorticalMask :  if (ui->scrubMaskCheckBox->isChecked()) return ExtractionStages::ScrubMask;
    case ExtractionStages::ScrubMask :  if (ui->topologyCorrectionCheckBox->isChecked()) return ExtractionStages::TopologyCorrection;
    case ExtractionStages::TopologyCorrection :  if (ui->dewispCheckBox->isChecked()) return ExtractionStages::WispFilter;
    case ExtractionStages::WispFilter :  if (ui->innerCorticalSurfaceCheckBox->isChecked()) return ExtractionStages::InnerCorticalSurface;
    case ExtractionStages::InnerCorticalSurface :  if (ui->pialSurfaceCheckBox->isChecked()) return ExtractionStages::PialSurface;
    case ExtractionStages::PialSurface :  if (ui->hemisplitCheckBox->isChecked()) return ExtractionStages::SplitHemispheres;
    case ExtractionStages::SplitHemispheres :  if (ui->registerLabelBrainCheckBox->isChecked()) return ExtractionStages::RegisterAndLabel;
    case ExtractionStages::RegisterAndLabel :  return ExtractionStages::Finished;
    case ExtractionStages::Finished :  return ExtractionStages::Finished;
    default :  break;
  }
  return ExtractionStages::Finished;
}

ExtractionStages::Stages CorticalExtractionSequenceDialog::presentStage(ExtractionStages::Stages stage)
{
  switch (stage)
  {
    case ExtractionStages::SkullStripping : if (ui->skullStrippingCheckBox->isChecked())  return ExtractionStages::SkullStripping;
    case ExtractionStages::SkullAndScalp : if (ui->skullfinderCheckBox->isChecked()) return ExtractionStages::SkullAndScalp;
    case ExtractionStages::BiasFieldCorrection : if (ui->biasFieldCorrectionCheckBox->isChecked()) return ExtractionStages::BiasFieldCorrection;
    case ExtractionStages::TissueClassification : if (ui->tissueClassificationCorrectionCheckBox->isChecked()) return ExtractionStages::TissueClassification;
    case ExtractionStages::CerebrumLabeling : if (ui->cerebrumLabelingCheckBox->isChecked()) return ExtractionStages::CerebrumLabeling;
    case ExtractionStages::InnerCorticalMask :  if (ui->innerCortexMaskCheckBox->isChecked()) return ExtractionStages::InnerCorticalMask;
    case ExtractionStages::ScrubMask :  if (ui->scrubMaskCheckBox->isChecked()) return ExtractionStages::ScrubMask;
    case ExtractionStages::TopologyCorrection :  if (ui->topologyCorrectionCheckBox->isChecked()) return ExtractionStages::TopologyCorrection;
    case ExtractionStages::WispFilter :  if (ui->dewispCheckBox->isChecked()) return ExtractionStages::WispFilter;
    case ExtractionStages::InnerCorticalSurface :  if (ui->innerCorticalSurfaceCheckBox->isChecked()) return ExtractionStages::InnerCorticalSurface;
    case ExtractionStages::PialSurface :  if (ui->pialSurfaceCheckBox->isChecked()) return ExtractionStages::PialSurface;
    case ExtractionStages::SplitHemispheres :  if (ui->hemisplitCheckBox->isChecked()) return ExtractionStages::SplitHemispheres;
    case ExtractionStages::RegisterAndLabel :  if (ui->registerLabelBrainCheckBox->isChecked()) return ExtractionStages::RegisterAndLabel;
    case ExtractionStages::Finished :  return ExtractionStages::Finished;
    default :  break;
  }
  return ExtractionStages::Finished;
}


void CorticalExtractionSequenceDialog::showCurrentTool()
{
  ui->parametersStackedWidget->setCurrentIndex(currentStage);
}

CorticalExtractionSequenceDialog::~CorticalExtractionSequenceDialog()
{
  delete ui;
}

void CorticalExtractionSequenceDialog::highlightNextStage()
{
  ExtractionStages::Stages localStage=presentStage(currentStage);
  highlightStage(localStage,false);
  ExtractionGUITool *extractionGUITool = extractionTool(localStage);
  if (extractionGUITool)
  {
    ui->extractionStageLabel->setText(ExtractionStages::stageName(localStage).c_str());
    ui->progressBar->setRange(0,extractionGUITool->nSteps());
    ui->progressBar->setValue(extractionGUITool->currentStep());
    if (!extractionGUITool->isFinished())
      ui->extractionStepLabel->setText((std::string("next step: ")+extractionGUITool->stepName()).c_str());
    else
      ui->extractionStepLabel->setText(extractionGUITool->stepName().c_str());
  }
}

void CorticalExtractionSequenceDialog::highlightPresentStage()
{
  highlightStage(currentStage,true);
}

void CorticalExtractionSequenceDialog::highlightStage(ExtractionStages::Stages stage, bool disable)
{
  QString boldStyle("QCheckBox { font: bold;}");
  QString normalStyle("QCheckBox { font: normal;}");
  ui->skullStrippingCheckBox->setStyleSheet(stage==ExtractionStages::SkullStripping ? boldStyle : normalStyle); ui->skullStrippingCheckBox->repaint();
  ui->skullfinderCheckBox->setStyleSheet(stage==ExtractionStages::SkullAndScalp ? boldStyle : normalStyle); ui->skullfinderCheckBox->repaint();
  ui->biasFieldCorrectionCheckBox->setStyleSheet(stage==ExtractionStages::BiasFieldCorrection ? boldStyle : normalStyle); ui->biasFieldCorrectionCheckBox->repaint();
  ui->tissueClassificationCorrectionCheckBox->setStyleSheet(stage==ExtractionStages::TissueClassification ? boldStyle : normalStyle); ui->tissueClassificationCorrectionCheckBox->repaint();
  ui->cerebrumLabelingCheckBox->setStyleSheet(stage==ExtractionStages::CerebrumLabeling ? boldStyle : normalStyle); ui->cerebrumLabelingCheckBox->repaint();
  ui->innerCortexMaskCheckBox->setStyleSheet(stage==ExtractionStages::InnerCorticalMask ? boldStyle : normalStyle); ui->innerCortexMaskCheckBox->repaint();
  ui->scrubMaskCheckBox->setStyleSheet(stage==ExtractionStages::ScrubMask ? boldStyle : normalStyle); ui->scrubMaskCheckBox->repaint();
  ui->topologyCorrectionCheckBox->setStyleSheet(stage==ExtractionStages::TopologyCorrection ? boldStyle : normalStyle); ui->topologyCorrectionCheckBox->repaint();
  ui->dewispCheckBox->setStyleSheet(stage==ExtractionStages::WispFilter ? boldStyle : normalStyle); ui->dewispCheckBox->repaint();
  ui->innerCorticalSurfaceCheckBox->setStyleSheet(stage==ExtractionStages::InnerCorticalSurface ? boldStyle : normalStyle); ui->innerCorticalSurfaceCheckBox->repaint();
  ui->pialSurfaceCheckBox->setStyleSheet(stage==ExtractionStages::PialSurface ? boldStyle : normalStyle); ui->pialSurfaceCheckBox->repaint();
  ui->hemisplitCheckBox->setStyleSheet(stage==ExtractionStages::SplitHemispheres ? boldStyle : normalStyle); ui->hemisplitCheckBox->repaint();
  ui->registerLabelBrainCheckBox->setStyleSheet(stage==ExtractionStages::RegisterAndLabel ? boldStyle : normalStyle); ui->registerLabelBrainCheckBox->repaint();

  if (disable)
  {
    ExtractionStages::Stages stopstage = stage;
    ui->skullStrippingCheckBox->setEnabled(stopstage<=ExtractionStages::SkullStripping); ui->skullStrippingCheckBox->repaint();
    ui->skullfinderCheckBox->setEnabled(stopstage<=ExtractionStages::SkullAndScalp); ui->skullfinderCheckBox->repaint();
    ui->biasFieldCorrectionCheckBox->setEnabled(stopstage<=ExtractionStages::BiasFieldCorrection); ui->biasFieldCorrectionCheckBox->repaint();
    ui->tissueClassificationCorrectionCheckBox->setEnabled(stopstage<=ExtractionStages::TissueClassification); ui->tissueClassificationCorrectionCheckBox->repaint();
    ui->cerebrumLabelingCheckBox->setEnabled(stopstage<=ExtractionStages::CerebrumLabeling); ui->cerebrumLabelingCheckBox->repaint();
    ui->innerCortexMaskCheckBox->setEnabled(stopstage<=ExtractionStages::InnerCorticalMask); ui->innerCortexMaskCheckBox->repaint();
    ui->scrubMaskCheckBox->setEnabled(stopstage<=ExtractionStages::ScrubMask); ui->scrubMaskCheckBox->repaint();
    ui->topologyCorrectionCheckBox->setEnabled(stopstage<=ExtractionStages::TopologyCorrection); ui->topologyCorrectionCheckBox->repaint();
    ui->dewispCheckBox->setEnabled(stopstage<=ExtractionStages::WispFilter); ui->dewispCheckBox->repaint();
    ui->innerCorticalSurfaceCheckBox->setEnabled(stopstage<=ExtractionStages::InnerCorticalSurface); ui->innerCorticalSurfaceCheckBox->repaint();
    ui->pialSurfaceCheckBox->setEnabled(stopstage<=ExtractionStages::PialSurface); ui->pialSurfaceCheckBox->repaint();
    ui->hemisplitCheckBox->setEnabled(stopstage<=ExtractionStages::SplitHemispheres); ui->hemisplitCheckBox->repaint();
    ui->registerLabelBrainCheckBox->setEnabled(stopstage<=ExtractionStages::RegisterAndLabel); ui->registerLabelBrainCheckBox->repaint();
  }
}

bool CorticalExtractionSequenceDialog::showNextTool(ExtractionStages::Stages nextStage)
{
  ui->parameterPageComboBox->setCurrentIndex(nextStage);
  return true;
}

bool CorticalExtractionSequenceDialog::updateCurrentStage()
{
  ExtractionStages::Stages nextStage=currentStage;
  switch (currentStage)
  {
    case ExtractionStages::SkullStripping : if (ui->skullStrippingCheckBox->isChecked()) { nextStage=ExtractionStages::SkullStripping; break; }
    case ExtractionStages::SkullAndScalp : if (ui->skullfinderCheckBox->isChecked()) { nextStage=ExtractionStages::SkullAndScalp; break; }
    case ExtractionStages::BiasFieldCorrection : if (ui->biasFieldCorrectionCheckBox->isChecked()) { nextStage=ExtractionStages::BiasFieldCorrection; break; }
    case ExtractionStages::TissueClassification : if (ui->tissueClassificationCorrectionCheckBox->isChecked()) { nextStage=ExtractionStages::TissueClassification; break; }
    case ExtractionStages::CerebrumLabeling : if (ui->cerebrumLabelingCheckBox->isChecked()) { nextStage=ExtractionStages::CerebrumLabeling; break; }
    case ExtractionStages::InnerCorticalMask :  if (ui->innerCortexMaskCheckBox->isChecked()) { nextStage=ExtractionStages::InnerCorticalMask; break; }
    case ExtractionStages::ScrubMask :  if (ui->scrubMaskCheckBox->isChecked()) { nextStage=ExtractionStages::ScrubMask; break; }
    case ExtractionStages::TopologyCorrection :  if (ui->topologyCorrectionCheckBox->isChecked()) { nextStage=ExtractionStages::TopologyCorrection; break; }
    case ExtractionStages::WispFilter :  if (ui->dewispCheckBox->isChecked()) { nextStage=ExtractionStages::WispFilter; break; }
    case ExtractionStages::InnerCorticalSurface :  if (ui->innerCorticalSurfaceCheckBox->isChecked()) { nextStage=ExtractionStages::InnerCorticalSurface; break; }
    case ExtractionStages::PialSurface :  if (ui->pialSurfaceCheckBox->isChecked()) { nextStage=ExtractionStages::PialSurface; break; }
    case ExtractionStages::SplitHemispheres :  if (ui->hemisplitCheckBox->isChecked()) { nextStage=ExtractionStages::SplitHemispheres; break; }
    case ExtractionStages::RegisterAndLabel : if (ui->registerLabelBrainCheckBox->isChecked()) { nextStage=ExtractionStages::RegisterAndLabel; break; }
    case ExtractionStages::Finished :  { nextStage=ExtractionStages:: Finished; break; }
  }
  if (nextStage!=currentStage) { currentStage = nextStage; return true; }
  return false;
}

ExtractionGUITool *CorticalExtractionSequenceDialog::currentExtractionTool()
{
  return extractionTool(currentStage);
}

ExtractionGUITool *CorticalExtractionSequenceDialog::extractionTool(ExtractionStages::Stages stage)
{
  switch (stage)
  {
    case ExtractionStages::SkullStripping : return &bseGUITool;
    case ExtractionStages::SkullAndScalp : return &skullfinderGUITool;
    case ExtractionStages::BiasFieldCorrection: return &bfcGUITool;
    case ExtractionStages::TissueClassification: return &pvcGUITool;
    case ExtractionStages::CerebrumLabeling: return &cerebroGUITool;
    case ExtractionStages::InnerCorticalMask: return &cortexModelGUITool;
    case ExtractionStages::ScrubMask: return &scrubMaskGUITool;
    case ExtractionStages::TopologyCorrection: return &topologyCorrectionGUITool;
    case ExtractionStages::WispFilter: return &dewispGUITool;
    case ExtractionStages::InnerCorticalSurface: return &innerCorticalSurfaceGUITool;
    case ExtractionStages::PialSurface: return &pialSurfaceGUITool;
    case ExtractionStages::SplitHemispheres: return &hemisplitGUITool;
    case ExtractionStages::RegisterAndLabel: return &svregGUITool;
    case ExtractionStages::Finished: return 0;
    default:
      break;
  }
  return 0;
}

void CorticalExtractionSequenceDialog::updateProgress(bool running)
{
  ui->progressBar->setVisible(true);
  ExtractionGUITool *extractionGUITool = currentExtractionTool();
  if (extractionGUITool)
  {
    ui->extractionStageLabel->setText(ExtractionStages::stageName(currentStage).c_str());
    ui->extractionStageLabel->repaint();
    ui->progressBar->setRange(0,extractionGUITool->nSteps());
    ui->progressBar->setValue(extractionGUITool->currentStep());
    if (running)
    {
      ui->extractionStepLabel->setText((std::string("running: ")+extractionGUITool->stepName()).c_str());
      ui->extractionStepLabel->repaint();
      ui->progressBar->repaint();
    }
    else
    {
      if (!extractionGUITool->isFinished())
        ui->extractionStepLabel->setText((std::string("next step: ")+extractionGUITool->stepName()).c_str());
      else
        ui->extractionStepLabel->setText(extractionGUITool->stepName().c_str());

    }
  }
  else
  {
    ui->progressBar->setRange(0,1);
    ui->extractionStageLabel->setText("Finished");
    ui->extractionStepLabel->setText("");
    ui->progressBar->setValue(1);
  }
}

bool CorticalExtractionSequenceDialog::runStep()
{
  if (updateCurrentStage()) {  }
  showCurrentTool();
  ui->parameterPageComboBox->setCurrentIndex(currentStage);
  highlightPresentStage();
  if (!brainSuiteWindow) return false;
  if (!brainSuiteWindow->brainSuiteDataManager.volume)
  {
    std::cerr<<"an image volume must be loaded."<<std::endl;
    return false;
  }
  bool state = false;
  bool toolFinished=false;
  ui->extractionStageLabel->setText(ExtractionStages::stageName(currentStage).c_str());
  ui->extractionStageLabel->repaint();

  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
  ExtractionGUITool *extractionTool = currentExtractionTool();
  if (extractionTool)
  {
    state = extractionTool->runStep(brainSuiteWindow,this);
    if (!state) toolFinished=extractionTool->isFinished();
  }
  if (toolFinished)
  {
    currentStage = nextStage(currentStage);
    ui->extractionStageLabel->setText(ExtractionStages::stageName(currentStage).c_str());
    ui->extractionStageLabel->repaint();
    ExtractionGUITool *nextTool = currentExtractionTool();
    if (nextTool)
    {
      ui->progressBar->setRange(0,nextTool->nSteps());
      ui->progressBar->setValue(nextTool->currentStep());
      updateStepText(nextTool->stepName());
    }
  }
  ui->parameterPageComboBox->setCurrentIndex(currentStage);
  highlightPresentStage();
  ui->extractionStageLabel->setText(ExtractionStages::stageName(currentStage).c_str());
  ui->extractionStageLabel->repaint();
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
  return state;
}

bool CorticalExtractionSequenceDialog::runStage()
{
  if (updateCurrentStage()) {  }
  if (currentStage==ExtractionStages::Finished) return false;
  ExtractionStages::Stages startStage = currentStage;
  Timer timer; timer.start();
  while (runStep());
  timer.stop();
  std::cout<<ExtractionStages::stageName(startStage)<<" took "<<timer.elapsed()<<std::endl;
  return (startStage != currentStage);
}

bool CorticalExtractionSequenceDialog::runAll()
{
  if (currentStage==ExtractionStages::Finished) return false;
  Timer timer;
  timer.start();
  ExtractionStages::Stages lastStage = currentStage;
  while (runStage())
  {
    if (lastStage == currentStage) break;
    lastStage = currentStage;
  }
  timer.stop();
  std::cout<<"sequence took "<<timer.elapsed()<<std::endl;
  return false;
}

void CorticalExtractionSequenceDialog::on_autosaveOutputCheckBox_clicked()
{
  BrainSuiteSettings::autosaveCorticalExtraction = ui->autosaveOutputCheckBox->isChecked();
  if (BrainSuiteSettings::autosaveCorticalExtraction && ui->filenamePrefixLineEdit->text().isEmpty() && ui->workingDirectoryLineEdit->text().isEmpty())
    on_suggestOutputsPushButton_clicked();
  else
    updateAutosaveText();
}

std::string stripExtensions(std::string fullPathname)
{
  std::string prefix = StrUtil::gzStrip(fullPathname);
  prefix = StrUtil::extStrip(prefix,"nii");
  prefix = StrUtil::extStrip(prefix,"img");
  prefix = StrUtil::extStrip(prefix,"hdr");
  return prefix;
}

std::string CorticalExtractionSequenceDialog::formAutosavePrefix(std::string filename)
{
  std::string autosavePrefix;
  if (!filename.empty())
  {
    QFileInfo qi(filename.c_str());
    std::string fileNamePrefix = qi.fileName().toStdString();
    QString prefix = stripExtensions(fileNamePrefix).c_str();
    QString path = QDir::toNativeSeparators(qi.canonicalPath());
    autosavePrefix = (path + "/"+ prefix).toStdString();
  }
  return autosavePrefix;
}

void CorticalExtractionSequenceDialog::on_suggestOutputsPushButton_clicked()
{
  suggestOutputs();
}

void CorticalExtractionSequenceDialog::suggestOutputs()
{
  if (brainSuiteWindow && brainSuiteWindow->brainSuiteDataManager.volume)
  {
    std::string filename=brainSuiteWindow->brainSuiteDataManager.volume->filename;
    if (!filename.empty())
    {
      QFileInfo qi(filename.c_str());
      std::string fileNamePrefix = qi.fileName().toStdString();
      QString prefix = stripExtensions(fileNamePrefix).c_str();
      ui->filenamePrefixLineEdit->setText(prefix);
      QString path = QDir::toNativeSeparators(qi.canonicalPath());
      BrainSuiteSettings::autosavePrefix = (path + "/"+ prefix).toStdString();
      ui->workingDirectoryLineEdit->setText(path);
      std::cout<<"Autosave Prefix is "<<BrainSuiteSettings::autosavePrefix<<std::endl;
      updateAutosaveText();
    }
    return;
  }
  ui->filenamePrefixLineEdit->setText("");
  ui->workingDirectoryLineEdit->setText("");
}

void CorticalExtractionSequenceDialog::on_runStepPushButton_clicked()
{
  WaitCursor wc(this);
  runStep();
}

void CorticalExtractionSequenceDialog::on_runStagePushButton_clicked()
{
  WaitCursor wc(this);
  runStage();
}

void CorticalExtractionSequenceDialog::on_runAllPushButton_clicked()
{
  WaitCursor wc(this);
  runAll();
}

void CorticalExtractionSequenceDialog::on_filenamePrefixLineEdit_textEdited(const QString &)
{
  updateAutosaveText();
}

void CorticalExtractionSequenceDialog::on_workingDirectoryLineEdit_textEdited(const QString &)
{
  updateAutosaveText();
}

bool CorticalExtractionSequenceDialog::autosaveValid()
{
  QString outputDir=ui->workingDirectoryLineEdit->text();
  QString prefix=ui->filenamePrefixLineEdit->text();
  if (outputDir.isEmpty()) return false;
  QFileInfo info(outputDir);
  if (info.exists() && info.isDir()) return ui->autosaveOutputCheckBox->isChecked();
  return false;
}

void CorticalExtractionSequenceDialog::updateAutosaveText()
{
  QString redStyle("QTextEdit { color: red; }");
  QString blackStyle("QTextEdit { color: normal;}");
  QString outputDir=ui->workingDirectoryLineEdit->text();
  QString prefix=ui->filenamePrefixLineEdit->text();
  bool valid=false;
  if (outputDir.isEmpty()==false)
  {
    QFileInfo info(outputDir);
    if (info.exists()==false || info.isDir()==false)
    {
      ui->autosaveStatus->setText(outputDir + " is not a valid directory.");
      ui->autosaveStatus->setStyleSheet(redStyle);
    }
    else
    {
      ui->autosaveStatus->setStyleSheet(blackStyle);
      if (ui->autosaveOutputCheckBox->isChecked())
      {
        BrainSuiteSettings::autosavePrefix = QDir::toNativeSeparators(QDir::cleanPath(outputDir + "/" + prefix)).toStdString();
        ui->autosaveStatus->setText(("Extraction results will be saved to\n" + BrainSuiteSettings::autosavePrefix + ".*").c_str());
        valid=true;
      }
      else
        ui->autosaveStatus->setText("Output will not be saved automatically.");
    }
  }
  else
  {
    if (ui->autosaveOutputCheckBox->isChecked())
    {
      ui->autosaveStatus->setStyleSheet(redStyle);
      ui->autosaveStatus->setText("No directory specified -- output will not be saved automatically.");
    }
    else
    {
      ui->autosaveStatus->setStyleSheet(blackStyle);
      ui->autosaveStatus->setText("Output will not be saved automatically.");
    }
  }
}

void CorticalExtractionSequenceDialog::on_workingDirectoryPushButton_clicked()
{
  QString outputDir=ui->workingDirectoryLineEdit->text();
  if (outputDir.isEmpty())
  {
    if (brainSuiteWindow && brainSuiteWindow->brainSuiteDataManager.volume)
    {
      std::string filename=brainSuiteWindow->brainSuiteDataManager.volume->filename;
      if (!filename.empty())
      {
        QFileInfo qi(filename.c_str());
        outputDir = qi.canonicalPath();
      }
    }
  }
  if (outputDir.isEmpty()) outputDir=QDir::homePath();
  if (brainSuiteWindow)
  {
    std::string directory=brainSuiteWindow->getDirectory(outputDir.toStdString(),"Select an output directory for the cortical extraction sequence");
    if (directory.empty()==false)
    {
      QString s=QDir::toNativeSeparators(QDir::cleanPath(directory.c_str()));
      ui->workingDirectoryLineEdit->setText(s);
      updateAutosaveText();
    }
  }
  return;
}

void CorticalExtractionSequenceDialog::on_pialLoadCustomMaskButton_clicked()
{
  pialSurfaceGUITool.loadCustomCerebrumMask(brainSuiteWindow);
}

void CorticalExtractionSequenceDialog::on_skullStrippingCheckBox_clicked()
{
  highlightNextStage();
}

void CorticalExtractionSequenceDialog::on_skullfinderCheckBox_clicked()
{
  highlightNextStage();
}

void CorticalExtractionSequenceDialog::on_biasFieldCorrectionCheckBox_clicked()
{
  highlightNextStage();
}

void CorticalExtractionSequenceDialog::on_tissueClassificationCorrectionCheckBox_clicked()
{
  highlightNextStage();
}

void CorticalExtractionSequenceDialog::on_cerebrumLabelingCheckBox_clicked()
{
  highlightNextStage();
}

void CorticalExtractionSequenceDialog::on_innerCortexMaskCheckBox_clicked()
{
  highlightNextStage();
}

void CorticalExtractionSequenceDialog::on_scrubMaskCheckBox_clicked()
{
  highlightNextStage();
}

void CorticalExtractionSequenceDialog::on_topologyCorrectionCheckBox_clicked()
{
  highlightNextStage();
}

void CorticalExtractionSequenceDialog::on_dewispCheckBox_clicked()
{
  highlightNextStage();
}

void CorticalExtractionSequenceDialog::on_innerCorticalSurfaceCheckBox_clicked()
{
  highlightNextStage();
}

void CorticalExtractionSequenceDialog::on_pialSurfaceCheckBox_clicked()
{
  highlightNextStage();
}

void CorticalExtractionSequenceDialog::on_hemisplitCheckBox_clicked()
{
  highlightNextStage();
}

void CorticalExtractionSequenceDialog::on_registerLabelBrainCheckBox_clicked()
{
  highlightNextStage();
}


void CorticalExtractionSequenceDialog::on_applyCortexThresholdPushButton_clicked()
{
  cortexModelGUITool.stepBack(brainSuiteWindow);
  cortexModelGUITool.runStep(brainSuiteWindow,this);
}
