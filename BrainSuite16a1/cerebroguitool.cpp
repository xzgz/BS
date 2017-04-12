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

#include <qdir.h>
#include <qapplication.h>
#include <brainsuitewindow.h>
#include <qfileinfo.h>
#include <corticalextractionsettings.h>
#include <DS/timer.h>
#include <waitcursor.h>
#include <brainsuitesettings.h>
#include <brainsuiteqtsettings.h>
#include <displaycode.h>
#include "cerebroguitool.h"
#include <cerebrumextractiondialog.h>
#include <corticalextractionsequencedialog.h>
#include <DS/getfilesize.h>

void CerebroGUITool::initialize(BrainSuiteWindow *brainSuiteWindow)
{
  static bool initd=false;
  if (!initd)
  {
    if (cerebrumLabeler.tempDirectoryBase.empty())
      cerebrumLabeler.tempDirectoryBase = QDir::tempPath().toStdString();
    std::string atlasPath = BrainSuiteQtSettings::qtinstalldir;
    if (atlasPath.empty()==false)
    {
      BrainSuiteSettings::defaultCerebrumAtlasBrain = brainSuiteWindow->ces.cbm.atlasFilename;
      BrainSuiteSettings::defaultCerebrumAtlasTemplate = brainSuiteWindow->ces.cbm.labelFilename;
      cerebrumLabeler.atlasBrain = brainSuiteWindow->ces.cbm.atlasFilename;
      cerebrumLabeler.atlasTemplate = brainSuiteWindow->ces.cbm.atlasFilename;
      initd=true;
    }
    else
      std::cerr<<"no atlas path"<<std::endl;
    cerebrumLabeler.cerebroHelper=BrainSuiteQtSettings::qtinstalldir+"/bin/cerebrohelper";
    cerebrumLabeler.cerebroHelper=QDir::toNativeSeparators(cerebrumLabeler.cerebroHelper.c_str()).toStdString();
#if _WIN32
    cerebrumLabeler.cerebroHelper+=".exe";
#endif
    if (!fileExists(cerebrumLabeler.cerebroHelper))
    {
      std::cerr<<"Could not find required file "<<cerebrumLabeler.cerebroHelper<<"\nPlease check your BrainSuite installation"<<std::endl;
    }
    cerebrumLabeler.initNames();
  }
}

int CerebroGUITool::nSteps() const
{
  return CerebrumLabelerQt::Finished;
}

int CerebroGUITool::currentStep() const
{
  return cerebrumLabeler.state;
}

void CerebroGUITool::setStepName(BrainSuiteWindow *brainSuiteWindow, std::string s)
{
  if (!brainSuiteWindow) return;
  if (brainSuiteWindow->toolManager.cerebrumExtractionDialog)
    brainSuiteWindow->toolManager.cerebrumExtractionDialog->updateStepName(s);
  if (brainSuiteWindow->toolManager.corticalExtractionSequenceDialog)
    brainSuiteWindow->toolManager.corticalExtractionSequenceDialog->updateStepText(s);
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

void CerebroGUITool::updateProgress(BrainSuiteWindow *brainSuiteWindow, const int value, const int maxvalue)
{
  if (!brainSuiteWindow) return;
  if (brainSuiteWindow->toolManager.cerebrumExtractionDialog)
    brainSuiteWindow->toolManager.cerebrumExtractionDialog->setProgress(value,maxvalue);
  if (brainSuiteWindow->toolManager.corticalExtractionSequenceDialog)
    brainSuiteWindow->toolManager.corticalExtractionSequenceDialog->setProgress(value,maxvalue);
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

bool copyWithOverwrite(std::string sourceFilename, std::string destinationFilename)
{
  QFile destinationFile(destinationFilename.c_str());
  QFile sourceFile(sourceFilename.c_str());
  if (destinationFile.exists())
  {
    if (!destinationFile.remove())
    {
      std::cerr<<"error: could not remove "<<destinationFilename<<" before copying "<<sourceFilename<<std::endl;
      return false;
    }
  }
  if (!sourceFile.exists())
  {
    std::cerr<<"error: "<<sourceFilename<<" does not exist"<<std::endl;
    return false;
  }
  return sourceFile.copy(destinationFilename.c_str());
}


bool CerebroGUITool::runStep(BrainSuiteWindow *brainSuiteWindow, QWidget * /*caller*/)
{
  if (!fileExists(cerebrumLabeler.cerebroHelper))
  {
    std::cerr<<"cerebrohelper not found (expected location: "<<cerebrumLabeler.cerebroHelper<<")"<<std::endl;
    return false;
  }
  int verbosity=1;
  if (!brainSuiteWindow) return false;
  if (!brainSuiteWindow->brainSuiteDataManager.volume)
  {
    std::cerr<<"A 3D image volume must be loaded before the cerebrum labeling tool can be applied."<<std::endl;
    return false;
  }
  bool killIt = false;
  cerebrumLabeler.useCentroids = brainSuiteWindow->ces.cbm.useCentroids;
  cerebrumLabeler.verbosity = brainSuiteWindow->ces.cbm.verbose ? 2 : 0;
  bool showAtlasOverlay = brainSuiteWindow->ces.cbm.showAtlasOverlay;
  Timer t; t.start();
  bool flag = false;
  cerebrumLabeler.atlasBrain = brainSuiteWindow->ces.cbm.atlasFilename;
  cerebrumLabeler.atlasTemplate = brainSuiteWindow->ces.cbm.labelFilename;
  cerebrumLabeler.tempDirectoryBase = brainSuiteWindow->ces.cbm.tempDirectory;
  std::string stepname=cerebrumLabeler.stepName();
  updateProgress(brainSuiteWindow,currentStep(),nSteps());
  if (!isFinished()) setStepName(brainSuiteWindow,"running: "+ stepname);
  WaitCursor waitCursor(brainSuiteWindow);
  switch (cerebrumLabeler.state)
  {
    case CerebrumLabelerQt::Initialize :
      flag = cerebrumLabeler.extractBrain(brainSuiteWindow->brainSuiteDataManager.volume,brainSuiteWindow->brainSuiteDataManager.vMask);
      if (flag && showAtlasOverlay)
      {
        brainSuiteWindow->loadOverlay2Volume(cerebrumLabeler.atlasBrain);
        brainSuiteWindow->updateView(DisplayCode::Image1Only|DisplayCode::MaskOn|DisplayCode::RedrawView|DisplayCode::Dirty);
      }
      break;
    case CerebrumLabelerQt::AlignLinear :
      {
        cerebrumLabeler.linearConvergence = brainSuiteWindow->ces.cbm.linearConvergence;
        cerebrumLabeler.costFunction = brainSuiteWindow->ces.cbm.costFunction;
        std::string ifname;
        flag = cerebrumLabeler.alignLinear(brainSuiteWindow->brainSuiteDataManager.volume,ifname);
        int signal=DisplayCode::Image1Only|DisplayCode::MaskOn|DisplayCode::RedrawView|DisplayCode::Dirty;
        if (flag && showAtlasOverlay)
        {
          if (!brainSuiteWindow->loadOverlay2Volume(ifname)) std::cerr<<"error loading "<<ifname<<std::endl;
          signal |= DisplayCode::ShowOverlay2;
        }
        brainSuiteWindow->updateView(signal);
      }
      break;
    case CerebrumLabelerQt::AlignWarp :
      {
        std::string ifname;
        cerebrumLabeler.warpConvergence = brainSuiteWindow->ces.cbm.warpConvergence;
        cerebrumLabeler.warpLevel = brainSuiteWindow->ces.cbm.warpLevel;
        flag = cerebrumLabeler.alignWarp(ifname);
        if (flag && showAtlasOverlay)
        {
          brainSuiteWindow->loadOverlay2Volume(ifname);
          brainSuiteWindow->updateView(DisplayCode::Image1Only|DisplayCode::MaskOn|DisplayCode::RedrawView|DisplayCode::ShowOverlay2|DisplayCode::Dirty);
        }
        else
        {
          brainSuiteWindow->updateView(DisplayCode::Image1Only|DisplayCode::MaskOn|DisplayCode::RedrawView|DisplayCode::Dirty);
        }
      }
      break;
    case CerebrumLabelerQt::ResliceWarp :
      flag = cerebrumLabeler.labelLobes( brainSuiteWindow->brainSuiteDataManager.volume, brainSuiteWindow->corticalExtractionData.vHemiLabels);
      brainSuiteWindow->corticalExtractionData.vHemiLabels.maskWith(brainSuiteWindow->brainSuiteDataManager.vMask);
      cerebrumLabeler.selectCerebrum(brainSuiteWindow->corticalExtractionData.vCerebrumMask,brainSuiteWindow->corticalExtractionData.vHemiLabels);
      brainSuiteWindow->brainSuiteDataManager.vMask.copy(brainSuiteWindow->corticalExtractionData.vCerebrumMask);
      if (brainSuiteWindow->toolManager.autosaveValid())
      {
        std::string prefix = BrainSuiteSettings::autosavePrefix;
        if (!prefix.empty())
        {
          std::string lfname = prefix + ".hemi.label.nii.gz";
          std::string mfname = prefix + ".cerebrum.mask.nii.gz";
          if (brainSuiteWindow->corticalExtractionData.vHemiLabels.write(lfname))
            std::cout<<"wrote "<<lfname<<std::endl;
          else
            std::cerr<<"error writing "<<lfname<<std::endl;
          if (brainSuiteWindow->corticalExtractionData.vCerebrumMask.write(mfname))
            std::cout<<"wrote "<<mfname<<std::endl;
          else
            std::cerr<<"error writing "<<mfname<<std::endl;
          if (!copyWithOverwrite(cerebrumLabeler.atlasToBrainAirFile,prefix+".air"))
          {
            std::cerr<<"error : could not copy "<<cerebrumLabeler.atlasToBrainAirFile<<" to "<<(prefix+".air")<<std::endl;
            flag=false;
          }
          if (!copyWithOverwrite(cerebrumLabeler.atlasToBrainWarpFile,prefix+".warp"))
          {
            std::cerr<<"error : could not copy "<<cerebrumLabeler.atlasToBrainWarpFile<<" to "<<(prefix+".warp")<<std::endl;
            flag=false;
          }
        }
      }
      brainSuiteWindow->updateView(DisplayCode::Image1Only|DisplayCode::MaskOn|DisplayCode::HideOverlay2|DisplayCode::RedrawView|DisplayCode::Dirty);
      break;
    case CerebrumLabelerQt::Finished :
      {
      }
      break;
    default:
      break;
  }
  updateProgress(brainSuiteWindow,currentStep(),nSteps());
  setStepName(brainSuiteWindow,nextStepText());
  t.stop();

  if (flag &&verbosity>1)
  {
    std::cout<<stepname<<" took "<<t.elapsed()<<'\n';
  }
  else
  {
    if (killIt)
    {
      std::cerr<<"Process killed by user"<<std::endl;
    }
  }
  return flag;
}

bool CerebroGUITool::stepBack(BrainSuiteWindow * /* brainSuiteWindow */)
{
  return false;
}

std::string CerebroGUITool::stepName() const
{
  return cerebrumLabeler.stepName();
}

std::string CerebroGUITool::nextStepText() const
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

bool CerebroGUITool::isFinished() const
{
  return (cerebrumLabeler.state==CerebrumLabelerQt::Finished);
}

bool CerebroGUITool::autosaveOutputs(BrainSuiteWindow * /* brainSuiteWindow */)
{
  return false;
}
