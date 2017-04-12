// Copyright (C) 2016 The Regents of the University of California and
// the University of Southern California
//
// Created by David W. Shattuck, Ph.D.
//
// This file is part of Skullfinder.
//
// Skullfinder is free software; you can redistribute it and/or
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


#include "skullandscalptool.h"
#include <volumescaler.h>
#include <vbit.h>
#include <DS/runlengthsegmenter.h>
#include <DS/morph32.h>

SkullAndScalpTool::SkullAndScalpTool() :
  state(Initialize), scalpThreshold(0), lowerSkullThreshold (0), upperSkullThreshold(0),
  backgroundLabelValue(0), scalpLabelValue(16),	skullLabelValue(17), spaceLabelValue(18), brainLabelValue(19),
  finalOpening(false),
  verbosity(1)
{
}

void SkullAndScalpTool::lessThan(Vol3D<uint8> &vMask, const Vol3D<uint8> &vIn, const uint8 t)
{
  vMask.makeCompatible(vIn);
  const int ds = vIn.size();
  for (int i=0;i<ds;i++)
    vMask[i] = (vIn[i]<t) ? 255 : 0;
}

void SkullAndScalpTool::applyMask(Vol3D<uint8> &vImage, const Vol3D<uint8> &vMask)
{
  const int ds = vImage.size();
  for (int i=0;i<ds;i++)
  {
    if (vMask[i]==0)
      vImage[i] = 0;
  }
}

template <class T>
void SkullAndScalpTool::zerofillMaskedArea(Vol3D<T> &vImage, const Vol3D<uint8> &vMask)
{
  const int ds = vImage.size();
  for (int i=0;i<ds;i++)
  {
    if (vMask[i])
      vImage[i] = 0;
  }
}

template <class T>
void SkullAndScalpTool::applyThreshold(Vol3D<uint8> &vol, const T threshold)
{
  const int ds = vol.size();
  for (int i=0;i<ds;i++)
    if (vol[i]<threshold) vol[i] = 0;
}

template <class T>
void SkullAndScalpTool::thresholdMaskFromImage(Vol3D<T> &vIn, int threshold)
{
  const int ds = vIn.size();
  for (int i=0;i<ds;i++)
    vIn[i] = (vIn[i]>=threshold) ? 255: 0;
}

template <class T>
double SkullAndScalpTool::nonzeroMeanT(const Vol3D<T> &vs)
{
  int sum=0;
  size_t count=0;
  T *d = vs.start();
  const int ds = vs.size();
  for (int i=0;i<ds;i++) if (d[i]) { sum += d[i]; count++; }
  return double(sum)/count;
}

template <class T>
double SkullAndScalpTool::gteThresholdMeanT(const Vol3D<T> &vs, const int threshold) // mean of voxels >= threshold
{
  int sum=0;
  size_t count=0;
  T *d = vs.start();
  const int ds = vs.size();
  for (int i=0;i<ds;i++) if (d[i]>=threshold) { sum += d[i]; count++; }
  return double(sum)/count;
}

template <class T>
bool SkullAndScalpTool::maskToLabel(Vol3D<T> &vLabel, const Vol3D<uint8> &vMask, const int labelID)
// assigns labelID to vLabel at all non-zero voxels in vMask
{
  if (!vLabel.makeCompatible(vMask))
    return false;
  const int ds = vMask.size();
  for (int i=0;i<ds;i++)
    if (vMask[i]) vLabel[i] = labelID;
  return true;
}

bool SkullAndScalpTool::initialize(Vol3DBase *volume)
{
  switch(volume->typeID())
  {
    case SILT::Uint16: VolumeScaler::scaleToUint8(vMRI,*(Vol3D<uint16> *)volume); break;
    case SILT::Sint16: VolumeScaler::scaleToUint8(vMRI,*(Vol3D<uint16> *)volume); break;
    case SILT::Float32 : VolumeScaler::scaleToUint8(vMRI,*(Vol3D<float32> *)volume); break;
    case SILT::Float64 : VolumeScaler::scaleToUint8(vMRI,*(Vol3D<float64> *)volume); break;
    case SILT::Uint8:
      vMRI.copy(*(Vol3D<uint8> *)volume);
      break;
    default:
      {
        std::cerr<<"unable to classify data of type "<<volume->datatypeName()<<std::endl;
        return false;
      }
  }
  state = FindScalp;
  vHeadLabels.makeCompatible(vMRI);
  vHeadLabels.set(backgroundLabelValue);
  return true;
}

void SkullAndScalpTool::autothresh(const Vol3D<uint8> &vMRI)
{
  if ((lowerSkullThreshold == 0) && (upperSkullThreshold == 0)) // if no thresholds are specified, then calculate them
  {
    lowerSkullThreshold = (int)nonzeroMeanT(vMRI);
    upperSkullThreshold = (int)gteThresholdMeanT(vMRI,lowerSkullThreshold);
  }
  scalpThreshold = abs((upperSkullThreshold + lowerSkullThreshold)/2);
}

bool SkullAndScalpTool::findScalp()
{
  if (!vMRI.isCompatible(vBrainMask))
  {
    std::cerr<<"MRI and mask are not compatible"<<std::endl;
    return false;
  }
  if (verbosity>1)
    std::cout<<"finding scalp"<<std::endl;
  vScalp.copy(vMRI);
  zerofillMaskedArea(vScalp,vBrainMask);
  autothresh(vScalp);
  if (verbosity>1)
    std::cout<<"thresholds are: "<<lowerSkullThreshold<<'\t'<<scalpThreshold<<'\t'<<upperSkullThreshold<<std::endl;
  vScalp.copy(vMRI);
  applyThreshold(vScalp,scalpThreshold);	// threshold the MR intensity image.
  Vol3D<VBit> vBit;				// bit-packed temp volume
  vBit.encode(vScalp);		// loads the byte mask scalp into packed bit format
  RunLengthSegmenter rls;
  Morph32 m32;
  m32.dilateO2(vBit);								// dilates with O2
  rls.segmentBG(vBit);							// floodfills the internal cavities of the object
  m32.erodeO2(vBit);								// erodes with O2
  rls.segmentFG(vBit);							// select the largest foreground component
  if (finalOpening)
  {
    m32.erodeC(vBit);
    m32.erodeC(vBit);
    m32.dilateC(vBit);
    m32.dilateC(vBit);
  }
  vBit.decode(vScalp);	// unpack the bit mask into the byte volume.
  vHeadLabels.set(backgroundLabelValue);
  // copy masks to labels
  maskToLabel(vHeadLabels,vScalp,scalpLabelValue);
  maskToLabel(vHeadLabels,vBrainMask,brainLabelValue);
  state = SkullAndScalpTool::FindOuterSkull;
  return true;
}

bool SkullAndScalpTool::findOuterSkull()
{
  Vol3D<uint8> vScalpOpened;
  Vol3D<VBit> vbitBrain;
  Vol3D<VBit> vbitScalp;
  Vol3D<VBit> vbitOuterSkull;
  Morph32 m32;
  RunLengthSegmenter rls;

  vOuterSkull.copy(vMRI);
  lessThan(vOuterSkull,vMRI,lowerSkullThreshold);	// include only voxels less than the threshold.
  vbitOuterSkull.encode(vOuterSkull);							// encode min mask into packed bit format.
  vbitBrain.encode(vBrainMask);
  vbitScalp.encode(vScalp);
  m32.dilateC(vbitBrain);
  m32.dilateC(vbitBrain);
  opOr(vbitOuterSkull,vbitBrain);
  vbitScalp.encode(vScalp);
  m32.erodeC(vbitScalp);	m32.erodeC(vbitScalp);	m32.erodeC(vbitScalp);
  m32.erodeC(vbitScalp);	m32.erodeC(vbitScalp);	m32.erodeC(vbitScalp);
  m32.erodeC(vbitScalp);	m32.erodeC(vbitScalp);	m32.erodeC(vbitScalp);
  m32.erodeC(vbitScalp);	m32.erodeC(vbitScalp);	m32.erodeC(vbitScalp);
  m32.dilateC(vbitScalp);	m32.dilateC(vbitScalp);	m32.dilateC(vbitScalp);
  m32.dilateC(vbitScalp);	m32.dilateC(vbitScalp);	m32.dilateC(vbitScalp);
  m32.dilateC(vbitScalp);	m32.dilateC(vbitScalp);	m32.dilateC(vbitScalp);
  m32.dilateC(vbitScalp);
  vbitScalp.decode(vScalpOpened);
  opAnd(vbitOuterSkull,vbitScalp);				// don't include background outside of scalp
  rls.segmentFG(vbitOuterSkull);			    // get rid of any background holes or noise.
  m32.dilateO2(vbitOuterSkull);				    // perform close O4 with intermediate background fill (dws: except fill is not intermediate?)
  m32.dilateO2(vbitOuterSkull);				    // dilate O4  ( O2 (+) O2 )
  m32.erodeO2(vbitOuterSkull);						// erode  O4  ( O2 (+) O2 )
  m32.erodeO2(vbitOuterSkull);
  vbitOuterSkull.decode(vOuterSkull);			// unpack the data into the skull volume.
  applyMask(vOuterSkull,vScalpOpened);
  vOuterSkull.copyInfo(vMRI);
  // copy masks to labels
  vHeadLabels.set(backgroundLabelValue);
  maskToLabel(vHeadLabels,vScalp,scalpLabelValue);
  maskToLabel(vHeadLabels,vOuterSkull,skullLabelValue);
  maskToLabel(vHeadLabels,vBrainMask,brainLabelValue);
  state = SkullAndScalpTool::FindInnerSkull;
  return true;
}

bool SkullAndScalpTool::findInnerSkull()
{
  Morph32 m32;
  Vol3D<uint8> vDilatedBrain;
  Vol3D<VBit> vbitDilatedBrain;
  Vol3D<VBit> vbitInnerSkull;
  Vol3D<VBit> vbitOuterSkull;
  Vol3D<VBit> vbitOuterSkullClosed;
  Vol3D<VBit> vbitScalp;
  Vol3D<VBit> vbitSpace;
  Vol3D<uint8> vSpace;

  vInnerSkull.copy(vMRI);
  vDilatedBrain.copy(vBrainMask);
  vbitDilatedBrain.encode(vDilatedBrain);
  vSpace.copy(vOuterSkull);
  vbitOuterSkull.encode(vOuterSkull);
  vbitScalp.encode(vScalp);
  m32.erodeC(vbitOuterSkull); // erode outer Skull
  vbitSpace.decode(vSpace);
  applyMask(vInnerSkull,vSpace);
  thresholdMaskFromImage(vInnerSkull,lowerSkullThreshold);
  vbitInnerSkull.encode(vInnerSkull);
  m32.dilateC(vbitDilatedBrain);
  opOr(vbitInnerSkull,vbitDilatedBrain);
  m32.erodeO2(vbitInnerSkull);
  m32.erodeO2(vbitInnerSkull);
  m32.dilateO2(vbitInnerSkull);
  m32.dilateO2(vbitInnerSkull);
  //impose the skull thickness;
  vbitOuterSkullClosed.encode(vOuterSkull);
  m32.erodeO2(vbitOuterSkullClosed);
  m32.erodeO2(vbitOuterSkullClosed);
  opOr(vbitInnerSkull,vbitOuterSkullClosed);
  vbitInnerSkull.decode(vInnerSkull);
  //correction
  opOr(vbitInnerSkull,vbitDilatedBrain);
  vbitInnerSkull.decode(vInnerSkull);
  m32.dilateC(vbitInnerSkull);
  m32.dilateC(vbitOuterSkull);
  opOr(vbitOuterSkull,vbitInnerSkull);
  vbitOuterSkull.decode(vOuterSkull);
  m32.dilateC(vbitOuterSkull);
  opOr(vbitScalp,vbitOuterSkull);
  vbitScalp.decode(vScalp);
  // copy masks to labels
  vHeadLabels.set(backgroundLabelValue);
  maskToLabel(vHeadLabels,vScalp,scalpLabelValue);
  maskToLabel(vHeadLabels,vOuterSkull,skullLabelValue);
  maskToLabel(vHeadLabels,vInnerSkull,spaceLabelValue);
  maskToLabel(vHeadLabels,vBrainMask,brainLabelValue);
  state = SkullAndScalpTool::GenerateSurfaces;	// used in BrainSuite GUI
  return true;
}
