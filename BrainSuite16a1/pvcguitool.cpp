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

#include "pvcguitool.h"
#include <corticalextractionsettings.h>
#include <brainsuitewindow.h>
#include <toolmanager.h>
#include <displaycode.h>
#include <brainsuitesettings.h>
#include <tissueclassificationdialog.h>
#include <corticalextractionsequencedialog.h>
#include <qapplication.h>
#include <waitcursor.h>

void PVCGUITool::setStepName(BrainSuiteWindow *brainSuiteWindow, std::string s)
{
  if (!brainSuiteWindow) return;
  if (brainSuiteWindow->toolManager.tissueClassificationDialog)
    brainSuiteWindow->toolManager.tissueClassificationDialog->updateStepText(s);
  if (brainSuiteWindow->toolManager.corticalExtractionSequenceDialog)
    brainSuiteWindow->toolManager.corticalExtractionSequenceDialog->updateStepText(s);
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

void PVCGUITool::updateProgress(BrainSuiteWindow *brainSuiteWindow, const int value, const int maxvalue)
{
  if (!brainSuiteWindow) return;
  if (brainSuiteWindow->toolManager.tissueClassificationDialog)
    brainSuiteWindow->toolManager.tissueClassificationDialog->setProgress(value,maxvalue);
  if (brainSuiteWindow->toolManager.corticalExtractionSequenceDialog)
    brainSuiteWindow->toolManager.corticalExtractionSequenceDialog->setProgress(value,maxvalue);
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

int PVCGUITool::nSteps() const
{
  return PVCTool::Finished;
}

int PVCGUITool::currentStep() const
{
  return pvcTool.state;
}

bool PVCGUITool::reset(BrainSuiteWindow *brainSuiteWindow)
{
  pvcTool.state = PVCTool::MLCassify;
  if (brainSuiteWindow)
  {
    setStepName(brainSuiteWindow,stepName());
    updateProgress(brainSuiteWindow,currentStep(),nSteps());
    brainSuiteWindow->updateView(DisplayCode::Image1Only|DisplayCode::RedrawView);
    brainSuiteWindow->repaintImages();
  }
  return true;
}

bool PVCGUITool::runStep(BrainSuiteWindow *brainSuiteWindow, QWidget * /*caller*/)
{
  if (!brainSuiteWindow) { std::cerr<<"dialog is disconnected."<<std::endl; return false; }
  if (!brainSuiteWindow->brainSuiteDataManager.volume)
  {
    std::cerr<<"An image volume must be loaded before tissue classification can be applied."<<std::endl;
    return false;
  }
  if (pvcTool.state == PVCTool::Finished) return false;
  updateProgress(brainSuiteWindow,currentStep(),nSteps());
  setStepName(brainSuiteWindow,"Running: "+stepName());
  pvcTool.settings.spatialPrior = brainSuiteWindow->ces.pvc.spatialPrior;
  bool retflag=false;
  WaitCursor cw(brainSuiteWindow);
  switch (pvcTool.state)
  {
    case PVCTool::MLCassify :
      if (pvcTool.stepForward(pvcTool.vl,brainSuiteWindow->brainSuiteDataManager.volume,brainSuiteWindow->brainSuiteDataManager.vMask))
      {
        pvcTool.vl.copyCast(brainSuiteWindow->brainSuiteDataManager.vLabel);
        brainSuiteWindow->updateView(DisplayCode::ShowLabels|DisplayCode::RedrawView);
        brainSuiteWindow->repaintImages();
        retflag=true;
      }
      break;
    case PVCTool::ICM :
      if (pvcTool.stepForward(pvcTool.vl,brainSuiteWindow->brainSuiteDataManager.volume,brainSuiteWindow->brainSuiteDataManager.vMask))
      {
        pvcTool.vl.copyCast(brainSuiteWindow->brainSuiteDataManager.vLabel);
        brainSuiteWindow->updateView(DisplayCode::ShowLabels|DisplayCode::RedrawView);
        brainSuiteWindow->repaintImages();
        pvcTool.computeFractions(brainSuiteWindow->corticalExtractionData.vPVCFractions, pvcTool.vb, pvcTool.vl, pvcTool.theta);
        retflag=true;
        if (brainSuiteWindow->toolManager.autosaveValid())
        {
          std::string prefix = BrainSuiteSettings::autosavePrefix;
          if (!prefix.empty())
          {
            std::string lfname = prefix + ".pvc.label.nii.gz";
            std::string ffname = prefix + ".pvc.frac.nii.gz";
            if (brainSuiteWindow->brainSuiteDataManager.vLabel->write(lfname))
              std::cout<<"wrote "<<lfname<<std::endl;
            else
              std::cerr<<"error writing "<<lfname<<std::endl;
            if (brainSuiteWindow->corticalExtractionData.vPVCFractions.write(ffname))
              std::cout<<"wrote "<<ffname<<std::endl;
            else
              std::cerr<<"error writing "<<ffname<<std::endl;
          }
        }
      }
      break;
    case PVCTool::Finished :
      break;
  }
  updateProgress(brainSuiteWindow,currentStep(),nSteps());
  setStepName(brainSuiteWindow,stepName());
  return true;
}

bool PVCGUITool::stepBack(BrainSuiteWindow * /* brainSuiteWindow */)
{
  return false;
}

std::string PVCGUITool::stepName() const
{
  switch (pvcTool.state)
  {
    case PVCTool::MLCassify : return "Initialization";
    case PVCTool::ICM : return "MAP classification";
    case PVCTool::Finished : return "Finished";
  }
  return "how did we get here?";
}

std::string PVCGUITool::nextStepText() const
{
  switch (pvcTool.state)
  {
    case PVCTool::MLCassify : return "Performing initialization";
    case PVCTool::ICM : return "Computing MAP classification";
    case PVCTool::Finished : return "Finished";
  }
  return "how did we get here?";
}

bool PVCGUITool::isFinished() const
{
  return (pvcTool.state==PVCTool::Finished);
}

bool PVCGUITool::autosaveOutputs(BrainSuiteWindow * /* brainSuiteWindow */)
{
  return false;
}
