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
#include <bfcguitool.h>
#include <displaycode.h>
#include <corticalextractionsettings.h>
#include <brainsuitesettings.h>
#include <waitcursor.h>
#include <bfcthreaddialog.h>
#include <nonuniformitycorrectiondialog.h>
#include <qapplication.h>
#include <corticalextractionsequencedialog.h>
#include <threaddialogstatus.h>
#include <biasfieldcorrectionthread.h>

int BFCGUITool::nSteps() const
{
  return BFCTool::Finished;
}

int BFCGUITool::currentStep() const
{
  return bfcTool.state;
}

bool BFCGUITool::runIterative(BrainSuiteWindow *brainSuiteWindow, QWidget *caller)
{
  if (!brainSuiteWindow || !brainSuiteWindow->brainSuiteDataManager.volume)
  {
    bfcTool.state = BFCTool::Init;
    return false;
  }
  WaitCursor waitCursor(brainSuiteWindow);
  updateProgress(brainSuiteWindow,0,(int)bfcTool.bfc.nIterations());
  bfcTool.bfc.iterative = true;
  try
  {
    ThreadDialogStatus::StatusCode returnState=ThreadDialogStatus::NotStarted;
    BFCThreadDialog bfcThreadDialog(brainSuiteWindow,caller);
    bfcThreadDialog.launch(this);
    returnState=bfcThreadDialog.status();
    if (returnState==ThreadDialogStatus::Succeeded)
    {
      Vol3DBase *correctedVolume = brainSuiteWindow->brainSuiteDataManager.volume->duplicate();
      if (!correctedVolume)
      {
        std::cerr<<"unable to create corrected image volume.";
        return false;
      }
      correctedVolume->filename = brainSuiteWindow->brainSuiteDataManager.volume->filename + "[bias corrected]";
      bfcThreadDialog.copyWorkingVolume(correctedVolume);
      if (finalizeOutput(brainSuiteWindow,correctedVolume))
      {
        bfcTool.state = BFCTool::Finished;
        return true;
      }
      else
        return false;
      brainSuiteWindow->updateImages();
    }
  }
  catch (std::bad_alloc &whatsit)
  {
    std::cerr << "memory allocation failure during bias correction: " << whatsit.what() << '\n';
  }
  catch (...)
  {
    std::cerr << "unknown error occurred during bias correction."<< '\n';
  }
  bfcTool.state = BFCTool::Init;
  return false;
}

void BFCGUITool::setStepName(BrainSuiteWindow *brainSuiteWindow, std::string s)
{
  if (!brainSuiteWindow) return;
  if (brainSuiteWindow->toolManager.nonuniformityCorrectionDialog)
    brainSuiteWindow->toolManager.nonuniformityCorrectionDialog->updateStepText(s);
  if (brainSuiteWindow->toolManager.corticalExtractionSequenceDialog)
    brainSuiteWindow->toolManager.corticalExtractionSequenceDialog->updateStepText(s);
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

void BFCGUITool::updateProgress(BrainSuiteWindow *brainSuiteWindow, const int value, const int maxvalue)
{
  if (!brainSuiteWindow) return;
  if (brainSuiteWindow->toolManager.nonuniformityCorrectionDialog)
    brainSuiteWindow->toolManager.nonuniformityCorrectionDialog->setProgress(value,maxvalue);
  if (brainSuiteWindow->toolManager.corticalExtractionSequenceDialog)
    brainSuiteWindow->toolManager.corticalExtractionSequenceDialog->setProgress(value,maxvalue);
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

bool BFCGUITool::finalizeOutput(BrainSuiteWindow *brainSuiteWindow, Vol3DBase *correctedVolume=0)
{
  if (!brainSuiteWindow) return false;
  if (!correctedVolume) return false;
  brainSuiteWindow->brainSuiteDataManager.addVolume(correctedVolume);
  if (correctedVolume)
    if (brainSuiteWindow->toolManager.autosaveValid())
    {
      std::string prefix = BrainSuiteSettings::autosavePrefix;
      if (!prefix.empty())
      {
        std::string ofname = prefix + ".bfc.nii.gz";
        if (correctedVolume->write(ofname))
          std::cout<<"wrote "<<ofname<<std::endl;
        else
          std::cerr<<"error writing "<<ofname<<std::endl;
      }
    }
  return true;
}

bool BFCGUITool::runStep(BrainSuiteWindow *brainSuiteWindow, QWidget *caller)
{
  if (!brainSuiteWindow) { std::cerr<<"dialog is disconnected."<<std::endl; return false; }
  if (!brainSuiteWindow->brainSuiteDataManager.volume)
  {
    std::cerr<<"no image volume is loaded "<<std::endl;
    return false;
  }
  if (brainSuiteWindow->ces.bfc.iterative)
  {
    if (bfcTool.state != BFCTool::Finished)
    {
      if (runIterative(brainSuiteWindow,caller))
      {
        updateProgress(brainSuiteWindow,4,4);
      }
      else
        return false;
    }
  }
  updateProgress(brainSuiteWindow,currentStep(),nSteps());
  if (!isFinished())
  {
    setStepName(brainSuiteWindow,"running " + stepName());
  }
  else
  {
    setStepName(brainSuiteWindow,"finished nonuniformity correction.");
  }
  bfcTool.bfc.histogramRadius=brainSuiteWindow->ces.bfc.histogramRadius;
  bfcTool.bfc.biasEstimateSpacing=brainSuiteWindow->ces.bfc.biasEstimateSampleSpacing;
  bfcTool.bfc.controlPointSpacing=brainSuiteWindow->ces.bfc.controlPointSpacing;
  bfcTool.bfc.splineLambda=brainSuiteWindow->ces.bfc.splineStiffness;
  bfcTool.bfc.roiType=(brainSuiteWindow->ces.bfc.roiType==CorticalExtractionSettings::BFC::Block) ?
        BiasFieldCorrector::Block : BiasFieldCorrector::Ellipsoid;
  bfcTool.bfc.biasRange=brainSuiteWindow->ces.bfc.biasRange;

  switch (brainSuiteWindow->brainSuiteDataManager.volume->typeID())
  {
    case SILT::Uint8 :
    case SILT::Sint16 :
    case SILT::Uint16 : break;
    case SILT::Float32 : break;
    default:
      std::cerr<<"BFC cannot process data of type "<<brainSuiteWindow->brainSuiteDataManager.volume->datatypeName()<<std::endl;
      return false;
  }

  bool retcode=false;
  switch (bfcTool.state)
  {
    case BFCTool::Init :
      retcode=bfcTool.initialize(brainSuiteWindow->brainSuiteDataManager.volume,
                                 brainSuiteWindow->brainSuiteDataManager.vMask);
      break;
    case BFCTool::ComputePoints :
      retcode = bfcTool.computePointwiseBiasEstimates();
      break;
    case BFCTool::ComputeSpline :
      retcode = bfcTool.estimateCorrectionField();
      break;
    case BFCTool::RemoveBias :
      {
        Vol3DBase *originalVolume = brainSuiteWindow->brainSuiteDataManager.volume;
        Vol3DBase *correctedVolume = originalVolume->duplicate();
        correctedVolume->filename = originalVolume->filename + "[bias corrected]";
        retcode = bfcTool.removeBias(correctedVolume,brainSuiteWindow->brainSuiteDataManager.vMask);
        if (retcode)
        {
          brainSuiteWindow->brainSuiteDataManager.addVolume(correctedVolume);
        }
        else
        {
          std::cerr<<"bfctool did not apply correction..."<<std::endl;
          delete correctedVolume;
          correctedVolume = 0;
        }
        if (correctedVolume)
          if (brainSuiteWindow->toolManager.autosaveValid())
          {
            std::string prefix = BrainSuiteSettings::autosavePrefix;
            if (!prefix.empty())
            {
              std::string ofname = prefix + ".bfc.nii.gz";
              if (correctedVolume->write(ofname))
                std::cout<<"wrote "<<ofname<<std::endl;
              else
                std::cerr<<"error writing "<<ofname<<std::endl;
            }
            brainSuiteWindow->updateView(DisplayCode::Image1Only|DisplayCode::RedrawView);
            brainSuiteWindow->repaintImages();
          }
      }
      break;
    case BFCTool::Finished : break;
    default: break;
  }
  updateProgress(brainSuiteWindow,currentStep(),nSteps());
  if (!isFinished())
  {
    setStepName(brainSuiteWindow,"next step: " + stepName());
  }
  else
  {
    setStepName(brainSuiteWindow,"finished nonuniformity correction.");
  }
  return retcode;
}

bool BFCGUITool::stepBack(BrainSuiteWindow * /* brainSuiteWindow */)
{
  return false;
}

std::string BFCGUITool::stepName() const
{
  return bfcTool.runningStepName();
}

std::string BFCGUITool::nextStepText() const
{
  return bfcTool.nextStepName();
}

bool BFCGUITool::isFinished() const
{
  return (bfcTool.state==BFCTool::Finished);
}

bool BFCGUITool::autosaveOutputs(BrainSuiteWindow * /* brainSuiteWindow */)
{
  return false;
}


bool BFCGUITool::resetIterations()
{
  return bfcTool.bfc.resetIterations();
}

bool BFCGUITool::atEnd()
{
  return bfcTool.bfc.atEnd();
}

int BFCGUITool::iterationNumber()
{
  return (int)bfcTool.bfc.iterationNumber();
}

int BFCGUITool::nIterations()
{
  return (int)bfcTool.bfc.nIterations();
}


bool BFCGUITool::performNextIteration(Vol3D<float32> &vWorking, Vol3D<float> &vSpline, Vol3D<uint8> &vMask)
{
  return bfcTool.bfc.performNextIteration(vWorking,vSpline,vMask);
}
