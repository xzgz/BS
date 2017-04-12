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

#include "topologycorrectionguitool.h"
#include <DS/timer.h>
#include <waitcursor.h>
#include <corticalextractionsettings.h>
#include <topofixer.h>
#include <brainsuiteqtsettings.h>
#include <brainsuitewindow.h>
#include <displaycode.h>
#include <topologicalcorrectiondialog.h>
#include <corticalextractionsequencedialog.h>
#include <qapplication.h>
#include <brainsuitesettings.h>

TopologyCorrectionGUITool::TopologyCorrectionGUITool() : state(CorrectTopology)
{
}

int TopologyCorrectionGUITool::nSteps() const
{
  return TopologyCorrectionGUITool::Finished;
}

int TopologyCorrectionGUITool::currentStep() const
{
  return state;
}

void TopologyCorrectionGUITool::setStepName(BrainSuiteWindow *brainSuiteWindow, std::string s)
{
  if (!brainSuiteWindow) return;
  if (brainSuiteWindow->toolManager.topologicalCorrectionDialog)
    brainSuiteWindow->toolManager.topologicalCorrectionDialog->updateStepText(s);
  if (brainSuiteWindow->toolManager.corticalExtractionSequenceDialog)
    brainSuiteWindow->toolManager.corticalExtractionSequenceDialog->updateStepText(s);
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

void TopologyCorrectionGUITool::updateProgress(BrainSuiteWindow *brainSuiteWindow, const int value, const int maxvalue)
{
  if (!brainSuiteWindow) return;
  if (brainSuiteWindow->toolManager.topologicalCorrectionDialog)
    brainSuiteWindow->toolManager.topologicalCorrectionDialog->setProgress(value,maxvalue);
  if (brainSuiteWindow->toolManager.corticalExtractionSequenceDialog)
    brainSuiteWindow->toolManager.corticalExtractionSequenceDialog->setProgress(value,maxvalue);
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

bool TopologyCorrectionGUITool::runStep(BrainSuiteWindow *brainSuiteWindow, QWidget * /*caller*/)
{
  if (state == Finished) return false;
  WaitCursor waitCursor(brainSuiteWindow);
  int progmax=brainSuiteWindow->ces.tca.maximumCorrectionSize>100 ? 100 : brainSuiteWindow->ces.tca.maximumCorrectionSize;
  updateProgress(brainSuiteWindow,0,progmax);
  bool completed = false;
  {
    int verbosity=1;
    Vol3D<uint8> &vMask(brainSuiteWindow->brainSuiteDataManager.vMask);
    int minFix=brainSuiteWindow->ces.tca.minimumCorrectionSize;
    int maxFix=brainSuiteWindow->ces.tca.maximumCorrectionSize;
    if (minFix>maxFix) maxFix = minFix+1;
    TopoFixer topoFixer;
    topoFixer.verbosity=verbosity;
    topoFixer.initialize(vMask);
    if (verbosity>1) std::cout<<"Largest correction will be "<<maxFix<<std::endl;
    for (int i=1;i<maxFix;i++)
    {
      std::ostringstream ostr;
      ostr<<"Fixing topological defects of size "<<i;
      setStepName(brainSuiteWindow,ostr.str());
      completed = topoFixer.iterate(i,brainSuiteWindow->ces.tca.fillOffset,std::cout);
      updateProgress(brainSuiteWindow,i,progmax);
      if (completed) break;
    }
    completed = topoFixer.wrapUp(vMask,std::cout);
    if (!completed)
    {
      std::cerr<<"warning : topological defects remain in this object."<<std::endl;
    }
    else
    {
      if (verbosity>1)
        std::cout<<"finished removing topological handles."<<std::endl;
    }
  }
  updateProgress(brainSuiteWindow,progmax,progmax);
  setStepName(brainSuiteWindow,"Finished");
  brainSuiteWindow->updateView(DisplayCode::Dirty|DisplayCode::MaskOn);
  state = Finished;
  if (brainSuiteWindow->toolManager.autosaveValid())
  {
    std::string prefix = BrainSuiteSettings::autosavePrefix;
    if (!prefix.empty())
    {
      std::string mfname = prefix + ".cortex.tca.mask.nii.gz";
      if (brainSuiteWindow->brainSuiteDataManager.vMask.write(mfname))
        std::cout<<"wrote "<<mfname<<std::endl;
      else
        std::cout<<"error writing "<<mfname<<std::endl;
    }
  }
  return true;
}

bool TopologyCorrectionGUITool::stepBack(BrainSuiteWindow * /* brainSuiteWindow */)
{
  return false;
}

std::string TopologyCorrectionGUITool::stepName() const
{
  return "fix topology defects";
}

std::string TopologyCorrectionGUITool::nextStepText() const
{
  return stepName();
}

bool TopologyCorrectionGUITool::isFinished() const
{
  return (state==Finished);
}
