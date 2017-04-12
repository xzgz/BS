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

#include "skullfinderguitool.h"
#include <surflib.h>
#include <displaycode.h>
#include <brainsuitewindow.h>
#include <corticalextractionsettings.h>
#include <corticalextractionsequencedialog.h>
#include <surfacealpha.h>
#include <brainsuitesettings.h>
#include <colorops.h>
#include <skullandscalpdialog.h>
#include <qapplication.h>
#include <marchingcubes.h>
#include <surflib.h>

void status(std::string s);
bool writeOrFail(SILT::Surface &surface, std::string ofname);
bool writeOrFail(SILT::Surface *surface, std::string ofname);
bool writeOrFail(Vol3DBase *volume, std::string ofname);

void copyScalpLabels(Vol3DBase *&vLabelDst, Vol3D<uint8> &vLabelSrc)
{
  Vol3D<uint8> *labelVolume=0;
  if (vLabelDst)
  {
    if (vLabelDst->typeID()==SILT::Uint8)
    {
      labelVolume = (Vol3D<uint8> *)vLabelDst;
    }
  }
  if (!labelVolume)
  {
    delete vLabelDst;
    labelVolume = new Vol3D<uint8>;
    vLabelDst = labelVolume;
  }
  labelVolume->copy(vLabelSrc);
}

void transferLabels(Vol3DBase *&label, Vol3D<uint8> &vMask, int labelID)
{
  Vol3D<uint8> *labelVolume=0;
  if (label)
  {
    if (label->typeID()==SILT::Uint8)
    {
      labelVolume = (Vol3D<uint8> *)label;
    }
  }
  if (!labelVolume)
  {
    delete label;
    labelVolume = new Vol3D<uint8>;
    label = labelVolume;
  }
  if (labelVolume->isCompatible(vMask)==false)
  {
    labelVolume->makeCompatible(vMask);
    labelVolume->set(0);
  }
  uint8 *labels = labelVolume->start();
  uint8 *mask = vMask.start();
  const int ds = vMask.size();
  for (int i=0;i<ds;i++)
    if (mask[i]) labels[i] = labelID;
}

inline DSPoint rgb2pt(const uint32 a)
{
  unsigned char *p = (unsigned char *)&a;
  return DSPoint(p[2]/255.0f,p[1]/255.0f,p[0]/255.0f);
}

bool tessellateMask(SurfaceAlpha *&surface, Vol3D<uint8> &vMask, std::string name, uint32 colorRef)
{
  MarchingCubes tessellator;
  const int nIter = 10;
  if (!surface)
  {
    surface = new SurfaceAlpha;
  }
  tessellator.cubes(vMask,*surface);
  SurfLib::laplaceSmooth(*surface,nIter,0.5f);
  surface->filename = name;
  surface->solidColor = rgb2pt(colorRef);
  surface->useSolidColor = true;
  return true;
}

SkullfinderGUITool::SkullfinderGUITool()
{
}

int SkullfinderGUITool::nSteps() const
{
  return SkullAndScalpTool::Finished;
}

int SkullfinderGUITool::currentStep() const
{
  return skullAndScalpTool.state;
}

void SkullfinderGUITool::updateParameters(BrainSuiteWindow *brainSuiteWindow)
{
  if (brainSuiteWindow->toolManager.corticalExtractionSequenceDialog)
    brainSuiteWindow->toolManager.corticalExtractionSequenceDialog->updateSkullfinderParameters();
  if (brainSuiteWindow && brainSuiteWindow->toolManager.skullfinderDialog)
    brainSuiteWindow->toolManager.skullfinderDialog->updateDialogFromCES();
}

void SkullfinderGUITool::updateProgress(BrainSuiteWindow *brainSuiteWindow)
{
  if (brainSuiteWindow &&brainSuiteWindow->toolManager.skullfinderDialog)
    brainSuiteWindow->toolManager.skullfinderDialog->updateProgress();
  if (brainSuiteWindow->toolManager.corticalExtractionSequenceDialog)
    brainSuiteWindow->toolManager.corticalExtractionSequenceDialog->updateProgress();
}

void SkullfinderGUITool::setStepName(BrainSuiteWindow *brainSuiteWindow, std::string s)
{
  if (!brainSuiteWindow) return;
  if (brainSuiteWindow->toolManager.skullfinderDialog)
    brainSuiteWindow->toolManager.skullfinderDialog->updateStepText(s);
  if (brainSuiteWindow->toolManager.corticalExtractionSequenceDialog)
    brainSuiteWindow->toolManager.corticalExtractionSequenceDialog->updateStepText(s);
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

void SkullfinderGUITool::updateProgress(BrainSuiteWindow *brainSuiteWindow, const int value, const int maxvalue)
{
  if (!brainSuiteWindow) return;
  if (brainSuiteWindow->toolManager.skullfinderDialog)
    brainSuiteWindow->toolManager.skullfinderDialog->setProgress(value,maxvalue);
  if (brainSuiteWindow->toolManager.corticalExtractionSequenceDialog)
    brainSuiteWindow->toolManager.corticalExtractionSequenceDialog->setProgress(value,maxvalue);
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

bool SkullfinderGUITool::runStep(BrainSuiteWindow *brainSuiteWindow, QWidget * /*caller*/)
{
  bool retState = false;
  if (!brainSuiteWindow) { std::cerr<<"dialog is disconnected."<<std::endl; return false; }
  if (!brainSuiteWindow->brainSuiteDataManager.volume)
  {
    std::cerr<<"no image volume is loaded "<<std::endl;
    return false;
  }
  Vol3DBase *volume = brainSuiteWindow->brainSuiteDataManager.volume;
  Vol3D<uint8> &vMask = brainSuiteWindow->brainSuiteDataManager.vMask;
  if (!volume)
  {
    std::cerr<<"no volume loaded"<<std::endl;
    return false;
  }
  if (vMask.size()==0)
  {
    std::cerr<<"brain mask has not been computed or loaded"<<std::endl;
    return false;
  }
  if (!brainSuiteWindow->brainSuiteDataManager.vMask.isCompatible(volume))
  {
    std::cerr<<"brain mask volume does not match image volume"<<std::endl;
    return false;
  }
  updateProgress(brainSuiteWindow,currentStep(),nSteps());
  if (isFinished())
    setStepName(brainSuiteWindow,"Finished skull and scalp processing.");
  else
    setStepName(brainSuiteWindow,stepName());
  switch (skullAndScalpTool.state)
  {
    case SkullAndScalpTool::Initialize:
      {
        if (skullAndScalpTool.initialize(volume))
        {
          skullAndScalpTool.lowerSkullThreshold = 0;
          skullAndScalpTool.scalpThreshold = 0;
          skullAndScalpTool.upperSkullThreshold = 0;
          skullAndScalpTool.state = SkullAndScalpTool::FindScalp;
          skullAndScalpTool.vBrainMask.copy(vMask);
          if (brainSuiteWindow->ces.skullfinder.computeThresholds)
          {
          }
          else
          {
            skullAndScalpTool.lowerSkullThreshold = brainSuiteWindow->ces.skullfinder.skullThreshold;
            skullAndScalpTool.upperSkullThreshold = brainSuiteWindow->ces.skullfinder.scalpThreshold;
          }
          updateParameters(brainSuiteWindow);
          copyScalpLabels(brainSuiteWindow->brainSuiteDataManager.vLabel,skullAndScalpTool.vHeadLabels);
          retState = true;
        }
      }
      break;
    case SkullAndScalpTool::FindScalp:
      {
        if (!brainSuiteWindow->ces.skullfinder.computeThresholds)
        {
          skullAndScalpTool.lowerSkullThreshold = brainSuiteWindow->ces.skullfinder.skullThreshold;
          skullAndScalpTool.upperSkullThreshold = brainSuiteWindow->ces.skullfinder.scalpThreshold;
        }
        if (skullAndScalpTool.findScalp())
        {
          copyScalpLabels(brainSuiteWindow->brainSuiteDataManager.vLabel,skullAndScalpTool.vHeadLabels);
          brainSuiteWindow->updateView(DisplayCode::ShowLabels|DisplayCode::Dirty|DisplayCode::MaskOff);
          updateParameters(brainSuiteWindow);
          retState = true;
        }
      }
      break;
    case SkullAndScalpTool::FindOuterSkull:
      {
        if (skullAndScalpTool.findOuterSkull())
        {
          copyScalpLabels(brainSuiteWindow->brainSuiteDataManager.vLabel,skullAndScalpTool.vHeadLabels);
          brainSuiteWindow->updateView(DisplayCode::ShowLabels|DisplayCode::Dirty|DisplayCode::MaskOff);
          brainSuiteWindow->ces.skullfinder.skullThreshold=skullAndScalpTool.lowerSkullThreshold;
          brainSuiteWindow->ces.skullfinder.scalpThreshold=skullAndScalpTool.upperSkullThreshold;
          updateParameters(brainSuiteWindow);
          retState = true;
        }
      }
      break;
    case SkullAndScalpTool::FindInnerSkull:
      {
        if (skullAndScalpTool.findInnerSkull())
        {
          copyScalpLabels(brainSuiteWindow->brainSuiteDataManager.vLabel,skullAndScalpTool.vHeadLabels);
          brainSuiteWindow->updateView(DisplayCode::ShowLabels|DisplayCode::Dirty|DisplayCode::MaskOff);
          brainSuiteWindow->ces.skullfinder.skullThreshold=skullAndScalpTool.lowerSkullThreshold;
          brainSuiteWindow->ces.skullfinder.scalpThreshold=skullAndScalpTool.upperSkullThreshold;
          if (brainSuiteWindow->toolManager.autosaveValid())
          {
            std::string prefix = BrainSuiteSettings::autosavePrefix;
            if (!prefix.empty())
            {
              writeOrFail(&skullAndScalpTool.vHeadLabels,prefix + ".skull.label.nii.gz");
            }
          }
          updateParameters(brainSuiteWindow);
          retState = true;
        }
      }
      break;
    case SkullAndScalpTool::GenerateSurfaces:
      {
        tessellateMask(brainSuiteWindow->corticalExtractionData.brainSurface,skullAndScalpTool.vBrainMask,"[brain]",
                       brainSuiteWindow->brainSuiteDataManager.labelDescriptionSet.labelDetails[19].color);
        brainSuiteWindow->corticalExtractionData.brainSurface->translucent = false;
        brainSuiteWindow->corticalExtractionData.brainSurface->show = false;
        tessellateMask(brainSuiteWindow->corticalExtractionData.innerSkullSurface,skullAndScalpTool.vInnerSkull,"[inner skull]",
                       brainSuiteWindow->brainSuiteDataManager.labelDescriptionSet.labelDetails[18].color);
        brainSuiteWindow->corticalExtractionData.innerSkullSurface->translucent = false;
        brainSuiteWindow->corticalExtractionData.innerSkullSurface->show = false;
        tessellateMask(brainSuiteWindow->corticalExtractionData.outerSkullSurface,skullAndScalpTool.vOuterSkull,"[outer skull]",
                       brainSuiteWindow->brainSuiteDataManager.labelDescriptionSet.labelDetails[17].color);
        brainSuiteWindow->corticalExtractionData.outerSkullSurface->translucent = false;
        brainSuiteWindow->corticalExtractionData.outerSkullSurface->show = true;
        tessellateMask(brainSuiteWindow->corticalExtractionData.scalpSurface,skullAndScalpTool.vScalp,"[scalp]",
                       brainSuiteWindow->brainSuiteDataManager.labelDescriptionSet.labelDetails[16].color);
        brainSuiteWindow->corticalExtractionData.scalpSurface->translucent = true;
        brainSuiteWindow->corticalExtractionData.scalpSurface->show = true;
        if (brainSuiteWindow->toolManager.autosaveValid())
        {
          std::string prefix = BrainSuiteSettings::autosavePrefix;
          if (!prefix.empty())
          {
            writeOrFail(brainSuiteWindow->corticalExtractionData.brainSurface,prefix + ".brain.dfs");
            writeOrFail(brainSuiteWindow->corticalExtractionData.innerSkullSurface,prefix + ".inner_skull.dfs");
            writeOrFail(brainSuiteWindow->corticalExtractionData.outerSkullSurface,prefix + ".outer_skull.dfs");
            writeOrFail(brainSuiteWindow->corticalExtractionData.scalpSurface,prefix + ".scalp.dfs");
          }
        }
        brainSuiteWindow->addSurface(brainSuiteWindow->corticalExtractionData.brainSurface,false);
        brainSuiteWindow->addSurface(brainSuiteWindow->corticalExtractionData.innerSkullSurface,false);
        brainSuiteWindow->addSurface(brainSuiteWindow->corticalExtractionData.outerSkullSurface,false);
        brainSuiteWindow->addSurface(brainSuiteWindow->corticalExtractionData.scalpSurface,false);
        brainSuiteWindow->updateView(DisplayCode::MaskOn);
        skullAndScalpTool.state = SkullAndScalpTool::Finished;
      }
      retState = false;
      break;
    case SkullAndScalpTool::Finished:
      break;
  }
  updateProgress(brainSuiteWindow,currentStep(),nSteps());
  if (isFinished())
    setStepName(brainSuiteWindow,"finished skull and scalp processing.");
  else
    setStepName(brainSuiteWindow,"next step: "+stepName());

  return retState;
}

bool SkullfinderGUITool::removeSurfaces(BrainSuiteWindow *brainSuiteWindow)
{
  brainSuiteWindow->brainSuiteDataManager.removeSurface(brainSuiteWindow->corticalExtractionData.innerSkullSurface);
  brainSuiteWindow->brainSuiteDataManager.removeSurface(brainSuiteWindow->corticalExtractionData.outerSkullSurface);
  brainSuiteWindow->brainSuiteDataManager.removeSurface(brainSuiteWindow->corticalExtractionData.brainSurface);
  brainSuiteWindow->brainSuiteDataManager.removeSurface(brainSuiteWindow->corticalExtractionData.scalpSurface);
  brainSuiteWindow->updateSurfaceDisplayLists();
  brainSuiteWindow->updateSurfaceView();
  return true;
}

bool SkullfinderGUITool::stepBack(BrainSuiteWindow *brainSuiteWindow)
{
  switch (skullAndScalpTool.state)
  {
    case SkullAndScalpTool::Initialize:
      return false;
      break;
    case SkullAndScalpTool::FindScalp:
      skullAndScalpTool.state = SkullAndScalpTool::Initialize;
      removeSurfaces(brainSuiteWindow);
      break;
    case SkullAndScalpTool::FindOuterSkull:
      skullAndScalpTool.state = SkullAndScalpTool::FindScalp;
      removeSurfaces(brainSuiteWindow);
      break;
    case SkullAndScalpTool::FindInnerSkull:
      skullAndScalpTool.state = SkullAndScalpTool::FindOuterSkull;
      removeSurfaces(brainSuiteWindow);
      break;
    case SkullAndScalpTool::GenerateSurfaces:
      skullAndScalpTool.state = SkullAndScalpTool::FindInnerSkull;
      removeSurfaces(brainSuiteWindow);
      break;
    case SkullAndScalpTool::Finished:
      skullAndScalpTool.state = SkullAndScalpTool::GenerateSurfaces;
      removeSurfaces(brainSuiteWindow);
      break;
  }
  updateParameters(brainSuiteWindow);
  updateProgress(brainSuiteWindow);
  return false;
}

std::string SkullfinderGUITool::stepName() const
{
  switch (skullAndScalpTool.state)
  {
    case SkullAndScalpTool::Initialize: return "initialize";
    case SkullAndScalpTool::FindScalp: return "segment scalp";
    case SkullAndScalpTool::FindOuterSkull: return "segment outer skull";
    case SkullAndScalpTool::FindInnerSkull: return "segment inner skull";
    case SkullAndScalpTool::GenerateSurfaces:	return "generate surfaces";
    case SkullAndScalpTool::Finished:	return "finished";
    default: return "";
  }
}

std::string SkullfinderGUITool::nextStepText() const
{
  return stepName();
}

bool SkullfinderGUITool::isFinished() const
{
  return (skullAndScalpTool.state==SkullAndScalpTool::Finished);
}
