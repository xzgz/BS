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

#include "dewispguitool.h"
#include <DS/timer.h>
#include <waitcursor.h>
#include <corticalextractionsettings.h>
#include <brainsuitesettings.h>
#include <brainsuitewindow.h>
#include <dewisptool.h>
#include <brainsuitedatamanager.h>
#include <dewispdialog.h>
#include <corticalextractionsequencedialog.h>
#include <qapplication.h>

DewispGUITool::DewispGUITool() : state(Dewisp)
{
}

void DewispGUITool::setStepName(BrainSuiteWindow *brainSuiteWindow, std::string s)
{
  if (!brainSuiteWindow) return;
  if (brainSuiteWindow->toolManager.dewispDialog)
    brainSuiteWindow->toolManager.dewispDialog->updateStepText(s);
  if (brainSuiteWindow->toolManager.corticalExtractionSequenceDialog)
    brainSuiteWindow->toolManager.corticalExtractionSequenceDialog->updateStepText(s);
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

void DewispGUITool::updateProgress(BrainSuiteWindow *brainSuiteWindow, const int value, const int maxvalue)
{
  if (!brainSuiteWindow) return;
  if (brainSuiteWindow->toolManager.dewispDialog)
    brainSuiteWindow->toolManager.dewispDialog->setProgress(value,maxvalue);
  if (brainSuiteWindow->toolManager.corticalExtractionSequenceDialog)
    brainSuiteWindow->toolManager.corticalExtractionSequenceDialog->setProgress(value,maxvalue);
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

int DewispGUITool::nSteps() const
{
  return DewispGUITool::Finished;
}

int DewispGUITool::currentStep() const
{
  return state;
}

bool DewispGUITool::runStep(BrainSuiteWindow *brainSuiteWindow, QWidget * /*caller*/)
{
  if (!brainSuiteWindow) { std::cerr<<"dialog is disconnected."<<std::endl; return false; }
  if (!brainSuiteWindow->brainSuiteDataManager.volume)
  {
    std::cerr<<"no image volume is loaded "<<std::endl;
    return false;
  }
  if (state == Finished) return false;
  WaitCursor waitCursor(brainSuiteWindow);
  DewispTool tool;
  const int maxIterations = brainSuiteWindow->ces.dewisp.maximumIterations;
  tool.sizeThreshold = brainSuiteWindow->ces.dewisp.threshold;
  Vol3D<uint8> vIn;
  vIn.copy(brainSuiteWindow->brainSuiteDataManager.vMask);
  int dVoxel=0;
  std::ostringstream ostr;
  Timer timer; timer.start();
  setStepName(brainSuiteWindow,"Performing dewisping");
  for (int i=0;i<maxIterations;i++)
  {
    updateProgress(brainSuiteWindow,i,maxIterations);
    int n = tool.loop(brainSuiteWindow->brainSuiteDataManager.vMask,vIn,1);
    if (n==dVoxel) break;
    dVoxel = n;
  }
  updateProgress(brainSuiteWindow,maxIterations,maxIterations);
  timer.stop();
  setStepName(brainSuiteWindow,"Finished dewisping.");
  ostr<<"Finished applying dewisp filter."<<'\n';
  ostr<<"Dewisping took "<<timer.elapsed()<<'\n';
  state = Finished;
  if (brainSuiteWindow->toolManager.autosaveValid())
  {
    std::string prefix = BrainSuiteSettings::autosavePrefix;
    if (!prefix.empty())
    {
      std::string mfname = prefix + ".cortex.dewisp.mask.nii.gz";
      if (brainSuiteWindow->brainSuiteDataManager.vMask.write(mfname))
        std::cout<<"wrote "<<mfname<<std::endl;
      else
        std::cerr<<"error writing "<<mfname<<std::endl;
    }
  }
  updateProgress(brainSuiteWindow,maxIterations,maxIterations);
  return false;
}

bool DewispGUITool::stepBack(BrainSuiteWindow * /* brainSuiteWindow */)
{
  return false;
}

std::string DewispGUITool::stepName() const
{
  switch (state)
  {
    case Dewisp: return "Removing wisps";
    case Finished: return "Finished";
    default: return "";
  }
}

std::string DewispGUITool::nextStepText() const
{
  return stepName();
}

bool DewispGUITool::isFinished() const
{
  return (state==Finished);
}
