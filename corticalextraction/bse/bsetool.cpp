// Copyright (C) 2016 The Regents of the University of California
//
// Created by David W. Shattuck, Ph.D.
//
// This file is part of BSE.
//
// BSE is free software; you can redistribute it and/or
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

#include <BSE/bsetool.h>
#include <DS/timer.h>
#include <vol3d.h>
#include <marrhildrethedgedetector.h>
#include <volumescaler.h>
#include <vol3dops.h>
#include "anisotropicdiffusionfilter.h"

BSETool::Settings::Settings() :
  diffusionIterations(3), diffusionConstant(25),
  edgeConstant(0.64f), erosionSize(1), removeBrainstem(false),
  verbosity(1)
{
}

BSETool::BSETool(): bseState(ADFilter)
{
}

void BSETool::doAll(Vol3D<uint8> &maskVolume, Vol3DBase *&referenceVolume, const Vol3DBase *volume)
{
  Timer t;
  t.start();
  for (int i=0;i<6;i++)
  {
    bool flag = false;
    switch (bseState)
    {
      case BSETool::ADFilter   : flag = stepForward(maskVolume, referenceVolume, volume); break;
      case BSETool::EdgeDetect : flag = stepForward(maskVolume, referenceVolume, volume); break;
      case BSETool::FindBrain  : flag = stepForward(maskVolume, referenceVolume, volume); break;
      case BSETool::FinishBrain: flag = stepForward(maskVolume, referenceVolume, volume); break;
      case BSETool::Finished   : flag = false; break;
      default: flag = false; break;
    }
    if (!flag) break;
  }
  t.stop();
  if (settings.verbosity>0)
  {
    std::cout<<"BSE took "<<t.elapsed()<<std::endl;
  }
}

std::string BSETool::nextStepName()
{
  switch (bseState)
  {
    case BSETool::ADFilter : return "anisotropic diffusion filter";
    case BSETool::EdgeDetect : return "edge detection";
    case BSETool::FindBrain : return "find initial brain mask";
    case BSETool::FinishBrain : return "refine brain mask";
    case BSETool::Finished : return "skull stripping completed!";
    default :  return ""; // impossible case
  }
}

bool BSETool::stepForward(Vol3D<uint8> &maskVolume, Vol3DBase *&referenceVolume, const Vol3DBase *volume)
{
  bool retcode = false;
  if (volume)
  {
    switch (bseState)
    {
      case BSETool::ADFilter :
        {
          if (settings.verbosity>1) { std::cout<<"Performing anisotropic diffusion filter"<<std::endl; }
          retcode = initialize(referenceVolume, volume);
          if (retcode) referenceVolume->filename = StrUtil::getBasename(volume->filename) + "[filtered]";
        }
        break;
      case BSETool::EdgeDetect :
        if (settings.verbosity>1) { std::cout<<"Performing edge detection"<<std::endl; }
        retcode = edgeDetect(maskVolume,referenceVolume,settings.edgeConstant);
        break;
      case BSETool::FindBrain :
        if (settings.verbosity>1) { std::cout<<"Finding brain"<<std::endl; }
        retcode = findBrain(maskVolume,volume,settings.erosionSize);
        break;
      case BSETool::FinishBrain:
        if (settings.verbosity>1) { std::cout<<"Finishing brain"<<std::endl; }
        retcode = finishBrain(maskVolume,settings.erosionSize,settings.removeBrainstem);
        break;
      case BSETool::Finished :
        break;
      default:
        errorMessage = "error: unrecognized state in BSETool";
        return false;
    }
  }
  return retcode;
}

bool BSETool::stepBack(Vol3D<uint8> &maskVolume, Vol3DBase *&/*referenceVolume*/, const Vol3DBase * /*volume*/)
// go back one step, return false if already at beginning
{
  switch (bseState)
  {
    case BSETool::ADFilter    : return false;
    case BSETool::EdgeDetect  :
      bseState = BSETool::ADFilter;
      break;
    case BSETool::FindBrain   :
      bseState = BSETool::EdgeDetect;
      break;
    case BSETool::FinishBrain :
      bseState = BSETool::FindBrain;
      edgemask.decode(maskVolume);
      break;
    case BSETool::Finished    :
      bseState = BSETool::FinishBrain;
      break;
    default:
      return false;
  }
  return true;
}

bool BSETool::finishBrain(Vol3D<uint8> &maskVolume, const int erosionSize, bool removeBrainstem)
{
  if (settings.verbosity>2)
  {
    std::cout<<"segmenting background"<<std::endl;
  }
  Vol3D<VBit> vBit;
  vBit.encode(maskVolume);
  RunLengthSegmenter rls;
  rls.segmentBG(vBit);
  if (settings.verbosity>2)
  {
    std::cout<<"dilating with operator size "<<erosionSize<<std::endl;
  }
  for (int i=0;i<erosionSize;i++) morphology.dilateR(vBit);
  if (settings.verbosity>2)
  {
    std::cout<<"closing"<<std::endl;
  }
  morphology.dilateO2(vBit);
  rls.segmentFG(vBit);
  rls.segmentBG(vBit);
  morphology.erodeO2(vBit);

  if (settings.verbosity>2)
  {
    std::cout<<"decoding"<<std::endl;
  }
  vBit.decode(maskVolume);
  if (settings.verbosity>2)
  {
    std::cout<<"finished"<<std::endl;
  }
  bseState = Finished;

  if (removeBrainstem)
  {
    if (settings.verbosity>1)
    {
      std::cout<<"Removing brainstem."<<std::endl;
    }
    stemTrim(maskVolume);
  }
  if (settings.dilateFinalMask)
  {
    if (settings.verbosity>1) { std::cout<<"Dilating final mask."<<std::endl; }
    Vol3D<VBit> vm0;
    vm0.encode(maskVolume);
    morphology.dilateR(vm0);
    vm0.decode(maskVolume);
  }
  if (saveCortex)
  {
    vCortex.copy(vBit);
    morphology.dilateR(vCortex);
    setDifference(vCortex,vBit);
  }
  return true;
}

void BSETool::stemTrim(Vol3D<uint8> &vmask, int nOpen, int nDilate)
{
  Vol3D<uint8> vtrimmed;
  Morph32 m32;
  RunLengthSegmenter rls;
  Vol3D<VBit> vm0;
  vm0.encode(vmask);
  for (int i=0;i<nOpen;i++) m32.erodeO2(vm0);
  rls.segmentFG(vm0);
  for (int i=0;i<nOpen;i++) m32.dilateO2(vm0);
  for (int i=0;i<nDilate;i++) morphology.dilateO2(vm0);
  vm0.decode(vtrimmed);
  mask(vmask,vtrimmed);
}

bool BSETool::findBrain(Vol3D<uint8> &maskVolume, const Vol3DBase *volume, int erosionSize)
{
  if (!volume)
  {
    errorMessage="error: empty volume passed to brain finding algorithm.";
    return false;
  }
  initBrain.encode(maskVolume);
  morphology.setup(initBrain);
  if (settings.verbosity>1)
  {
    std::cout<<"eroding with operator size "<<erosionSize<<std::endl;
  }
  for (int i=0;i<erosionSize;i++) morphology.erodeR(initBrain);
  if (settings.verbosity>1)
  {
    std::cout<<"segmenting foreground"<<std::endl;
  }
  int firstlabel = runLengthSegmenter.segmentFG(initBrain);

  Vol3D<uint8> vm;
  initBrain.decode(vm);
  // check the top 3 regions to see if they are reasonably bright.
  // this avoids selecting large regions of noisy background
  bool failed = true;
  int labeled = firstlabel;
  float globalMean = (float)Vol3DOps::mean(volume);
  for (int i=0;i<10;i++)
  {
    float roiMean = (float)Vol3DOps::mean(volume,vm);
    if (roiMean>globalMean) { failed = false; break; }
    runLengthSegmenter.regionInfo[labeled].selected = 0;
    labeled++;
    runLengthSegmenter.regionInfo[labeled].selected = 1;
    runLengthSegmenter.label32FG(initBrain);
    initBrain.decode(vm);
  }
  if (failed)
  {
    runLengthSegmenter.regionInfo[labeled].selected = 0;
    runLengthSegmenter.regionInfo[firstlabel].selected = 1;
    runLengthSegmenter.label32FG(initBrain);
  }
  bseState = FinishBrain;
  initBrain.decode(maskVolume);
  return true;
}

bool BSETool::edgeDetect(Vol3D<uint8> &maskVolume, const Vol3DBase *referenceVolume, const float edgeConstant)
{
  switch (referenceVolume->typeID())
  {
    case SILT::Uint8  : marrHildrethEdgeDetection(maskVolume,(Vol3D<uint8> *)referenceVolume,edgeConstant); bseState=FindBrain; break;
    case SILT::Sint8  : marrHildrethEdgeDetection(maskVolume,(Vol3D<sint8> *)referenceVolume,edgeConstant); bseState=FindBrain; break;
    case SILT::Uint16 : marrHildrethEdgeDetection(maskVolume,(Vol3D<uint16> *)referenceVolume,edgeConstant); bseState=FindBrain; break;
    case SILT::Sint16 : marrHildrethEdgeDetection(maskVolume,(Vol3D<sint16> *)referenceVolume,edgeConstant); bseState=FindBrain; break;
    default:
      errorMessage = "error: datatype ("+referenceVolume->datatypeName()+") is not currently supported for BSE.";
      std::cerr<<errorMessage<<std::endl;
      return false;
  }
  edgemask.encode(maskVolume);
  return true;
}

template <class T>
void BSETool::marrHildrethEdgeDetection(Vol3D<uint8> &vMask, Vol3D<T> *vIn, const float sigma)
{
  MarrHildrethEdgeDetector<T> mh;
  mh.sigma = sigma;
  mh.detect(*vIn,vMask);
}

bool BSETool::initialize(Vol3DBase *& referenceVolume, const Vol3DBase *volume)
{
  switch (volume->typeID())
  {
    case SILT::Uint8 :
    case SILT::Sint8 :  break;
    case SILT::Uint16 : VolumeScaler::scaleToUint8(vBuf,*(Vol3D<uint16> *)volume); volume = &vBuf; break;
    case SILT::Sint16 : VolumeScaler::scaleToUint8(vBuf,*(Vol3D<sint16> *)volume); volume = &vBuf; break;
    case SILT::Float32 : VolumeScaler::scaleToUint8(vBuf,*(Vol3D<float32> *)volume); volume = &vBuf; break;
    case SILT::Float64 : VolumeScaler::scaleToUint8(vBuf,*(Vol3D<float64> *)volume); volume = &vBuf; break;
    default:
      errorMessage = "error: datatype ("+volume->datatypeName()+") is not currently supported for BSE.";
      std::cout<<errorMessage<<std::endl;
      return false;
  }
  if (volume->typeID()!=SILT::Uint8 && volume->typeID()!=SILT::Sint8)
  {
    errorMessage = "error: datatype ("+volume->datatypeName()+") is not currently supported for BSE.";
    return false; // this is impossible, but serves as a reminder for later changes in the code
  }
  adf(referenceVolume,(Vol3D<uint8> *)volume,settings.diffusionIterations,settings.diffusionConstant,settings.verbosity);
  bseState=EdgeDetect;
  return true;
}

void BSETool::adf(Vol3DBasePtr &ref, Vol3D<uint8> *vol, const int n, const float c, int verbosity)
{

  Vol3D<uint8> *result=0;
  if (ref)
  {
    if (ref->typeID()==vol->typeID()) result = (Vol3D<uint8> *)ref;
    else { delete ref; ref = 0; }
  }
  if (!result)
  {
    result = new Vol3D<uint8>;
    if (verbosity>1) std::cout<<"made new reference volume"<<std::endl;
  }
  if (n==0)
  {
    result->copy(*vol);
  }
  else
  {
    AnisotropicDiffusionFilter f(n,c);
    f.filter(*result,*vol,verbosity);
  }
  ref = result;
}
