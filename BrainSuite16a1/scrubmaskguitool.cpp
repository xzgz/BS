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

#include <qapplication.h>
#include <brainsuitewindow.h>
#include "scrubmaskguitool.h"
#include <brainsuitesettings.h>
#include <corticalextractionsettings.h>
#include <maskscrubber.h>
#include <brainsuitedatamanager.h>
#include <DS/timer.h>
#include <displaycode.h>
#include <brainsuiteqtsettings.h>
#include <corticalextractionsequencedialog.h>
#include <scrubmaskdialog.h>

ScrubMaskGUITool::ScrubMaskGUITool() : state(Scrub)
{
}

void ScrubMaskGUITool::setStepName(BrainSuiteWindow *brainSuiteWindow, std::string s)
{
  if (!brainSuiteWindow) return;
  if (brainSuiteWindow->toolManager.scrubMaskDialog)
    brainSuiteWindow->toolManager.scrubMaskDialog->updateStepName(s);
  if (brainSuiteWindow->toolManager.corticalExtractionSequenceDialog)
    brainSuiteWindow->toolManager.corticalExtractionSequenceDialog->updateStepText(s);
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

void ScrubMaskGUITool::updateProgress(BrainSuiteWindow *brainSuiteWindow, const int value, const int maxvalue)
{
  if (!brainSuiteWindow) return;
  if (brainSuiteWindow->toolManager.scrubMaskDialog)
    brainSuiteWindow->toolManager.scrubMaskDialog->setProgress(value,maxvalue);
  if (brainSuiteWindow->toolManager.corticalExtractionSequenceDialog)
    brainSuiteWindow->toolManager.corticalExtractionSequenceDialog->setProgress(value,maxvalue);
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

int ScrubMaskGUITool::nSteps() const
{
  return ScrubMaskGUITool::Finished;
}

int ScrubMaskGUITool::currentStep() const
{
  return state;
}

bool ScrubMaskGUITool::runStep(BrainSuiteWindow *brainSuiteWindow, QWidget * /*caller*/)
{
  if (!brainSuiteWindow) return false;
  bool flag=false;
  int fgThresh = brainSuiteWindow->ces.scrubMask.foregroundThreshold;
  int bgThresh = brainSuiteWindow->ces.scrubMask.backgroundThreshold;
  int verbosity=1;
  MaskScrubber scrubber;
  Vol3D<uint8> vLabel;
  vLabel.copy(brainSuiteWindow->brainSuiteDataManager.vMask);
  const int ds = vLabel.size();
  for (int i=0;i<brainSuiteWindow->ces.scrubMask.nScrubIterations;i++)
  {
    updateProgress(brainSuiteWindow,i,brainSuiteWindow->ces.scrubMask.nScrubIterations);
    for (int j=0;j<ds;j++)
    {
      if (vLabel[j]>0 && vLabel[j]<128) vLabel[j]++;
      if (vLabel[j]<255 && vLabel[j]>128) vLabel[j]--;
    }
    int changeCount = 0;
    if (fgThresh>0) changeCount += (scrubber.filterFGL(brainSuiteWindow->brainSuiteDataManager.vMask,vLabel,fgThresh,verbosity));
    if (bgThresh>0) changeCount += (scrubber.filterBGL(brainSuiteWindow->brainSuiteDataManager.vMask,vLabel,bgThresh,verbosity));
    std::ostringstream o;
    o<<"Iteration: "<<i<<"\tchanged: "<<changeCount;
    setStepName(brainSuiteWindow,o.str());
    if (changeCount==0) break;
  }
  updateProgress(brainSuiteWindow,brainSuiteWindow->ces.scrubMask.nScrubIterations,brainSuiteWindow->ces.scrubMask.nScrubIterations);
  setStepName(brainSuiteWindow,"Finished!");
  brainSuiteWindow->updateView(DisplayCode::MaskOn|DisplayCode::Dirty);
  state = Finished;
  if (brainSuiteWindow->toolManager.autosaveValid())
  {
    std::string prefix = BrainSuiteSettings::autosavePrefix;
    if (!prefix.empty())
    {
      std::string mfname = prefix + ".cortex.scrubbed.mask.nii.gz";
      if (brainSuiteWindow->brainSuiteDataManager.vMask.write(mfname))
        std::cout<<"wrote "<<mfname<<std::endl;
      else
        std::cout<<"error writing "<<mfname<<std::endl;
    }
  }
  return flag;
}

bool ScrubMaskGUITool::stepBack(BrainSuiteWindow * /* brainSuiteWindow */)
{
  return false;
}

std::string ScrubMaskGUITool::stepName() const
{
  switch (state)
  {
    case Scrub: return "Scrubbing mask";
    case Finished: return "Finished";
    default: return "";
  }
}

std::string ScrubMaskGUITool::nextStepText() const
{
  return stepName();
}

bool ScrubMaskGUITool::isFinished() const
{
  return (state==Finished);
}
