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

#include <svregguitool.h>
#include <brainsuitewindow.h>
#include <toolmanager.h>
#include <DS/timer.h>
#include <waitcursor.h>
#include <surfacealpha.h>
#include <displaycode.h>
#include <brainsuiteqtsettings.h>
#include <dualregistrationprocessdialog.h>
#include <registrationprocessdialog.h>
#include <qmessagebox.h>
#include <qdir.h>
#include <corticalextractionsequencedialog.h>
#include <corticalextractionsettings.h>
#include <svregistrationdialog.h>
#include <qapplication.h>
#include <qprocess.h>
#include <brainsuitesettings.h>
#include <corticalextractionsequencedialog.h>
#include <toolmanager.h>

SVRegGUITool::SVRegGUITool() : initialized(0), left(0), right(0)
{
}

bool SVRegGUITool::initializeTool(CorticalExtractionSettings &ces)
{
  initialized=svreg.initializeExecutablePath(ces);
  return initialized;
}

int SVRegGUITool::nSteps() const
{
  return SVRegistrationTool::Finished;
}

int SVRegGUITool::currentStep() const
{
  return svreg.state;
}

void SVRegGUITool::updateProgress(BrainSuiteWindow *brainSuiteWindow, const int value, const int maxvalue)
{
  if (!brainSuiteWindow) return;
  if (brainSuiteWindow->toolManager.svRegistrationDialog)
    brainSuiteWindow->toolManager.svRegistrationDialog->setProgress(value,maxvalue);
  if (brainSuiteWindow->toolManager.corticalExtractionSequenceDialog)
    brainSuiteWindow->toolManager.corticalExtractionSequenceDialog->setProgress(value,maxvalue);
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

void SVRegGUITool::setStepName(BrainSuiteWindow *brainSuiteWindow, std::string s)
{
  if (!brainSuiteWindow) return;
  if (brainSuiteWindow->toolManager.svRegistrationDialog)
    brainSuiteWindow->toolManager.svRegistrationDialog->updateStepText(s);
  if (brainSuiteWindow->toolManager.corticalExtractionSequenceDialog)
    brainSuiteWindow->toolManager.corticalExtractionSequenceDialog->updateStepText(s);
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

bool SVRegGUITool::runStep(BrainSuiteWindow *brainSuiteWindow, QWidget *caller)
{
  if (!brainSuiteWindow) return false;
  if (!brainSuiteWindow->brainSuiteDataManager.volume)
  {
    std::cerr<<"You must load an image volume before you can perform registration."<<std::endl;
    return false;
  }
  WaitCursor wc(brainSuiteWindow);
  if (!initialized)
  {
    if (!initializeTool(brainSuiteWindow->ces))
    {
      if (BrainSuiteQtSettings::qtsvregdir.empty())
        std::cerr<<"SVReg is not installed."<<std::endl;
      else
        std::cerr<<"SVReg could not be initialized."<<std::endl;
      return false;
    }
  }
  updateProgress(brainSuiteWindow,currentStep(),nSteps());
  if (svreg.state == SVRegistrationTool::Finished) return false;
  Timer t;
  t.start();
  bool surfaceOnly = brainSuiteWindow->ces.svreg.surfaceOnly;
  if (svreg.state == SVRegistrationTool::Finished) return false;
  bool retcode=false;
  if (!isFinished())
  {
    setStepName(brainSuiteWindow,"running " + stepName());
  }
  else
  {
    setStepName(brainSuiteWindow,"finished registration and labeling.");
  }
  switch (svreg.state)
  {
    case SVRegistrationTool::Initialize :
      retcode=initialize(brainSuiteWindow);
      break;
    case SVRegistrationTool::LabelSurfaces :
      retcode=labelSurfaces(brainSuiteWindow,caller);
      break;
    case SVRegistrationTool::MakeVolumeMap :
      if (!surfaceOnly)
      {
        retcode=makeVolumeMap(brainSuiteWindow,caller);
        break;
      }
    case SVRegistrationTool::RegisterVolumes :
      if (!surfaceOnly)
      {
        retcode=registerVolumes(brainSuiteWindow,caller);
        break;
      }
    case SVRegistrationTool::RefineROIs:
      retcode=refineROIs(brainSuiteWindow);
      break;
    case SVRegistrationTool::RefineRegistration:
      if (!surfaceOnly)
      {
        retcode=refineRegistration(brainSuiteWindow,caller);
        break;
      }
    case SVRegistrationTool::RefineSulci:
      if (brainSuiteWindow->ces.svreg.refineSulci)
      {
        retcode=refineSulci(brainSuiteWindow,caller);
        break;
      }
      svreg.state = SVRegistrationTool::ComputeStats;
      retcode=false;
      break;
    case SVRegistrationTool::ComputeStats:
      retcode=computeStats(brainSuiteWindow);
      break;
    case SVRegistrationTool::CleanUp:
      retcode=cleanupFiles(brainSuiteWindow);
      break;
    case SVRegistrationTool::Finished:
      retcode=false;
      break;
  }
  t.stop();
  if (!isFinished())
  {
    setStepName(brainSuiteWindow,stepName());
  }
  else
  {
    setStepName(brainSuiteWindow,"finished registration and labeling.");
  }
  updateProgress(brainSuiteWindow,svreg.state,nSteps());
  return retcode;
}

bool SVRegGUITool::stepBack(BrainSuiteWindow * /*brainSuiteWindow*/)
{
  return false;
}

std::string SVRegGUITool::stepName() const
{
  return svreg.stateName(svreg.state);
}

std::string SVRegGUITool::nextStepText() const
{
  return (isFinished()) ? "finished" : std::string("next step: ")+stepName();
}

bool SVRegGUITool::isFinished() const
{
  return (svreg.state==SVRegistrationTool::Finished);
}


bool SVRegGUITool::initialize(BrainSuiteWindow *brainSuiteWindow)
{
  svreg.initializeExecutablePath(brainSuiteWindow->ces);
  if (!brainSuiteWindow->brainSuiteDataManager.volume) return false;
  Timer progTimer; progTimer.start();
  std::string basefile;
  if (brainSuiteWindow->toolManager.corticalExtractionSequenceDialog) // move to BSW?
  {
    if (brainSuiteWindow->toolManager.corticalExtractionSequenceDialog->autosaveValid())
    {
      std::cout<<"Using Autosave Prefix:"<<BrainSuiteSettings::autosavePrefix<<std::endl;
      basefile = BrainSuiteSettings::autosavePrefix;
    }
  }
  else
  {
  }
  if (basefile.empty())
  {
    if (!brainSuiteWindow->brainSuiteDataManager.volume) return false;
    basefile = brainSuiteWindow->brainSuiteDataManager.volume->filename;
  }
  if (!svreg.setup(basefile))
  {
    std::cerr<<"Error: Unable to start registration tool. Process halted."<<std::endl;
    return false;
  }
  if (!svreg.checkForRequiredFiles())
  {
    std::cerr<<svreg.errorString;
    std::cerr<<"Error: Required files were not found. Process halted.\n";
    return false;
  }
  svreg.state = SVRegistrationTool::LabelSurfaces;
  progTimer.stop();
  std::cout<<"Finished file preparation\t"<<progTimer.elapsedSecs()<<'\n';
  return true;
}


bool SVRegGUITool::labelSurfaces(BrainSuiteWindow *brainSuiteWindow, QWidget *caller)
{
  Timer progTimer; progTimer.start();
  DualRegistrationProcessDialog dialog(caller ? caller : brainSuiteWindow);
  dialog.processName = "Label Cerebral Hemisphere Surfaces";
  dialog.rightTag = "Label Right Hemisphere Surface";
  dialog.leftTag = "Label Left Hemisphere Surface";
  QString program = svreg.labelSurfaceExe.c_str();
  QStringList leftArguments;
  if (svreg.MCRPath.empty()==false)
    leftArguments.push_back(svreg.MCRPath.c_str());
  leftArguments.push_back(svreg.subjectBase.c_str());
  leftArguments.push_back(svreg.atlasBase.c_str());
  leftArguments.push_back("left");
  leftArguments.push_back("-v1");
  QStringList rightArguments;
  if (svreg.MCRPath.empty()==false)
    rightArguments.push_back(svreg.MCRPath.c_str());
  rightArguments.push_back(svreg.subjectBase.c_str());
  rightArguments.push_back(svreg.atlasBase.c_str());
  rightArguments.push_back("right");
  rightArguments.push_back("-v1");
  int retcode = dialog.run(program,svreg.dirPath,leftArguments,rightArguments);
  if (retcode!=QDialog::Accepted)
  {
    switch (dialog.state())
    {
      case RegistrationProcessDialog::Error :
        {
          std::cout<<"Error running surface labeling. Processing halted.\n";
          QMessageBox messageBox(caller ? caller : brainSuiteWindow);
          messageBox.setText("Registration Process Error");
          messageBox.setIcon(QMessageBox::Critical);
          messageBox.setStandardButtons(QMessageBox::Ok);
          std::ostringstream ostr;
          if (dialog.errorMessage().empty())
          {
            ostr<<"An error occurred during processing\n";
          }
          else
            ostr<<dialog.errorMessage()<<'\n';
          ostr<<"Please see log window for process output\n";
          std::cout<<dialog.leftProcessLog()<<std::endl;
          std::cout<<dialog.rightProcessLog()<<std::endl;
          messageBox.setInformativeText(ostr.str().c_str());
          messageBox.exec();
          return false;
        }
      case RegistrationProcessDialog::CanceledByUser :
        {
          std::cout<<"Surface labeling process canceled by user.\n";
          return false;
        }
      default:
        break;
    }
  }
  progTimer.stop();
  std::cout<<"Finished surface labeling\t"<<progTimer.elapsedMMSS()<<"\n";
  brainSuiteWindow->hideAllSurfaces();
  left=brainSuiteWindow->loadSurface(svreg.subjectBase + ".left.mid.cortex.svreg.init.dfs");
  right=brainSuiteWindow->loadSurface(svreg.subjectBase + ".right.mid.cortex.svreg.init.dfs");
  svreg.state = brainSuiteWindow->ces.svreg.surfaceOnly ? SVRegistrationTool::RefineROIs : SVRegistrationTool::MakeVolumeMap;
  return true;
}

bool SVRegGUITool::makeVolumeMap(BrainSuiteWindow *brainSuiteWindow, QWidget *caller)
{
  Timer progTimer; progTimer.start();
  QString program = QDir::toNativeSeparators(svreg.volmapBallExe.c_str());
  QStringList subjectArguments;
  if (svreg.MCRPath.empty()==false)
    subjectArguments.push_back(svreg.MCRPath.c_str());
  subjectArguments.push_back(svreg.subjectBase.c_str());
  subjectArguments.push_back("1");
  subjectArguments.push_back("-v1");
  QStringList atlasArguments;
  if (svreg.MCRPath.empty()==false)
    atlasArguments.push_back(svreg.MCRPath.c_str());
  atlasArguments.push_back((svreg.subjectBase+".target").c_str());
  atlasArguments.push_back("1");
  atlasArguments.push_back("-v1");
  DualRegistrationProcessDialog dialog(caller ? caller : brainSuiteWindow);
  dialog.processName = "Make Volume Map";
  dialog.leftTag = "Make Subject Volume Map";
  dialog.rightTag = "Make Atlas Volume Map";
  int retcode = dialog.run(program,svreg.dirPath,subjectArguments,atlasArguments);
  if (retcode!=QDialog::Accepted)
  {
    switch (dialog.state())
    {
      case RegistrationProcessDialog::Error :
        {
          std::cout<<"Error running volume mapping. Processing halted.\n";
          QMessageBox msgBox(brainSuiteWindow);
          msgBox.setText("Registration Process Error");
          msgBox.setIcon(QMessageBox::Critical);
          msgBox.setStandardButtons(QMessageBox::Ok);
          std::ostringstream ostr;
          if (dialog.errorMessage().empty())
            ostr<<"An error occurred during processing\n";
          else
            ostr<<dialog.errorMessage()<<'\n';
          ostr<<"Please see log window for process output\n";
          std::cout<<dialog.leftProcessLog()<<std::endl;
          std::cout<<dialog.rightProcessLog()<<std::endl;
          msgBox.setInformativeText(ostr.str().c_str());
          msgBox.exec();
          return false;
        }
      case RegistrationProcessDialog::CanceledByUser :
        {
          std::cout<<"Surface labeling process canceled by user.\n";
          return false;
        }
      default:
        break;
    }
  }
  progTimer.stop();
  std::cout<<"Finished volume mapping\t"<<progTimer.elapsedMMSS()<<"\n";
  svreg.state = SVRegistrationTool::RegisterVolumes;
  return true;
}

bool SVRegGUITool::registerVolumes(BrainSuiteWindow *brainSuiteWindow, QWidget *caller)
{
  Timer progTimer; progTimer.start();
  QString program = QDir::toNativeSeparators(svreg.volregExe.c_str());
  QStringList arguments;
  if (svreg.MCRPath.empty()==false)
    arguments.push_back(svreg.MCRPath.c_str());
  arguments.push_back(svreg.subjectBase.c_str());
  arguments.push_back((svreg.subjectBase+".target").c_str());
  arguments.push_back("-v1");
  RegistrationProcessDialog dialog(caller ? caller : brainSuiteWindow);
  dialog.processName = "Volume Registration";
  int retcode = dialog.run(program,svreg.dirPath,arguments);
  if (retcode!=QDialog::Accepted)
  {
    switch (dialog.state())
    {
      case RegistrationProcessDialog::Error :
        {
          std::cout<<"Error running volume registration. Processing halted.\n";
          //				updateStatusText();
          QMessageBox msgBox(brainSuiteWindow);
          msgBox.setText("Registration Process Error");
          msgBox.setIcon(QMessageBox::Critical);
          msgBox.setStandardButtons(QMessageBox::Ok);
          std::ostringstream ostr;
          if (dialog.errorMessage().empty())
            ostr<<"An error occurred during processing\n";
          else
            ostr<<dialog.errorMessage()<<'\n';
          ostr<<"Please see log window for process output\n";
          std::cout<<dialog.processLog()<<std::endl;
          msgBox.setInformativeText(ostr.str().c_str());
          /*int code = */msgBox.exec();
          return false;
        }
      case RegistrationProcessDialog::CanceledByUser :
        {
          std::cout<<"Volume registration process canceled by user.\n";
          return false;
        }
      default:
        break;
    }
  }
  progTimer.stop();
  std::cout<<"Finished volume registration\t"<<progTimer.elapsedMMSS()<<"\n";
  svreg.state = SVRegistrationTool::RefineROIs;
  brainSuiteWindow->loadLabelVolume(svreg.subjectBase + ".svreg.init.label.nii.gz");
  brainSuiteWindow->updateView(DisplayCode::ShowLabels|DisplayCode::RedrawView);
  return true;
}

bool SVRegGUITool::refineROIs(BrainSuiteWindow *brainSuiteWindow, QWidget *caller)
{
  Timer progTimer; progTimer.start();
  DualRegistrationProcessDialog dialog(caller ? caller : brainSuiteWindow);
  dialog.processName = "Refine Hemisphere Surface Labels";
  dialog.rightTag = "Refine Right Hemisphere Surface Labels";
  dialog.leftTag = "Refine Left Hemisphere Surface Labels";
  QString program = QDir::toNativeSeparators(svreg.refineROIExe.c_str());
  QStringList leftArguments;
  if (svreg.MCRPath.empty()==false)
    leftArguments.push_back(svreg.MCRPath.c_str());
  leftArguments.push_back(svreg.subjectBase.c_str());
  leftArguments.push_back("left");
  leftArguments.push_back("-v1");
  QStringList rightArguments;
  if (svreg.MCRPath.empty()==false)
    rightArguments.push_back(svreg.MCRPath.c_str());
  rightArguments.push_back(svreg.subjectBase.c_str());
  rightArguments.push_back("right");
  rightArguments.push_back("-v1");
  int retcode = dialog.run(program,svreg.dirPath,leftArguments,rightArguments);
  if (retcode!=QDialog::Accepted)
  {
    std::cout<<"Error running surface labeling refinement. Processing halted.\n";
    QMessageBox msgBox(brainSuiteWindow);
    msgBox.setText("Registration Process Error");
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setStandardButtons(QMessageBox::Ok);
    std::ostringstream ostr;
    if (dialog.errorMessage().empty())
      ostr<<"An error occurred during processing\n";
    else
      ostr<<dialog.errorMessage()<<'\n';
    msgBox.setInformativeText(ostr.str().c_str());
    msgBox.exec();
    return false;
  }
  progTimer.stop();
  std::cout<<"Finished refining surface labels\t"<<progTimer.elapsedMMSS()<<"\n";
  if (brainSuiteWindow->ces.svreg.surfaceOnly)
  {
    if (brainSuiteWindow->ces.svreg.refineSulci)
      svreg.state = SVRegistrationTool::RefineSulci;
    else
      svreg.state = //SVRegistrationTool::CleanUp;
          SVRegistrationTool::ComputeStats;

  }
  else
    svreg.state = SVRegistrationTool::RefineRegistration;
  brainSuiteWindow->hideAllSurfaces();
  if (left) brainSuiteWindow->brainSuiteDataManager.removeSurface(left);
  if (right) brainSuiteWindow->brainSuiteDataManager.removeSurface(right);
  left=brainSuiteWindow->loadSurface(svreg.subjectBase + ".left.mid.cortex.svreg.dfs");
  right=brainSuiteWindow->loadSurface(svreg.subjectBase + ".right.mid.cortex.svreg.dfs");
  return true;
}

bool SVRegGUITool::refineRegistration(BrainSuiteWindow *brainSuiteWindow, QWidget *caller)
{
  Timer progTimer; progTimer.start();
  QString program = QDir::toNativeSeparators(svreg.refineRegistrationExe.c_str());
  QStringList arguments;
  if (svreg.MCRPath.empty()==false)
    arguments.push_back(svreg.MCRPath.c_str());
  arguments.push_back(svreg.subjectBase.c_str());
  arguments.push_back((svreg.subjectBase+".target").c_str());
  if (brainSuiteWindow->ces.svreg.extendLabels)
    arguments.push_back("-C");
  arguments.push_back("-v1");
  RegistrationProcessDialog dialog(caller ? caller : brainSuiteWindow);
  dialog.processName = "Volume Labeling Refinement";
  dialog.tag = "Perform Volume Labeling Refinement";
  int retcode = dialog.run(program,svreg.dirPath,arguments);
  if (retcode!=QDialog::Accepted)
  {
    std::cerr<<"Error running volume labeling refinement. Processing halted.\n";
    QMessageBox msgBox(caller ? caller : brainSuiteWindow);
    msgBox.setText("Registration Process Error");
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setStandardButtons(QMessageBox::Ok);
    std::ostringstream ostr;
    if (dialog.errorMessage().empty())
      ostr<<"An error occurred during processing\n";
    else
      ostr<<dialog.errorMessage()<<'\n';
    msgBox.setInformativeText(ostr.str().c_str());
    msgBox.exec();
    return false;
  }
  progTimer.stop();
  std::cout<<"Finished volume labeling refinement\t"<<progTimer.elapsedMMSS()<<"\n";
  svreg.state = (brainSuiteWindow->ces.svreg.refineSulci) ? SVRegistrationTool::RefineSulci : SVRegistrationTool::ComputeStats;
  brainSuiteWindow->loadLabelVolume(svreg.subjectBase + ".svreg.label.nii.gz");
  brainSuiteWindow->updateView(DisplayCode::ShowLabels|DisplayCode::RedrawView);
  return true;
}

bool SVRegGUITool::computeStats(BrainSuiteWindow *brainSuiteWindow)
{
  QString program = QDir::toNativeSeparators(svreg.computeStatsExe.c_str());
  QStringList arguments;
  if (svreg.MCRPath.empty()==false)
    arguments.push_back(svreg.MCRPath.c_str());
  arguments.push_back(svreg.subjectBase.c_str());
  arguments.push_back("-r");
  QProcess process(brainSuiteWindow);
  process.start(program,arguments);
  std::cout<<"Executing: "<<program.toStdString();
  foreach (QString line, arguments)
    std::cout<<" "<<line.toStdString();
  std::cout<<std::endl;
  process.waitForFinished();
  svreg.state = SVRegistrationTool::CleanUp;
  return true;
}

bool SVRegGUITool::cleanupFiles(BrainSuiteWindow *brainSuiteWindow)
{
  QString program = QDir::toNativeSeparators(svreg.cleanIntermediateFilesExe.c_str());
  QStringList arguments;
  if (svreg.MCRPath.empty()==false)
    arguments.push_back(svreg.MCRPath.c_str());
  arguments.push_back(svreg.subjectBase.c_str());
  QProcess process(brainSuiteWindow);
  process.start(program,arguments);
  std::cout<<"Executing: "<<program.toStdString();
  foreach (QString line, arguments)
    std::cout<<" "<<line.toStdString();
  std::cout<<std::endl;
  process.waitForFinished();
  svreg.state = SVRegistrationTool::Finished;
  return true;
}

bool SVRegGUITool::refineSulci(BrainSuiteWindow *brainSuiteWindow, QWidget *caller)
{
  if (!brainSuiteWindow) return false;
  if (brainSuiteWindow->ces.svreg.refineSulci==false)
  {
    svreg.state = SVRegistrationTool::ComputeStats;
    return true;
  }
  Timer progTimer; progTimer.start();
  DualRegistrationProcessDialog dialog(caller ? caller : brainSuiteWindow);
  dialog.processName = "Refine Sulcal Landmarks";
  dialog.rightTag = "Refine Right Hemisphere Sulcal Landmarks";
  dialog.leftTag = "Refine Left Hemisphere Sulcal Landmarks";
  QString program = QDir::toNativeSeparators(svreg.refineSulciExe.c_str());
  QStringList leftArguments;
  if (svreg.MCRPath.empty()==false)
    leftArguments.push_back(svreg.MCRPath.c_str());
  leftArguments.push_back(svreg.subjectBase.c_str());
  leftArguments.push_back("left");
  leftArguments.push_back("-v1");
  QStringList rightArguments;
  if (svreg.MCRPath.empty()==false)
    rightArguments.push_back(svreg.MCRPath.c_str());
  rightArguments.push_back(svreg.subjectBase.c_str());
  rightArguments.push_back("right");
  rightArguments.push_back("-v1");
  int retcode = dialog.run(program,svreg.dirPath,leftArguments,rightArguments);
  if (retcode!=QDialog::Accepted)
  {
    std::cerr<<"Error running sulcal refinement. Processing halted.\n";
    QMessageBox msgBox(brainSuiteWindow);
    msgBox.setText("Registration Process Error");
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setStandardButtons(QMessageBox::Ok);
    std::ostringstream ostr;
    if (dialog.errorMessage().empty())
      ostr<<"An error occurred during processing\n";
    else
      ostr<<dialog.errorMessage()<<'\n';
    msgBox.setInformativeText(ostr.str().c_str());
    msgBox.exec();
    return false;
  }
  progTimer.stop();
  std::cout<<"Finished refining sulcal landmarks\t"<<progTimer.elapsedMMSS()<<"\n";
  svreg.state = SVRegistrationTool::ComputeStats;
  return true;
}
