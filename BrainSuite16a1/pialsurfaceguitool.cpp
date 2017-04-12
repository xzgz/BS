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

#include "pialsurfaceguitool.h"
#include <corticalextractionsettings.h>
#include <brainsuitewindow.h>
#include <brainsuitedatamanager.h>
#include <corticalextractiondata.h>
#include <displaycode.h>
#include <pialsurfacethreaddialog.h>
#include <waitcursor.h>
#include <brainsuitesettings.h>
#include <pialsurfacetool.h>
#include <surflib.h>
#include <surfacealpha.h>
#include <colormap.h>
#include <pialsurfacedialog.h>
#include <corticalextractionsequencedialog.h>
#include <qapplication.h>

void copySurface(Surface &destinationSurface, const Surface *sourceSurface);
void copySurface(Surface &destinationSurface, const Surface &sourceSurface);

bool writeOrFail(SILT::Surface *surface, std::string ofname);
bool writeOrFail(SILT::Surface &surface, std::string ofname);

PialSurfaceGUITool::PialSurfaceGUITool() : state(Initialize)
{
}

void PialSurfaceGUITool::setStepName(BrainSuiteWindow *brainSuiteWindow, std::string s)
{
  if (!brainSuiteWindow) return;
  if (brainSuiteWindow->toolManager.pialSurfaceDialog)
    brainSuiteWindow->toolManager.pialSurfaceDialog->updateStepText(s);
  if (brainSuiteWindow->toolManager.corticalExtractionSequenceDialog)
    brainSuiteWindow->toolManager.corticalExtractionSequenceDialog->updateStepText(s);
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

void PialSurfaceGUITool::updateProgress(BrainSuiteWindow *brainSuiteWindow, const int value, const int maxvalue)
{
  if (!brainSuiteWindow) return;
  if (brainSuiteWindow->toolManager.pialSurfaceDialog)
    brainSuiteWindow->toolManager.pialSurfaceDialog->setProgress(value,maxvalue);
  if (brainSuiteWindow->toolManager.corticalExtractionSequenceDialog)
    brainSuiteWindow->toolManager.corticalExtractionSequenceDialog->setProgress(value,maxvalue);
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

int PialSurfaceGUITool::nSteps() const
{
  return Finished;
}

int PialSurfaceGUITool::currentStep() const
{
  return state;
}

bool PialSurfaceGUITool::startingIteration(BrainSuiteWindow *brainSuiteWindow, PialSurfaceTool *tool, int n)
{
  if (!brainSuiteWindow) return false;
  if (!tool) return false;
  if (n==0) return false;
  updateProgress(brainSuiteWindow,n,(int)tool->nIterations());

  if ((n % brainSuiteWindow->ces.pial.pialUpdateSurfaceInterval)==0)
  {
    std::ostringstream ostr;
    ostr<<"[Pial Surface Iteration "<<n+1<<"]";
    tool->sOut.filename = ostr.str();
    updateSurface(brainSuiteWindow,tool->sOut);
  }
  return true;
}

bool PialSurfaceGUITool::initialize(BrainSuiteWindow *brainSuiteWindow)
{
  if (!brainSuiteWindow) return false;
  if (brainSuiteWindow->ces.pial.pialConstrainWithCerebrumMask) // copy cerebrum mask if necessary
  {
    if (!brainSuiteWindow->brainSuiteDataManager.volume)
    {
      std::cerr<<"no volume loaded -- cannot start"<<std::endl;
      return false;
    }
    if (brainSuiteWindow->corticalExtractionData.vCerebrumMask.isCompatible(brainSuiteWindow->brainSuiteDataManager.volume))
    {
      brainSuiteWindow->brainSuiteDataManager.vMask.copy(brainSuiteWindow->corticalExtractionData.vCerebrumMask);
      brainSuiteWindow->updateView(DisplayCode::Image1Only|DisplayCode::MaskOn|DisplayCode::RedrawView|DisplayCode::Dirty);
      std::cout<<"copied cerebrum mask to displayed mask"<<std::endl;
    }
    else
    {
      std::cout<<"cerebrum mask has different dimensions than image volume. mask was not copied."<<std::endl;
    }
  }
  state=Expanding;
  return true;
}

bool PialSurfaceGUITool::runStep(BrainSuiteWindow *brainSuiteWindow, QWidget *caller)
{
  if (!brainSuiteWindow) { std::cerr<<"dialog is disconnected."<<std::endl; return false; }
  if (!brainSuiteWindow->brainSuiteDataManager.volume)
  {
    std::cerr<<"no image volume is loaded "<<std::endl;
    return false;
  }
  bool retstate = false;
  if (currentStep()==Initialize)
    updateProgress(brainSuiteWindow,0,nSteps());
  else
    updateProgress(brainSuiteWindow,currentStep(),nSteps());
  setStepName(brainSuiteWindow,stepName());
  switch (state)
  {
    case Initialize: retstate = initialize(brainSuiteWindow); break;
    case Expanding: retstate = computePialSurface(brainSuiteWindow, caller); break;
    case Finished: retstate = false; break;
  }
  if (currentStep()==Finished)
    updateProgress(brainSuiteWindow,currentStep(),nSteps());
  else
    updateProgress(brainSuiteWindow,0,nSteps());
  setStepName(brainSuiteWindow,stepName());
  return retstate;
}

bool PialSurfaceGUITool::stepBack(BrainSuiteWindow * /* brainSuiteWindow */)
{
  return false;
}

std::string PialSurfaceGUITool::stepName() const
{
  switch (state)
  {
    case Initialize: return "initialize";
    case Expanding: return "generate pial surface";
    case Finished: return "finished";
    default: return "";
  }
}

std::string PialSurfaceGUITool::nextStepText() const
{
  return stepName();
}

bool PialSurfaceGUITool::isFinished() const
{
  return (state==Finished);
}

bool colorThickness(SILT::Surface &surface1, SILT::Surface &surface2,
                    const float lower=0.0f, const float upper=5.0f, uint32 *LUT = ColorMap::jetLUT)
{
  if (!LUT) LUT = ColorMap::jetLUT;
  if (surface1.nv() != surface2.nv())
  {
    std::cerr<<"surfaces must have equal number of vertices."<<std::endl;
    return 1;
  }
  const size_t nv=surface1.nv();
  surface1.vertexAttributes.resize(nv);
  for (size_t i=0;i<nv;i++)
  {
    surface1.vertexAttributes[i] = (surface1.vertices[i] - surface2.vertices[i]).mag();
  }
  surface1.vertexColor.resize(nv);
  for (size_t i=0;i<nv;i++)
  {
    surface1.vertexColor[i] = ColorMap::colormap(LUT,surface1.vertexAttributes[i],lower,upper);
  }
  return true;
}

bool PialSurfaceGUITool::updateSurface(BrainSuiteWindow *brainSuiteWindow, SILT::Surface &surface)
{
  if (!brainSuiteWindow) return false;
  if (!brainSuiteWindow->corticalExtractionData.pialSurface)
    brainSuiteWindow->corticalExtractionData.pialSurface = new SurfaceAlpha;
  ::copySurface(*brainSuiteWindow->corticalExtractionData.pialSurface,surface);
  if (true)
    if (brainSuiteWindow->corticalExtractionData.innerCorticalSurface)
    {
      colorThickness(*brainSuiteWindow->corticalExtractionData.pialSurface,*brainSuiteWindow->corticalExtractionData.innerCorticalSurface);
    }
  brainSuiteWindow->corticalExtractionData.pialSurface->filename = surface.filename;
  brainSuiteWindow->brainSuiteDataManager.removeSurface(brainSuiteWindow->corticalExtractionData.pialSurface);
  brainSuiteWindow->addSurface(brainSuiteWindow->corticalExtractionData.pialSurface,false);
  brainSuiteWindow->updateView(DisplayCode::Image1Only|DisplayCode::MaskOn|DisplayCode::RedrawView|DisplayCode::Dirty);
  return true;
}

bool PialSurfaceGUITool::computePialSurface(BrainSuiteWindow *brainSuiteWindow, QWidget *caller)
{
  if (!brainSuiteWindow) return false;
  WaitCursor waitCursor(brainSuiteWindow);
  SurfaceAlpha *surface = brainSuiteWindow->corticalExtractionData.innerCorticalSurface;
  if (!surface)
  {
    surface = 0;
    if (!surface)
    {
      std::cerr<<"Inner cortical surface model was not computed."<<std::endl;
      return false;
    }
    else
    {
    }
  }
  if (brainSuiteWindow->corticalExtractionData.vPVCFractions.size()<=0)
  {
    std::cerr<<"This module requires a tissue fraction volume.\n";
    return false;
  }
  PialSurfaceTool pialSurfaceTool;
  pialSurfaceTool.vTissue.copy(brainSuiteWindow->corticalExtractionData.vPVCFractions);
  pialSurfaceTool.presmoothIterations = brainSuiteWindow->ces.pial.pialPresmoothIterations;
  pialSurfaceTool.estimateCorticalThickness = true;
  pialSurfaceTool.numIterations = brainSuiteWindow->ces.pial.pialIterations;
  pialSurfaceTool.searchRadius = brainSuiteWindow->ces.pial.pialCollisionDetectionRadius;
  pialSurfaceTool.maximumThickness = brainSuiteWindow->ces.pial.pialThicknessLimit;
  pialSurfaceTool.stepSize = brainSuiteWindow->ces.pial.pialStepSize;
  pialSurfaceTool.tissueThreshold = brainSuiteWindow->ces.pial.pialTissueThreshold;
  pialSurfaceTool.smoothingConstant = brainSuiteWindow->ces.pial.pialSmoothingConstant;
  pialSurfaceTool.radConstant = brainSuiteWindow->ces.pial.pialRadialConstant;
  pialSurfaceTool.tanConstant = 0.0f;
  bool useCbmMask = brainSuiteWindow->ces.pial.pialConstrainWithCerebrumMask;
  if (brainSuiteWindow->corticalExtractionData.vCerebrumMask.isCompatible(pialSurfaceTool.vTissue)&&useCbmMask)
  {
    pialSurfaceTool.vTissue.maskWith(brainSuiteWindow->corticalExtractionData.vCerebrumMask);
    std::cout<<"using cerebrum mask to constrain surface expansion.\n";
  }
  else
  {
    std::cout<<"not using cerebrum mask to constrain surface expansion.";
  }
  copySurface(pialSurfaceTool.sIn,surface);

  bool systemError = false;
  std::ostringstream errorStream;
  std::string errorLocation="pial surface expansion";

  ThreadDialogStatus::StatusCode returnState=ThreadDialogStatus::NotStarted;
  try
  {
    PialSurfaceThreadDialog pialSurfaceThreadDialog(brainSuiteWindow,caller);
    pialSurfaceThreadDialog.launch(&pialSurfaceTool);
    returnState=pialSurfaceThreadDialog.status();
  }
  catch (std::bad_alloc &whatsit)
  {
    systemError=true;
    errorStream<<"A memory allocation error ("<<whatsit.what()<<") occurred during "<<errorLocation<<'.';
  }
  catch (...)
  {
    systemError=true;
    errorStream<<"An unknown error occurred during pial surface expansion.";
  }
  if (systemError)
  {
    std::cerr <<errorStream.str()<< '\n';
  }
  if (returnState==ThreadDialogStatus::Succeeded)
  {
    std::cout<<"Completed pial surface generation."<<std::endl;
    pialSurfaceTool.sOut.filename = "[pial surface]";
    updateSurface(brainSuiteWindow,pialSurfaceTool.sOut);

    if (brainSuiteWindow->toolManager.autosaveValid())
    {
      std::string prefix = BrainSuiteSettings::autosavePrefix;
      if (!prefix.empty())
      {
        writeOrFail(brainSuiteWindow->corticalExtractionData.pialSurface,prefix + ".pial.cortex.dfs");
      }
    }
    pialSurfaceTool.collisionVolume.clear();
    pialSurfaceTool.collisionVolume.releaseMemory();
  }
  else
  {
    std::cerr<<"Pial surface generation was halted."<<std::endl;
    return false;
  }
  state = Finished;
  return true;
}

bool PialSurfaceGUITool::loadCustomCerebrumMask(BrainSuiteWindow *brainSuiteWindow)
{
  if (!brainSuiteWindow) return false;
  if (brainSuiteWindow->loadCustomCerebrumMask())
  {
    initialize(brainSuiteWindow);
    return true;
  }
  return false;
}
