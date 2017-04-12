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

#include "hemisplitguitool.h"
#include <brainsuitewindow.h>
#include <toolmanager.h>
#include <brainsplitter.h>
#include <DS/timer.h>
#include <waitcursor.h>
#include <surfacealpha.h>
#include <brainsuitesettings.h>
#include <svregistrationdialog.h>
#include <corticalextractionsequencedialog.h>
#include <qapplication.h>
#include <splithemispheresdialog.h>

bool writeOrFail(SILT::Surface *surface, std::string ofname);
bool writeOrFail(SILT::Surface &surface, std::string ofname);
bool copySurface(SurfaceAlpha *&destinationSurface, const SurfaceAlpha *sourceSurface)
{
  if (!destinationSurface) destinationSurface = new SurfaceAlpha;
  if (!destinationSurface) { std::cerr<<"could not allocate memory for new surface"<<std::endl; return false; }
  destinationSurface->vertices=sourceSurface->vertices;
  destinationSurface->triangles=sourceSurface->triangles;
  destinationSurface->vertexColor=sourceSurface->vertexColor;
  destinationSurface->computeConnectivity();
  destinationSurface->computeNormals();
  return true;
}

HemisplitGUITool::HemisplitGUITool() : state(SplitHemispheres)
{
}

void HemisplitGUITool::setStepName(BrainSuiteWindow *brainSuiteWindow, std::string s)
{
  if (!brainSuiteWindow) return;
  if (brainSuiteWindow->toolManager.splitHemispheresDialog)
    brainSuiteWindow->toolManager.splitHemispheresDialog->updateStepText(s);
  if (brainSuiteWindow->toolManager.corticalExtractionSequenceDialog)
    brainSuiteWindow->toolManager.corticalExtractionSequenceDialog->updateStepText(s);
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

void HemisplitGUITool::updateProgress(BrainSuiteWindow *brainSuiteWindow, const int value, const int maxvalue)
{
  if (!brainSuiteWindow) return;
  if (brainSuiteWindow->toolManager.splitHemispheresDialog)
    brainSuiteWindow->toolManager.splitHemispheresDialog->setProgress(value,maxvalue);
  if (brainSuiteWindow->toolManager.corticalExtractionSequenceDialog)
    brainSuiteWindow->toolManager.corticalExtractionSequenceDialog->setProgress(value,maxvalue);
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

int HemisplitGUITool::nSteps() const
{
  return HemisplitGUITool::Finished;
}

int HemisplitGUITool::currentStep() const
{
  return state;
}

bool HemisplitGUITool::runStep(BrainSuiteWindow *brainSuiteWindow, QWidget * /*caller*/)
{
  if (!brainSuiteWindow) { std::cerr<<"dialog is disconnected."<<std::endl; return false; }
  if (!brainSuiteWindow->brainSuiteDataManager.volume)
  {
    std::cerr<<"no image volume is loaded "<<std::endl;
    return false;
  }
  if (state == Finished) return false;
  Timer t;
  t.start();
  WaitCursor waitCursor(brainSuiteWindow);
  setStepName(brainSuiteWindow,"Splitting hemispheres");
  updateProgress(brainSuiteWindow,0,1);
  if (brainSuiteWindow->corticalExtractionData.leftInnerCorticalSurface)
  {
    brainSuiteWindow->brainSuiteDataManager.removeSurface(brainSuiteWindow->corticalExtractionData.leftInnerCorticalSurface);
    delete brainSuiteWindow->corticalExtractionData.leftInnerCorticalSurface;
    brainSuiteWindow->corticalExtractionData.leftInnerCorticalSurface = 0;
  }
  brainSuiteWindow->corticalExtractionData.leftInnerCorticalSurface = new SurfaceAlpha;
  if (brainSuiteWindow->corticalExtractionData.rightInnerCorticalSurface)
  {
    brainSuiteWindow->brainSuiteDataManager.removeSurface(brainSuiteWindow->corticalExtractionData.rightInnerCorticalSurface);
    delete brainSuiteWindow->corticalExtractionData.rightInnerCorticalSurface;
    brainSuiteWindow->corticalExtractionData.rightInnerCorticalSurface = 0;
  }
  brainSuiteWindow->corticalExtractionData.rightInnerCorticalSurface=new SurfaceAlpha;

  std::vector<int> leftMap;
  std::vector<int> rightMap;
  BrainSplitter::split(*brainSuiteWindow->corticalExtractionData.innerCorticalSurface,brainSuiteWindow->corticalExtractionData.vHemiLabels,*brainSuiteWindow->corticalExtractionData.leftInnerCorticalSurface,*brainSuiteWindow->corticalExtractionData.rightInnerCorticalSurface,leftMap,rightMap);
  if (!brainSuiteWindow->corticalExtractionData.leftInnerCorticalSurface||
      !brainSuiteWindow->corticalExtractionData.leftInnerCorticalSurface)
  {
    std::cerr<<"surface splitting failed."<<std::endl;
    //				updateStatus();
    return false;
  }
  brainSuiteWindow->hideAllSurfaces();

  brainSuiteWindow->corticalExtractionData.leftInnerCorticalSurface->show = true;
  brainSuiteWindow->corticalExtractionData.leftInnerCorticalSurface->solidColor = DSPoint(0,0.5,0.75);
  brainSuiteWindow->corticalExtractionData.leftInnerCorticalSurface->filename = "[left inner hemisphere]";
  brainSuiteWindow->corticalExtractionData.leftInnerCorticalSurface->useVertexColor = false;
  brainSuiteWindow->corticalExtractionData.leftInnerCorticalSurface->useSolidColor = true;

  brainSuiteWindow->corticalExtractionData.rightInnerCorticalSurface->show = true;
  brainSuiteWindow->corticalExtractionData.rightInnerCorticalSurface->solidColor = DSPoint(1,0.5,0.25);
  brainSuiteWindow->corticalExtractionData.rightInnerCorticalSurface->filename = "[right inner hemisphere]";
  brainSuiteWindow->corticalExtractionData.rightInnerCorticalSurface->useVertexColor = false;
  brainSuiteWindow->corticalExtractionData.rightInnerCorticalSurface->useSolidColor = true;
  //		std::cout<<"Split inner cortical surface."<<std::endl;
  //		updateStatus();

  if (brainSuiteWindow->toolManager.autosaveValid())
  {
    std::string prefix = BrainSuiteSettings::autosavePrefix;
    if (!prefix.empty())
    {
      writeOrFail(brainSuiteWindow->corticalExtractionData.rightInnerCorticalSurface,prefix + ".right.inner.cortex.dfs");
      writeOrFail(brainSuiteWindow->corticalExtractionData.leftInnerCorticalSurface,prefix + ".left.inner.cortex.dfs");
    }
  }

  brainSuiteWindow->addSurface(brainSuiteWindow->corticalExtractionData.leftInnerCorticalSurface,false);
  brainSuiteWindow->addSurface(brainSuiteWindow->corticalExtractionData.rightInnerCorticalSurface,false);

  if (brainSuiteWindow->corticalExtractionData.pialSurface)
  {
    if (brainSuiteWindow->corticalExtractionData.innerCorticalSurface->nv()==brainSuiteWindow->corticalExtractionData.pialSurface->nv())
    {
      ::copySurface(brainSuiteWindow->corticalExtractionData.rightPialSurface,brainSuiteWindow->corticalExtractionData.rightInnerCorticalSurface);
      ::copySurface(brainSuiteWindow->corticalExtractionData.leftPialSurface,brainSuiteWindow->corticalExtractionData.leftInnerCorticalSurface);
      if (brainSuiteWindow->corticalExtractionData.leftPialSurface && brainSuiteWindow->corticalExtractionData.rightPialSurface)
      {
        BrainSplitter::mapVertices(*brainSuiteWindow->corticalExtractionData.leftPialSurface,*brainSuiteWindow->corticalExtractionData.pialSurface,leftMap);
        BrainSplitter::mapVertices(*brainSuiteWindow->corticalExtractionData.rightPialSurface,*brainSuiteWindow->corticalExtractionData.pialSurface,rightMap);
        brainSuiteWindow->corticalExtractionData.leftPialSurface->show = true;
        brainSuiteWindow->corticalExtractionData.leftPialSurface->solidColor = DSPoint(0,0.5,0.75);
        brainSuiteWindow->corticalExtractionData.leftPialSurface->filename = "[left pial hemisphere]";
        brainSuiteWindow->corticalExtractionData.leftPialSurface->useVertexColor = false;
        brainSuiteWindow->corticalExtractionData.leftPialSurface->useSolidColor = true;
        brainSuiteWindow->corticalExtractionData.rightPialSurface->show = true;
        brainSuiteWindow->corticalExtractionData.rightPialSurface->solidColor = DSPoint(1,0.5,0.25);
        brainSuiteWindow->corticalExtractionData.rightPialSurface->filename = "[right pial hemisphere]";
        brainSuiteWindow->corticalExtractionData.rightPialSurface->useVertexColor = false;
        brainSuiteWindow->corticalExtractionData.rightPialSurface->useSolidColor = true;
        brainSuiteWindow->corticalExtractionData.rightPialSurface->computeNormals();
        brainSuiteWindow->corticalExtractionData.leftPialSurface->computeNormals();
        //								std::cout<<"Split pial surface."<<std::endl;
        //								updateStatus();
        if (brainSuiteWindow->toolManager.autosaveValid())
        {
          std::string prefix = BrainSuiteSettings::autosavePrefix;
          if (!prefix.empty())
          {
            writeOrFail(brainSuiteWindow->corticalExtractionData.rightPialSurface,prefix + ".right.pial.cortex.dfs");
            writeOrFail(brainSuiteWindow->corticalExtractionData.leftPialSurface,prefix + ".left.pial.cortex.dfs");
          }
        }
        brainSuiteWindow->addSurface(brainSuiteWindow->corticalExtractionData.leftPialSurface,false);
        brainSuiteWindow->addSurface(brainSuiteWindow->corticalExtractionData.rightPialSurface,false);
      }
    }
    else
    {
    }
  }
  t.stop();
  state=Finished;
  setStepName(brainSuiteWindow,"Finished splitting hemispheres");
  updateProgress(brainSuiteWindow,1,1);
  return true;
}

bool HemisplitGUITool::stepBack(BrainSuiteWindow * /* brainSuiteWindow */)
{
  return false;
}

std::string HemisplitGUITool::stepName() const
{
  switch (state)
  {
    case SplitHemispheres: return "Splitting hemispheres";
    case Finished: return "Finished";
  }
  return "error";
}

std::string HemisplitGUITool::nextStepText() const
{
  return stepName();
}

bool HemisplitGUITool::isFinished() const
{
  return (state==Finished);
}
