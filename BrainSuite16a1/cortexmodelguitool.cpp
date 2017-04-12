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

#include <brainsuiteqtsettings.h>
#include <brainsuitewindow.h>
#include "cortexmodelguitool.h"
#include <corticalextractionsettings.h>
#include <brainsuitesettings.h>
#include <displaycode.h>
#include "cortexidentificationdialog.h"
#include "corticalextractionsequencedialog.h"
#include "qapplication.h"

CortexModelGUITool cortexModelGUITool;

CortexModelGUITool::CortexModelGUITool() : state(FindCortex)
{
}

int CortexModelGUITool::nSteps() const
{
  return Finished;
}

int CortexModelGUITool::currentStep() const
{
  return state;
}

bool CortexModelGUITool::runStep(BrainSuiteWindow *brainSuiteWindow, QWidget * /*caller*/)
{
  if (state == Finished) return false;
  if (!brainSuiteWindow) { std::cerr<<"dialog is disconnected."<<std::endl; return false; }
  if (!brainSuiteWindow->brainSuiteDataManager.volume)
  {
    std::cerr<<"no image volume is loaded "<<std::endl;
    return false;
  }
  cortexModeler.thresh = brainSuiteWindow->ces.ctx.tissueThreshold;
  CortexModeler::SurfaceType surfaceType = CortexModeler::Cerebrum;
  cortexModeler.findVentricles(surfaceType, brainSuiteWindow->corticalExtractionData.vPVCFractions, cortexModeler.vVentricles,
                               brainSuiteWindow->corticalExtractionData.vHemiLabels, 1.5f /* ventricleThreshold */);
  bool flag = cortexModeler.whiteMatterFromFractions(surfaceType,
                               brainSuiteWindow->corticalExtractionData.vPVCFractions,brainSuiteWindow->corticalExtractionData.vCortexInitMask,
                               brainSuiteWindow->corticalExtractionData.vHemiLabels,
                               brainSuiteWindow->ces.ctx.includeAllSubcorticalAreas);
  if (flag)
  {
    state = Finished;
    brainSuiteWindow->brainSuiteDataManager.vMask.copy(brainSuiteWindow->corticalExtractionData.vCortexInitMask);
    if (brainSuiteWindow->toolManager.autosaveValid())
    {
      std::string prefix = BrainSuiteSettings::autosavePrefix;
      if (!prefix.empty())
      {
        brainSuiteWindow->corticalExtractionData.vCortexInitMask.write(prefix + ".init.cortex.mask.nii.gz");
      }
    }
  }
  else
  {
    std::cerr<<cortexModeler.errorString;
  }
  updateProgress(brainSuiteWindow,state,Finished);
  brainSuiteWindow->updateView(DisplayCode::MaskOn|DisplayCode::Dirty);
  return flag;
}

bool CortexModelGUITool::stepBack(BrainSuiteWindow * /* brainSuiteWindow */)
{
  state=FindCortex;
  return false;
}

std::string CortexModelGUITool::stepName() const
{
  return (state==FindCortex) ? "identify cortex" : "finished";
}

std::string CortexModelGUITool::nextStepText() const
{
  return stepName();
}

bool CortexModelGUITool::isFinished() const
{
  return (state==Finished);
}

bool CortexModelGUITool::autosaveOutputs(BrainSuiteWindow * /* brainSuiteWindow */)
{
  return false;
}

void CortexModelGUITool::updateProgress(BrainSuiteWindow *brainSuiteWindow, const int value, const int maxvalue)
{
  if (!brainSuiteWindow) return;
  if (brainSuiteWindow->toolManager.cortexIdentificationDialog)
    brainSuiteWindow->toolManager.cortexIdentificationDialog->setProgress(value,maxvalue);
  if (brainSuiteWindow->toolManager.corticalExtractionSequenceDialog)
    brainSuiteWindow->toolManager.corticalExtractionSequenceDialog->setProgress(value,maxvalue);
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}
