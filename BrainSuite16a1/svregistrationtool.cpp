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

#include "svregistrationtool.h"
#include <brainsuiteqtsettings.h>
#include <qdir.h>
#include <iostream>
#include <sstream>
#include <qdir.h>
#include <volumeloader.h>
#include <vol3d.h>
#include <vector>
#include <qdiriterator.h>
#include <corticalextractionsettings.h>

std::string stripExtensions(std::string fullPathname);

bool copyAndUnzip(std::string ifname, std::string ofname)
{
  Vol3DBase *v = VolumeLoader::load(ifname);
  if (!v) return false;
  if (!v->write(ofname)) return false;
  return true;
}

bool checkSVRegVersion(std::string minVersion)
{
  if (minVersion.empty()) return false;
  std::istringstream istr(minVersion);
  int major;
  char minor;
  istr>>major>>minor;
	if (major>16) return false;
	if (major==16 && minor>'a') return false;
  if (major<14) return false; // we can't use SVReg built for BrainSuite13
  return true;
}

bool checkSVRegVersion(SVRegManifest &manifest)
{
  return checkSVRegVersion(manifest.minimumBrainSuiteVersion);
}

SVRegistrationTool::SVRegistrationTool() : state(SVRegistrationTool::Initialize), executablePathOK(false), executablesOK(false), atlasPathOK(false),
  atlasFilesOK(false)
{
  labelSurfaceBasename="svreg_label_surf_hemi";
  volmapBallBasename="volmap_ball";
  volregBasename="svreg_volreg";
  refineROIBasename="refine_ROIs2";
  refineRegistrationBasename="svreg_refinements";
  refineSulciBasename="refine_sulci_hemi";
  computeStatsBasename="generate_stats_xls";
  cleanIntermediateFilesBasename="clean_intermediate_files";
#if _WIN32
  matlabAppPrefix = "";
  matlabAppSuffix = ".ctf";
  matlabExePrefix = "";
  matlabExeSuffix = ".exe";
#else
#ifdef Q_OS_LINUX
  matlabAppPrefix = "";
  matlabAppSuffix = "";
  matlabExePrefix = "";
  matlabExeSuffix = ".sh";
#else
  matlabAppPrefix = "";
  matlabAppSuffix = ".app";
  matlabExePrefix = "";
  matlabExeSuffix = ".sh";
#endif
#endif
}

bool SVRegistrationTool::checkForRequiredFiles(const SVRegistrationTool::RegistrationState state)
{
  const char *initialFileEndings[] =
  {
    ".left.inner.cortex.dfs",
    ".left.pial.cortex.dfs",
    ".right.inner.cortex.dfs",
    ".right.pial.cortex.dfs",
    ".warp",
    ".cortex.dewisp.mask.nii.gz",
    ".hemi.label.nii.gz",
    ".bfc.nii.gz"
  };
  std::ostringstream errstream;
  bool allFilesOK=true;
  std::list<std::string> missingFiles;
  switch (state)
  {
    case Initialize :
    case LabelSurfaces :
    default:
      {
        const int nFiles = sizeof(initialFileEndings)/sizeof(initialFileEndings[0]);
        for (int i=0;i<nFiles;i++)
        {
          std::string filename=(subjectBase + initialFileEndings[i]);
          QFileInfo qi(filename.c_str());
          if (qi.exists()==false)
          {
            missingFiles.push_back(filename);
            allFilesOK=false;
          }
        }
        break;
      }
  }
  if (missingFiles.size())
  {
    errstream<<"The following required files are not present:\n";
    for (std::list<std::string>::iterator filename=missingFiles.begin();filename!=missingFiles.end();filename++)
    {
      errstream<<*filename<<'\n';
    }
  }
  errorString = errstream.str();
  return allFilesOK;
}

std::string SVRegistrationTool::runningStepName(const SVRegistrationTool::RegistrationState &state)
{
  switch (state)
  {
    case Initialize : return "Initializing files"; break;
    case LabelSurfaces : return "Labeling surfaces"; break;
      //		case MapCorticalThickness : return "Map Cortical Thickness"; break;
    case MakeVolumeMap : return "Making volume map"; break;
    case RegisterVolumes : return "Registering volumes"; break;
    case RefineROIs : return "Refining ROIs"; break;
    case RefineRegistration : return "Refining Registration"; break;
    case RefineSulci : return "Refining Sulci"; break;
    case ComputeStats : return "Computing Statistics"; break;
    case CleanUp : return "Removing Intermediate Files"; break;
    case Finished : return "Finished"; break;
    default : break;
  }
  std::cout<<"state code is "<<(int)state<<std::endl;
  return "error";
}

std::string SVRegistrationTool::stateName(const SVRegistrationTool::RegistrationState &state) const
{
  switch (state) {
    case Initialize : return "Initialize Files"; break;
    case LabelSurfaces : return "Label Surfaces"; break;
    case MakeVolumeMap : return "Make Volume Map"; break;
    case RegisterVolumes : return "Register Volumes"; break;
    case RefineROIs : return "Refine ROIs"; break;
    case RefineRegistration : return "Refine Registration"; break;
    case RefineSulci : return "Refine Sulci"; break;
    case ComputeStats : return "Compute Statistics"; break;
    case CleanUp : return "Cleanup Intermediate Files"; break;
    case Finished : return "Finished"; break;
    default : break;
  }
  std::cout<<"state code is "<<(int)state<<std::endl;
  return "error";
}

std::string fixPath(std::string filepath)
{
  return QDir::toNativeSeparators(QDir::cleanPath(filepath.c_str())).toStdString();
}

std::string cleanFilename(std::string filepath)
{
  return QDir::toNativeSeparators(QDir::cleanPath(filepath.c_str())).toStdString();
}

std::string SVRegistrationTool::matlabAppPath(std::string basename)
{
  return fixPath(svregPath + "/" + matlabAppPrefix + basename + matlabAppSuffix);
}

std::string SVRegistrationTool::matlabExePath(std::string basename)
{
  return fixPath(svregPath + "/" + matlabExePrefix + basename + matlabExeSuffix);
}

bool SVRegistrationTool::initializeExecutablePath(CorticalExtractionSettings &ces)
{
  if (BrainSuiteQtSettings::qtsvregdir.empty())
  {
    return false;
  }
  std::ostringstream errstream;
  svregPath=fixPath(BrainSuiteQtSettings::qtsvregdir +  "/bin");
  std::string manifestFile(BrainSuiteQtSettings::qtsvregdir + "/svregmanifest.xml");
  if (svregManifest.read(manifestFile))
  {
    bool compatible = checkSVRegVersion(svregManifest.minimumBrainSuiteVersion);
    if (svregManifest.minimumBrainSuiteVersion.empty()||!compatible)
    {
      std::cerr<<"The selected version of SVReg ("<<manifestFile<<") is not compatible with this version of BrainSuite"<<std::endl;
      return false;
    }
  }
  else
  {
    svregManifest=SVRegManifest();
  }
  atlasPath=fixPath(BrainSuiteQtSettings::qtsvregdir +  "/BrainSuiteAtlas1");
  atlasBase=fixPath(atlasPath+"/mri");
  QDir directory(svregPath.c_str());
  executablePathOK=directory.exists();
  if (executablePathOK==false)
  {
    errstream<<"Error: SVReg executable directory " + svregPath + " does not exist.\n";
    executablesOK = false;
  }
  else if (ces.svreg.atlasIndex<0 || ces.svreg.atlasIndex>= (int)svregManifest.atlases.size())
  {
    executablesOK=false;
  }
  else
  {
    SVRegManifest::Atlas atlas = svregManifest.atlases[ces.svreg.atlasIndex];
    ces.svreg.atlasName = atlas.name;
    atlasPath=fixPath(BrainSuiteQtSettings::qtsvregdir +  "/" + atlas.name);
    atlasBase=fixPath(atlasPath+"/"+atlas.basename);
    executablesOK = true;
    labelSurfaceExe=matlabExePath(labelSurfaceBasename);
    volmapBallExe=matlabExePath(volmapBallBasename);
    volregExe=matlabExePath(volregBasename);
    refineROIExe=matlabExePath(refineROIBasename);
    refineRegistrationExe=matlabExePath(refineRegistrationBasename);
    refineSulciExe=matlabExePath(refineSulciBasename);
    computeStatsExe=matlabExePath(computeStatsBasename);
    cleanIntermediateFilesExe=matlabExePath(cleanIntermediateFilesBasename);
    std::list<std::string> requiredExecutables;
    requiredExecutables.push_back(labelSurfaceExe);
#ifndef _WIN32
    requiredExecutables.push_back(matlabAppPath(labelSurfaceBasename));
#endif
    requiredExecutables.push_back(volmapBallExe);
#ifndef _WIN32
    requiredExecutables.push_back(matlabAppPath(volmapBallBasename));
#endif
    requiredExecutables.push_back(volregExe);
#ifndef _WIN32
    requiredExecutables.push_back(matlabAppPath(volregBasename));
#endif
    requiredExecutables.push_back(refineROIExe);
#ifndef _WIN32
    requiredExecutables.push_back(matlabAppPath(refineROIBasename));
#endif
    requiredExecutables.push_back(refineRegistrationExe);
#ifndef _WIN32
    requiredExecutables.push_back(matlabAppPath(refineRegistrationBasename));
#endif
    requiredExecutables.push_back(refineSulciExe);
#ifndef _WIN32
    requiredExecutables.push_back(matlabAppPath(computeStatsBasename));
#endif
    requiredExecutables.push_back(computeStatsExe);
#ifndef _WIN32
    requiredExecutables.push_back(matlabAppPath(refineSulciBasename));
#endif
    requiredExecutables.push_back(cleanIntermediateFilesExe);
    std::list<std::string> missingExecutableFiles;
    for (std::list<std::string>::iterator filename=requiredExecutables.begin();filename!=requiredExecutables.end();filename++)
    {
      QFileInfo qi(filename->c_str());
      if (qi.exists()==false)
      {
        missingExecutableFiles.push_back(*filename);
        executablesOK=false;
      }
    }
    if (missingExecutableFiles.size())
    {
      errstream<<"The following required files are not present:\n";
      for (std::list<std::string>::iterator filename=missingExecutableFiles.begin();filename!=missingExecutableFiles.end();filename++)
      {
        errstream<<*filename<<'\n';
      }
    }
  }
  QDir atlasDir(atlasPath.c_str());
  atlasPathOK=atlasDir.exists();
  if (!atlasPathOK)
  {
    errstream<<"Error: SVReg atlas directory " + atlasPath + " does not exist.\n";
    atlasFilesOK = false;
  }
  else
  {
    atlasFilesOK = true;
    std::list<std::string> missingAtlasFiles;
  }
  errorString = errstream.str();
  if (errorString.empty()==false)
    std::cerr<<errorString;
  mcrOK=true;
  return executablePathOK && executablesOK && atlasPathOK && atlasFilesOK && mcrOK;
}

bool SVRegistrationTool::copyAtlasFiles()
{
  bool fail=false;
  std::ostringstream errstream;
  std::string srcAtlasCortexMaskFilename=atlasBase+".cortex.dewisp.mask.nii.gz";
  std::string dstAtlasCortexMaskFilename=subjectBase+".target.cortex.dewisp.mask.nii.gz";
  std::string srcAtlasXMLfilename=cleanFilename(atlasPath+"/brainsuite_labeldescription.xml");
  std::string dstAtlasXMLfilename=cleanFilename(subjectPath+"/brainsuite_labeldescription.xml");
  {
    QFileInfo fileInfo(srcAtlasXMLfilename.c_str());
    if (!fileInfo.exists()||!fileInfo.isFile())
    {
      std::cerr<<"unable to open "<<srcAtlasXMLfilename.c_str()<<std::endl;
      std::string alternate=cleanFilename(atlasPath+"/brainsuite_labeltext.xml");
      QFileInfo fileInfo(alternate.c_str());
      if (fileInfo.exists()&&fileInfo.isFile())
      {
        srcAtlasXMLfilename=alternate;
        std::cout<<"using "<<srcAtlasXMLfilename.c_str()<<" instead"<<std::endl;
      }
    }
  }
  std::string srcMRIFilename=atlasBase+".bfc.nii.gz";
  std::string dstMRIFilename=subjectBase+".target.bfc.nii";
  std::string srcLabelFilename=atlasBase+".label.nii.gz";
  std::string dstLabelFilename=subjectBase+".target.label.nii";
  QFile atlasXMLCheck(dstAtlasXMLfilename.c_str());
  if (atlasXMLCheck.exists()) QFile::remove(dstAtlasXMLfilename.c_str());
  if (!QFile::copy(srcAtlasXMLfilename.c_str(),dstAtlasXMLfilename.c_str()))
  {
    errstream<<"Unable to copy "<<srcAtlasXMLfilename<<'\n';
    fail=true;
  }
  QFile atlasCortexCheck(dstAtlasCortexMaskFilename.c_str());
  if (atlasCortexCheck.exists()) QFile::remove(dstAtlasCortexMaskFilename.c_str());
  if (!QFile::copy(srcAtlasCortexMaskFilename.c_str(),dstAtlasCortexMaskFilename.c_str()))
  {
    errstream<<"Unable to copy "<<srcAtlasCortexMaskFilename<<'\n';
    fail=true;
  }
  if (!copyAndUnzip(srcMRIFilename,dstMRIFilename))
  {
    errstream<<"Unable to copy "<<srcMRIFilename<<'\n';
    fail=true;
  }
  if (!copyAndUnzip(srcLabelFilename,dstLabelFilename))
  {
    errstream<<"Unable to copy "<<srcLabelFilename<<'\n';
    fail=true;
  }
  errorString=errstream.str();
  return !fail;
}

bool SVRegistrationTool::setup(std::string primaryFile)
{
  if (primaryFile.empty()) { return false; }
  QFileInfo qi(primaryFile.c_str());
  dirPath=QDir::toNativeSeparators(qi.absolutePath());
  subjectPath=dirPath.toStdString();
  std::string prefix = stripExtensions(qi.fileName().toStdString());
  subjectBase = QDir::toNativeSeparators((dirPath.toStdString()+'/'+prefix).c_str()).toStdString();
  return true;
}
