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

#include <brainsuitewindow.h>
#include "bseguitool.h"
#include <displaycode.h>
#include <corticalextractionsettings.h>
#include <skullstrippingdialog.h>
#include <corticalextractionsequencedialog.h>
#include <brainsuitesettings.h>
#include <brainsuiteqtsettings.h>
#include <qapplication.h>
#include <skullstrippingdialog.h>

int BSEGUITool::nSteps() const
{
  return BSETool::Finished;
}

int BSEGUITool::currentStep() const
{
  return bseTool.bseState;
}

uint32 BSEGUITool::currentViewState()
{
  switch (bseTool.bseState)
  {
    case BSETool::ADFilter    : return DisplayCode::Dirty|DisplayCode::Image1Only|DisplayCode::MaskOff;
    case BSETool::EdgeDetect  : return DisplayCode::Dirty|DisplayCode::ShowOverlay1|DisplayCode::MaskOff;
    case BSETool::FindBrain   : return DisplayCode::Dirty|DisplayCode::Image1Only|DisplayCode::MaskOn;
    case BSETool::FinishBrain : return DisplayCode::Dirty|DisplayCode::Image1Only|DisplayCode::MaskOn;
    case BSETool::Finished    : return DisplayCode::Dirty|DisplayCode::Image1Only|DisplayCode::MaskOn;
    default: break;
  }
  return DisplayCode::Dirty|DisplayCode::Image1Only|DisplayCode::MaskOff;
}

bool BSEGUITool::runStep(BrainSuiteWindow *brainSuiteWindow, QWidget * /*caller*/)
{
  if (!brainSuiteWindow) { std::cerr<<"dialog is disconnected."<<std::endl; return false; }
  if (!brainSuiteWindow->brainSuiteDataManager.volume)
  {
    std::cerr<<"no image volume is loaded "<<std::endl;
    return false;
  }
  brainSuiteWindow->corticalExtractionData.basefile = brainSuiteWindow->brainSuiteDataManager.volume->filename;
  bseTool.settings.diffusionConstant = brainSuiteWindow->ces.bse.diffusionConstant;
  bseTool.settings.diffusionIterations = brainSuiteWindow->ces.bse.diffusionIterations;
  bseTool.settings.edgeConstant = brainSuiteWindow->ces.bse.edgeConstant;
  bseTool.settings.erosionSize = brainSuiteWindow->ces.bse.erosionSize;
  bseTool.settings.dilateFinalMask = brainSuiteWindow->ces.bse.dilateFinalMask;
  bseTool.settings.removeBrainstem = brainSuiteWindow->ces.bse.trimSpinalCord;
  updateProgress(brainSuiteWindow,currentStep(),nSteps());
  if (!isFinished())
  {
    setStepName(brainSuiteWindow,"running " + stepName());
  }
  else
  {
    setStepName(brainSuiteWindow,"finished skullstripping.");
  }
  bool flag=bseTool.stepForward(brainSuiteWindow->brainSuiteDataManager.vMask,
                                brainSuiteWindow->brainSuiteDataManager.imageOverlay1Volume,
                                brainSuiteWindow->brainSuiteDataManager.volume);
  if (!isFinished())
  {
    setStepName(brainSuiteWindow,stepName());
  }
  else
  {
    setStepName(brainSuiteWindow,"finished skullstripping.");
  }
  updateProgress(brainSuiteWindow,currentStep(),nSteps());
  if (flag && isFinished())
    autosaveOutputs(brainSuiteWindow);
  brainSuiteWindow->updateView(currentViewState()|DisplayCode::RedrawView);
  return flag;
}

bool BSEGUITool::stepBack(BrainSuiteWindow *brainSuiteWindow)
{
  if (brainSuiteWindow && brainSuiteWindow->brainSuiteDataManager.volume)
  {
    if (bseTool.stepBack(brainSuiteWindow->brainSuiteDataManager.vMask,
                         brainSuiteWindow->brainSuiteDataManager.imageOverlay1Volume,
                         brainSuiteWindow->brainSuiteDataManager.volume))
    {
      brainSuiteWindow->updateView(currentViewState());
      setStepName(brainSuiteWindow,stepName());
      updateProgress(brainSuiteWindow,currentStep(),nSteps());
    }
  }
  return false;
}

std::string BSEGUITool::stepName() const
{
  switch (bseTool.bseState)
  {
    case BSETool::ADFilter    : return("anisotropic diffusion filter"); break;
    case BSETool::EdgeDetect  : return("edge detection"); break;
    case BSETool::FindBrain   : return("find initial brain region"); break;
    case BSETool::FinishBrain : return("enhance brain boundaries"); break;
    case BSETool::Finished    : return("finished skull stripping"); break;
    default: break;
  }
  return ("");
}

std::string BSEGUITool::nextStepText() const
{
  if (isFinished())
  {
    return "finished";
  }
  else
  {
    return std::string("next step: ") + stepName();
  }
}

bool BSEGUITool::isFinished() const
{
  return (bseTool.bseState==BSETool::Finished);
}

void BSEGUITool::setStepName(BrainSuiteWindow *brainSuiteWindow, std::string s)
{
  if (!brainSuiteWindow) return;
  if (brainSuiteWindow->toolManager.skullStrippingDialog)
    brainSuiteWindow->toolManager.skullStrippingDialog->updateStepText(s);
  if (brainSuiteWindow->toolManager.corticalExtractionSequenceDialog)
    brainSuiteWindow->toolManager.corticalExtractionSequenceDialog->updateStepText(s);
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

void BSEGUITool::updateProgress(BrainSuiteWindow *brainSuiteWindow, const int value, const int maxvalue)
{
  if (!brainSuiteWindow) return;
  if (brainSuiteWindow->toolManager.skullStrippingDialog)
    brainSuiteWindow->toolManager.skullStrippingDialog->setProgress(value,maxvalue);
  if (brainSuiteWindow->toolManager.corticalExtractionSequenceDialog)
    brainSuiteWindow->toolManager.corticalExtractionSequenceDialog->setProgress(value,maxvalue);
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

bool BSEGUITool::autosaveOutputs(BrainSuiteWindow *brainSuiteWindow)
{
  if (!brainSuiteWindow) return false;
  if (!brainSuiteWindow->toolManager.autosaveValid()) return false;
  if (!BrainSuiteSettings::autosaveCorticalExtraction) return false;
  std::string prefix = BrainSuiteSettings::autosavePrefix;
  if (!prefix.empty())
  {
    std::string ofname = prefix + ".mask.nii.gz";
    if (brainSuiteWindow->brainSuiteDataManager.vMask.write(ofname))
      std::cout<<"saved "<<ofname<<std::endl;
    else
      std::cerr<<"error writing "<<ofname<<std::endl;
    Vol3DBase *v = brainSuiteWindow->brainSuiteDataManager.volume->duplicate();
    if (v)
    {
      v->maskWith(brainSuiteWindow->brainSuiteDataManager.vMask);
      std::string ofname = prefix + ".bse.nii.gz";
      if (v->write(ofname))
        std::cout<<"saved "<<ofname<<std::endl;
      else
        std::cerr<<"error writing "<<ofname<<std::endl;
      delete v;
    }
    return true;
  }
  return false;
}
