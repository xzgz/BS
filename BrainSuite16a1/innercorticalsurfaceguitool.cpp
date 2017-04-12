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

#include "innercorticalsurfaceguitool.h"
#include <DS/timer.h>
#include <waitcursor.h>
#include <corticalextractionsettings.h>
#include <brainsuitesettings.h>
#include <brainsuitewindow.h>
#include <displaycode.h>
#include <marchingcubes_t.h>
#include <surfacealpha.h>
#include <surflib.h>
#include <innercorticalsurfacedialog.h>
#include <corticalextractionsequencedialog.h>
#include <qapplication.h>

bool writeOrFail(SILT::Surface *surface, std::string ofname)
{
  if (!surface) return false;
  if (surface->write(ofname))
  {	std::cout<<"wrote "<<ofname<<'\n'; surface->filename = ofname; return true; }
  else
  {	std::cerr<<"error writing "<<ofname<<'\n'; return false; }
}

bool writeOrFail(SILT::Surface &surface, std::string ofname)
{
  if (surface.write(ofname))
  {	std::cout<<"wrote "<<ofname<<'\n'; surface.filename = ofname; return true; }
  else
  {	std::cerr<<"error writing "<<ofname<<'\n'; return false; }
}

bool writeOrFail(Vol3DBase *volume, std::string ofname)
{
  if (!volume) return false;
  if (volume->write(ofname))
  {	std::cout<<"wrote "<<ofname<<'\n'; return true; }
  else
  {	std::cerr<<"error writing "<<ofname<<'\n'; return false; }
}

InnerCorticalSurfaceGUITool::InnerCorticalSurfaceGUITool() : state(MakeSurface)
{
}

void InnerCorticalSurfaceGUITool::setStepName(BrainSuiteWindow *brainSuiteWindow, std::string s)
{
  if (!brainSuiteWindow) return;
  if (brainSuiteWindow->toolManager.innerCorticalSurfaceDialog)
    brainSuiteWindow->toolManager.innerCorticalSurfaceDialog->updateStepText(s);
  if (brainSuiteWindow->toolManager.corticalExtractionSequenceDialog)
    brainSuiteWindow->toolManager.corticalExtractionSequenceDialog->updateStepText(s);
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

void InnerCorticalSurfaceGUITool::updateProgress(BrainSuiteWindow *brainSuiteWindow, const int value, const int maxvalue)
{
  if (!brainSuiteWindow) return;
  if (brainSuiteWindow->toolManager.innerCorticalSurfaceDialog)
    brainSuiteWindow->toolManager.innerCorticalSurfaceDialog->setProgress(value,maxvalue);
  if (brainSuiteWindow->toolManager.corticalExtractionSequenceDialog)
    brainSuiteWindow->toolManager.corticalExtractionSequenceDialog->setProgress(value,maxvalue);
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

int InnerCorticalSurfaceGUITool::nSteps() const
{
  return InnerCorticalSurfaceGUITool::Finished;
}

int InnerCorticalSurfaceGUITool::currentStep() const
{
  return state;
}

bool InnerCorticalSurfaceGUITool::runStep(BrainSuiteWindow *brainSuiteWindow, QWidget * /*caller*/)
{
  if (!brainSuiteWindow) { std::cerr<<"dialog is disconnected."<<std::endl; return false; }
  if (!brainSuiteWindow->brainSuiteDataManager.volume)
  {
    std::cerr<<"no image volume is loaded "<<std::endl;
    return false;
  }
  if (state == Finished) return false;
  updateProgress(brainSuiteWindow,0,1);
  setStepName(brainSuiteWindow,"Generating inner cortical surface");
  MarchingCubes tessellator;
  WaitCursor waitCursor(brainSuiteWindow);
  SurfaceAlpha *innerCorticalSurface=new SurfaceAlpha;
  if (!innerCorticalSurface) return false;
  int n = brainSuiteWindow->ces.ics.smoothingIterations;
  float alpha = brainSuiteWindow->ces.ics.smoothingConstant;
  float c = brainSuiteWindow->ces.ics.curvatureWeight;
  tessellator.cubes(brainSuiteWindow->brainSuiteDataManager.vMask,*innerCorticalSurface);
  innerCorticalSurface->computeNormals();
  SurfLib::computeCurvature(*(innerCorticalSurface));
  SurfLib::laplaceSmoothCurvatureWeighted(*(innerCorticalSurface),n,alpha,c);
  innerCorticalSurface->computeNormals();
  innerCorticalSurface->filename = "[inner cortical surface]";
  if (brainSuiteWindow->corticalExtractionData.innerCorticalSurface)
  {
    delete brainSuiteWindow->corticalExtractionData.innerCorticalSurface;
  }
  brainSuiteWindow->corticalExtractionData.innerCorticalSurface = innerCorticalSurface;
  if (brainSuiteWindow->toolManager.autosaveValid())
  {
    std::string prefix = BrainSuiteSettings::autosavePrefix;
    if (!prefix.empty())
    {
      writeOrFail(brainSuiteWindow->corticalExtractionData.innerCorticalSurface,prefix + ".inner.cortex.dfs");
    }
  }
  brainSuiteWindow->hideAllSurfaces();
  brainSuiteWindow->addSurface(brainSuiteWindow->corticalExtractionData.innerCorticalSurface,false);
  brainSuiteWindow->updateView(DisplayCode::HideLabels|DisplayCode::Dirty);
  state = Finished;
  updateProgress(brainSuiteWindow,1,1);
  setStepName(brainSuiteWindow,"Generated inner cortical surface");
  return false;
}

bool InnerCorticalSurfaceGUITool::stepBack(BrainSuiteWindow * /* brainSuiteWindow */)
{
  return false;
}

std::string InnerCorticalSurfaceGUITool::stepName() const
{
  switch (state)
  {
    case MakeSurface: return "Generate inner cortical surface";
    case Finished: return "Finished";
    default: return "";
  }
}

std::string InnerCorticalSurfaceGUITool::nextStepText() const
{
  return stepName();
}

bool InnerCorticalSurfaceGUITool::isFinished() const
{
  return (state==Finished);
}
