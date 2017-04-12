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

#include <cerebrumlabelerqt.h>
#include <volumescaler.h>
#include <vol3dops.h>
#include <DS/getfilesize.h>
#include <stdlib.h>
#include <brainsuitewindow.h>

#ifdef _MSC_VER
#include <io.h>
#include <direct.h>
#define tempnam _tempnam
inline bool makedir(std::string dirname) { return (_mkdir(dirname.c_str())==0); }
inline bool removedir(std::string dirname) { return _rmdir(dirname.c_str())==0; }
#else
#include <sys/stat.h>
#include <unistd.h>
#ifndef S_IRGRP
#define S_IRGRP 0
#define S_IWGRP 0
#define S_IXGRP 0
#define S_IROTH 0
#define S_IWOTH 0
#define S_IXOTH 0
#endif
#ifdef __APPLE__
inline bool makedir(std::string dirname) { return (mkdir(dirname.c_str(), S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IWOTH|S_IXOTH)==0); }
#else
#ifdef _WIN32
inline bool makedir(std::string dirname) { return (mkdir(dirname.c_str())==0); }
#else
inline bool makedir(std::string dirname) { return (mkdir(dirname.c_str(), S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IWOTH|S_IXOTH)==0); }
#endif
#endif
inline bool removedir(std::string dirname) { return rmdir(dirname.c_str())==0; }
#endif

#include <volumeloader.h>
#include <DS/timer.h>
#include <qprocess.h>
#include <qdebug.h>
#include <qfileinfo.h>

bool runProgram(std::string programName, std::string stepname, QStringList &arguments)
{
  Timer progTimer; progTimer.start();
  QString program = QDir::toNativeSeparators(programName.c_str());
  std::cout<<"running "<<stepname<<std::endl;
  QProcess process;
  process.start(program,arguments,QIODevice::ReadWrite | QIODevice::Text);
  if(!process.waitForFinished(-1)) // beware the timeout default parameter
  {
    std::cerr<< "executing "<<stepname<<" failed with exit code" << process.exitCode()<<std::endl;
    return false;
  }
  else
    std::cout<< QString(process.readAllStandardOutput()).toStdString();
  progTimer.stop();
  if (process.exitCode()==0) std::cout<<"finished "<<stepname<<" "<<progTimer.elapsedMMSS()<<"\n";
  else std::cout<<"exit: "<<process.exitCode()<<std::endl;
  return (process.exitCode()==0);
}

CerebrumLabelerQt::CerebrumLabelerQt() :
  state (Initialize), verbosity(1), madeTempDir(false),
  costFunction(1), linearConvergence(0.1f), warpConvergence(100.0f), warpLevel(5),
  keepAIR(false), keepWarp(false), keepAll(false), cleanOnClose(true), useCentroids(false)
{
  StrUtil::trailingSlash(atlasPath);
  StrUtil::trailingSlash(tempDirectory);
  initFile = tempDirectory + "scale.init";
  brainFile = tempDirectory + "masked_brain.img";
  warpedLabelFile = tempDirectory + "warped.labels.img";
}

CerebrumLabelerQt::~CerebrumLabelerQt()
{
  if (cleanOnClose) cleanup();
}

std::string CerebrumLabelerQt::stepName() const
{
  switch(state)
  {
    case Initialize  : return "initialize";
    case AlignLinear : return "align linear";
    case AlignWarp   : return "align warp";
    case ResliceWarp : return "reslice warp";
    case Finished    : return "finished";
    default: return "<error in step name>"; // should not reach here
  }
}

bool CerebrumLabelerQt::makeTempDir()
{
  if (verbosity>1)
    std::cout<<"making tempdir"<<std::endl;
  const char *prefix = "brainsuite";
  std::string targetname;
  if (!tempDirectoryBase.empty())
  {
    targetname = tempnam(tempDirectoryBase.c_str(),prefix);
  }
  else
  {
    targetname = tempnam(".",prefix);
  }
  if (verbosity>1)
    std::cout<<"cerebro is making temporary directory "<<targetname<<std::endl;
  madeTempDir = makedir(targetname.c_str());
  if (madeTempDir)
  {
    tempDirectory = targetname;
    createdTempPath = targetname;
  }
  else
  {
    std::cerr<<"Error encountered creating temporary directory. (tried to create:"<<targetname<<" )\n"
               "If this problem persists, please specify a temporary directory on the command line."<<std::endl;
    return false;
  }
  return true;
}

bool CerebrumLabelerQt::initNames()
{
  if (tempDirectory.empty())
  {
    if (!makeTempDir())
    {
      std::cerr<<"error making temp directory for cerebrum labeler."<<std::endl;
      return false;
    }
  }
  if (verbosity>1)
  {
    std::cout<<"using temporary directory "<<tempDirectory.c_str()<<std::endl;
  }
  StrUtil::trailingSlash(tempDirectory);
  initFile = tempDirectory + "scale.init";
  brainFile = tempDirectory + "masked.brain.img";
  if (!keepAIR  && centroidInitializerFile.empty())  centroidInitializerFile  = tempDirectory + "centroid.init";
  if (!keepAIR  && atlasToBrainAirFile.empty())  atlasToBrainAirFile  = tempDirectory + "atlas_to_brain.air";
  if (!keepWarp && atlasToBrainWarpFile.empty()) atlasToBrainWarpFile = tempDirectory + "atlas_to_brain.warp";
  warpedLabelFile=tempDirectory + "warped.labels.img";
  return true;
}

template<class T>
bool CerebrumLabelerQt::maskDataT(Vol3D<uint16> &vOut, Vol3D<T> &volume, Vol3D<uint8> &vMask)
{
  if (volume.isCompatible(vMask))
  {
    vOut.makeCompatible(volume);
    const int ds = vOut.size();
    for (int i=0;i<ds;i++) vOut[i] = (vMask[i]) ? volume[i] : 0;
  }
  else
  {
    std::cout<<"warning: specified mask has different dimensions than volume image.\n"
               "         making mask from image."<<std::endl;
    vOut.makeCompatible(volume);
    const int ds = vOut.size();
    for (int i=0;i<ds;i++) vOut[i] = volume[i];
  }
  return true;
}

template <class T>
bool CerebrumLabelerQt::threshold(Vol3D<uint8> &vMask, Vol3D<T> &vIn)
{
  vMask.makeCompatible(vIn);
  const int ds = vMask.size();
  for (int i=0;i<ds;i++)
    vMask[i] = (vIn[i]) ? 255 : 0;
  return true;
}

template <class FloatT>
bool CerebrumLabelerQt::clampToUint16Masked(Vol3D<uint16> &vOut, const Vol3D<FloatT> &vf, const Vol3D<uint8> &vMask)
{
  if (vf.isCompatible(vMask)==false)
  {
    std::cout<<"mask and volume have different dimensions"<<std::endl;
    return false;
  }
  if (!vOut.makeCompatible(vf))
  {
    std::cout<<"unable to create output volume"<<std::endl;
    return false;
  }
  const int ds = vOut.size();
  for (int i=0;i<ds;i++) vOut[i] = VolumeScaler::u16clamp(vf[i]);
  return true;
}

bool CerebrumLabelerQt::maskData(Vol3D<uint16> &vOut, Vol3DBase *volume, Vol3D<uint8> &vMask, int verbosity)
{
  if (vMask.size()==0)
  {
    if (verbosity>1) std::cout<<"producing mask from volume."<<std::endl;
    switch (volume->typeID())
    {
      case SILT::Uint8 : threshold(vMask,*(Vol3D<uint8> *)volume); break;
      case SILT::Sint8 : threshold(vMask,*(Vol3D<sint8> *)volume); break;
      case SILT::Sint16 : threshold(vMask,*(Vol3D<sint16> *)volume); break;
      case SILT::Uint16 : threshold(vMask,*(Vol3D<uint16> *)volume); break;
      case SILT::Float32 : threshold(vMask,*(Vol3D<float32> *)volume); break;
      case SILT::Float64 : threshold(vMask,*(Vol3D<float64> *)volume); break;
      default:
        std::cout<<"error: unable to process type "<<volume->datatypeName();
        if (volume->filename.empty()==false) std::cout<<" ("<<volume->filename<<")";
        std::cout<<std::endl;
        return false;
    }
  }
  if ((vMask.cx!=volume->cx)||
      (vMask.cy!=volume->cy)||
      (vMask.cz!=volume->cz))
  {
    std::cerr<<"error: mask is incompatible with data volume."<<std::endl;
    return false;
  }
  switch (volume->typeID())
  {
    case SILT::Uint8 : maskDataT(vOut,*(Vol3D<uint8> *)volume,vMask); break;
    case SILT::Sint8 : maskDataT(vOut,*(Vol3D<sint8> *)volume,vMask); break;
    case SILT::Uint16 : maskDataT(vOut,*(Vol3D<uint16> *)volume,vMask); break;
    case SILT::Sint16 : maskDataT(vOut,*(Vol3D<sint16> *)volume,vMask); break;
    case SILT::Float32 : return clampToUint16Masked(vOut, *(Vol3D<float32> *)volume,vMask); break;
    case SILT::Float64 : return clampToUint16Masked(vOut, *(Vol3D<float64> *)volume,vMask); break;
    default:
      std::cout<<"error: unable to process type "<<volume->datatypeName();
      if (volume->filename.empty()==false) std::cout<<" ("<<volume->filename<<")";
      std::cout<<std::endl;
      return false;
  }
  return true;
}

void CerebrumLabelerQt::selectCerebrum(Vol3D<uint8> &vMask, Vol3D<uint8> &vLabel)
{
  vMask.makeCompatible(vLabel);
  const int ds = vMask.size();
  uint8 table[256];
  for (int i=0;i<256;i++) table[i] = 0;
  table[1] = 255;
  table[2] = 255;
  table[3] = 0;
  table[4] = 0;
  table[5] = 255;
  table[6] = 255;
  table[7] = 0;
  table[8] = 0;
  table[15] = 255;
  table[16] = 255;
  for (int i=0;i<ds;i++)
  {
    vMask[i] = table[vLabel[i]];
  }
}

bool CerebrumLabelerQt::labelLobes(Vol3DBase *volume)
{
  return labelLobes(volume,vLabeledLobes);
}

void CerebrumLabelerQt::removeAnalyze(std::string imgfilename)
{
  std::string hfname;
  ::remove(imgfilename.c_str());
  headername(hfname,imgfilename.c_str());
  ::remove(hfname.c_str());
}

void CerebrumLabelerQt::cleanup()
{
  if (!keepAll)
  {
    if (::fileExists(initFile))
    {
      if (verbosity>1) std::cout<<"removing "<<initFile<<std::endl;
      if (::remove(initFile.c_str())!=0)
        std::cerr<<"encountered error removing "<<initFile<<std::endl;
    }
    if (!keepAIR)
    {
      if (::fileExists(atlasToBrainAirFile))
      {
        if (verbosity>1)
          std::cout<<"removing "<<atlasToBrainAirFile<<std::endl;
        if (::remove(atlasToBrainAirFile.c_str())!=0)
          std::cerr<<"encountered error removing "<<atlasToBrainAirFile<<std::endl;
      }
    }
    if (!keepWarp)
    {
      if (verbosity>1) std::cout<<"removing "<<atlasToBrainWarpFile<<std::endl;
      ::remove(atlasToBrainWarpFile.c_str());
    }
    if (centroidInitializerFile.empty()==false)
    {
      if (verbosity>1) std::cout<<"removing "<<centroidInitializerFile.c_str()<<std::endl;
      ::remove(centroidInitializerFile.c_str());
    }
    removeAnalyze(brainFile);
    removeAnalyze(warpedLabelFile);
    removeAnalyze(alignedAtlasFile);
    removeAnalyze(warpedAtlasFile);
    if (madeTempDir)
    {
      if (verbosity>1) std::cout<<"removing directory "<<createdTempPath<<std::endl;
      if (!removedir(createdTempPath))
      {
        std::cerr<<"encountered error attempting to remove "<<createdTempPath<<std::endl;
      }
    }
  }
}

bool CerebrumLabelerQt::extractBrain(Vol3DBase *volume, Vol3D<uint8> &vMask)
{
  if (verbosity>1)
    std::cout<<"Initializing brain volume"<<std::endl;
  if (volume)
  {
    Vol3D<uint16> vMaskedMRI;
    if (maskData(vMaskedMRI,volume,vMask,verbosity))
    {
      if (!vMaskedMRI.write(brainFile))
      {
        std::cerr<<"error: unable to write to "<<brainFile<<std::endl;
        return false;
      }
      state = AlignLinear;
      return true;
    }
  }
  return false;
}

template <class T>
DSPoint computeCentroid(Vol3D<T> &vIn)
{
  const int cx=vIn.cx;
  const int cy=vIn.cy;
  const int cz=vIn.cz;
  uint64 xsum=0;
  uint64 ysum=0;
  uint64 zsum=0;
  uint64 nz=0;
  for (int z=0;z<cz;z++)
    for (int y=0;y<cy;y++)
      for (int x=0;x<cx;x++)
      {
        if (vIn(x,y,z))
        {
          xsum+=(x+1);
          ysum+=(y+1);
          zsum+=(z+1);
          nz++;
        }
      }
  return DSPoint(xsum*vIn.rx/nz,ysum*vIn.ry/nz,zsum*vIn.rz/nz);
}

DSPoint computeCentroid(Vol3DBase *volume)
{
  DSPoint centroid=DSPoint(volume->cx*volume->rx/2.0f,volume->cy*volume->ry/2.0f,volume->cz*volume->rz/2.0f);
  switch (volume->typeID())
  {
    case SILT::Uint8 : centroid=computeCentroid(*(Vol3D<uint8> *)volume); break;
    case SILT::Sint8 : centroid=computeCentroid(*(Vol3D<sint8> *)volume); break;
    case SILT::Sint16 : centroid=computeCentroid(*(Vol3D<sint16> *)volume); break;
    case SILT::Uint16 : centroid=computeCentroid(*(Vol3D<uint16> *)volume); break;
    case SILT::Uint32 : centroid=computeCentroid(*(Vol3D<uint32> *)volume); break;
    case SILT::Sint32 : centroid=computeCentroid(*(Vol3D<sint32> *)volume); break;
    case SILT::Float32 : centroid=computeCentroid(*(Vol3D<float32> *)volume); break;
    case SILT::Float64 : centroid=computeCentroid(*(Vol3D<float64> *)volume); break;
    default:
      std::cout<<"error: unable to process type "<<volume->datatypeName();
      if (volume->filename.empty()==false) std::cout<<" ("<<volume->filename<<")";
      std::cout<<std::endl;
  }
  return centroid;
}

bool CerebrumLabelerQt::writeInitializer(std::string ofname, Vol3DBase *vSubject, Vol3DBase *vAtlas)
{
  DSPoint subjectCentroid = ::computeCentroid(vSubject);
  DSPoint atlasCentroid=::computeCentroid(vAtlas);
  std::ofstream ofile(ofname.c_str());
  double sx = vSubject->rx / vAtlas->rx;
  double sy = vSubject->ry / vAtlas->ry;
  double sz = vSubject->rz / vAtlas->rz;
  double tx = ((vAtlas->cx-1) - (vSubject->cx-1)*sx)/2;
  double ty = ((vAtlas->cy-1) - (vSubject->cy-1)*sy)/2;
  double tz = ((vAtlas->cz-1) - (vSubject->cz-1)*sz)/2;

  tx = atlasCentroid.x - subjectCentroid.x * sx;
  ty = atlasCentroid.y - subjectCentroid.y * sy;
  tz = atlasCentroid.z - subjectCentroid.z * sz;
  if (!ofile) { std::cerr<<"unable to open "<<ofname<<std::endl; return false; }
  ofile<<sx<<" 0 0 "<<tx<<"\n";
  ofile<<"0 "<<sy<<" 0 "<<ty<<"\n";
  ofile<<"0 0 "<<sz<<" "<<tz<<"\n";
  return true;
}

bool CerebrumLabelerQt::alignLinear(Vol3DBase *volume, std::string &atlasName)
{
  if (!fileExists(cerebroHelper))
  {
    std::cerr<<"cerebrohelper not found (expected location: "<<cerebroHelper<<")";
    return false;
  }
  if (verbosity>0)
  {
    std::cout<<"Performing linear alignment of atlas brain to subject brain"<<std::endl;
  }
  centroidInitializerFile=tempDirectory+"centroid.init";
  if (useCentroids)
  {
    Vol3DBase *vAtlas = VolumeLoader::load(atlasBrain);
    if (!vAtlas)
    {
      std::cerr<<"unable to load atlas "<<atlasBrain<<std::endl;
      return false;
    }
    if (!writeInitializer(centroidInitializerFile,volume,vAtlas))
    {
      useCentroids=false;
      std::cout<<"failed to create centroid initializer"<<std::endl;
    }
  }
  alignedAtlasFile = tempDirectory + "aligned_atlas.img";
  QStringList arguments;
  initFile=tempDirectory+"air.init";
  arguments<<"--step"<<"1"
           <<"-i"<<brainFile.c_str()
           <<"-o"<<alignedAtlasFile.c_str()
           <<"-c"<<QString::number(costFunction)
           <<"--linconv"<<QString::number(linearConvergence)
           <<"--atlas"<<atlasBrain.c_str()
           <<"--atlaslabels"<<atlasTemplate.c_str()
           <<"--air"<<atlasToBrainAirFile.c_str()
           <<"--initfile"<<initFile.c_str();
  if (verbosity)
    arguments<<"-v"<<"2";
  if (useCentroids)
    arguments<<"--centroids"<<"--centinit"<<centroidInitializerFile.c_str();
  bool flag=::runProgram(cerebroHelper,"align linear",arguments);
  if (!flag)
  {
    std::cerr<<"error in CerebrumLabeler module: alignlinear process failed."<<std::endl;
    return false;
  }
  atlasName=alignedAtlasFile;
  state = AlignWarp;
  return true;
}

bool CerebrumLabelerQt::alignWarp(std::string &ofname)
{  
  warpedAtlasFile=tempDirectory + "warped_atlas.img";
  warpedLabelFile=tempDirectory + "warped.label.img";
  QStringList arguments;
  arguments<<"--step"<<"2"
           <<"-i"<<brainFile.c_str()
           <<"--atlas"<<atlasBrain.c_str()
           <<"--atlaslabels"<<atlasTemplate.c_str()
           <<"-c"<<QString::number(costFunction)
           <<"--initfile"<<(tempDirectory+"air.init").c_str()
           <<"--air"<<atlasToBrainAirFile.c_str()
           <<"--warp"<<atlasToBrainWarpFile.c_str()
           <<"--warplevel"<<QString::number(warpLevel)
           <<"--warpconv"<<QString::number(warpConvergence)
           <<"-o"<<warpedAtlasFile.c_str()
           <<"-l"<<warpedLabelFile.c_str();
  bool flag=::runProgram(cerebroHelper,"align warp",arguments);
  if (!flag)
  {
    std::cerr<<"error in CerebrumLabeler module: align_warp process failed."<<std::endl;
    return false;
  }
  ofname=warpedAtlasFile;
  state = ResliceWarp;
  return true;
}

bool CerebrumLabelerQt::labelLobes(Vol3DBase *volume, Vol3D<uint8> &vCerebrumLabels)
{
  Vol3D<uint16> vLabelIn;
  if (!vLabelIn.read(warpedLabelFile))
  {
    std::cerr<<"couldn't read warped label file "<<warpedLabelFile<<std::endl;
    return false;
  }
  vLabelIn.origin = volume->origin;
  vLabelIn.currentOrientation = volume->currentOrientation;
  vCerebrumLabels.makeCompatible(vLabelIn);
  const int ds = vLabelIn.size();
  for (int i=0;i<ds;i++) vCerebrumLabels[i] = (uint8)vLabelIn[i];
  state = Finished;
  return true;
}
