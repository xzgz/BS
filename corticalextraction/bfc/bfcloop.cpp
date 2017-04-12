// Copyright (C) 2016 The Regents of the University of California
//
// Created by David W. Shattuck, Ph.D.
//
// This file is part of BFC.
//
// BFC is free software; you can redistribute it and/or
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

#include <vol3d.h>
#include <BFC/biasfieldcorrector.h>
#include <DS/timer.h>
#include <volumeloader.h>
#include <vol3dutils.h>
#include <volumedivisor.h>
#include <BFC/clampcopy.h>
#include <commonerrors.h>

bool BiasFieldCorrector::correct(Vol3D<float> &vSpline, Vol3DBase *vIn, Vol3D<uint8> &vMask)
{
  if (!initialize(vIn,vMask)) return 1;
  computeBiasPoints();
  if (halt) return false;
  computeSpline();
  if (halt) return false;
  Timer splineTimer; splineTimer.start();
  if (needFullField)
    computeField(vSpline);
  else
    computeFieldMasked(vSpline);
  if (halt) return false;
  if (timer && verbosity > 0)
  {
    splineTimer.stop();
    std::cout<<"spline computation took "<<splineTimer.elapsed()<<std::endl;
  }
  bool divideOkay = false;
  if (!correctWholeVolume)
  {
    divideOkay = VolumeDivisor::divideByMasked(vIn,vSpline,vol);
  }
  else
    divideOkay = VolumeDivisor::divideBy(vIn,vSpline);
  if (divideOkay)
    return true;
  std::cerr<<"Error: an error occurred while applying the correction to the image."<<std::endl;
  return false;
}

bool BiasFieldCorrector::initializeIterations(Vol3DBase * /*vIn*/, Vol3D<uint8> &/*vMask*/)
{
  return true;
}

bool BiasFieldCorrector::performNextIteration(Vol3D<float32> &vWorking, Vol3D<float> &vSpline, Vol3D<uint8> &vMask)
{
  if (verbosity>1)
    std::cout<<"performing next iteration"<<std::endl;
  if (scheduleIterator==schedule.end()) return false;
  biasEstimateSpacing = scheduleIterator->sampleSpacing;
  controlPointSpacing = scheduleIterator->controlPointSpacing;
  histogramRadius = scheduleIterator->radius;
  splineLambda = scheduleIterator->stiffness;
  if (verbosity>1)
  {
    if (iterationNumber()>0) std::cout<<'\n';
    std::cout<<"Performing iteration "<<iterationNumber()+1<<" ("
             <<scheduleIterator->radius<<'/'
             <<scheduleIterator->sampleSpacing<<'/'
             <<scheduleIterator->controlPointSpacing<<'/'
             <<scheduleIterator->stiffness<<')'
             <<std::endl;
  }
  correct(vSpline,&vWorking,vMask);
  if (!imagePrefix.empty())
  {
    std::ostringstream ofname;
    ofname<<imagePrefix<<'.'<<iterationNumber()+1<<"."<<outputSuffix();
    if (vWorking.write(ofname.str()))
    {
      if (verbosity>1) std::cout<<"Wrote image file "<<ofname.str()<<std::endl;
    }
    else
    {
      CommonErrors::cantWrite(ofname.str());
    }
  }
  if (!biasPrefix.empty())
  {
    std::ostringstream ofname;
    ofname<<biasPrefix<<'.'<<iterationNumber()+1<<".field."<<outputSuffix();
    if (vSpline.write(ofname.str()))
    {
      if (verbosity>1) std::cout<<"Wrote image file "<<ofname.str()<<std::endl;
    }
    else
    {
      CommonErrors::cantWrite(ofname.str());
    }
  }
  iterationNo++;
  scheduleIterator++;
  return true;
}

bool BiasFieldCorrector::resetIterations()
{
  halt = false;
  if (schedule.size()==0) loadDefaultSchedule();
  scheduleIterator = schedule.begin();
  iterationNo = 0;
  return true;
}

bool BiasFieldCorrector::performAllIterations(Vol3DBase *vIn, Vol3D<uint8> &vMask, Vol3D<float> &vSpline)
{
  if (schedule.size()==0) loadDefaultSchedule();

  Vol3D<float32> vWorking;
  ClampCopy::vcopy(vWorking,vIn);
  int i=0;
  for (BiasFieldCorrector::ScheduleIterator iter = schedule.begin(); iter != schedule.end(); iter++, i++)
  {
    biasEstimateSpacing = iter->sampleSpacing;
    controlPointSpacing = iter->controlPointSpacing;
    histogramRadius = iter->radius;
    splineLambda = iter->stiffness;
    if (verbosity>0)
    {
      if (i>0) std::cout<<'\n';
      std::cout<<"Performing iteration "<<i+1<<" ("
               <<iter->radius<<'/'
               <<iter->sampleSpacing<<'/'
               <<iter->controlPointSpacing<<'/'
               <<iter->stiffness<<')'
               <<std::endl;
    }
    correct(vSpline,&vWorking,vMask);
    if (!imagePrefix.empty())
    {
      std::ostringstream ofname;
      ofname<<imagePrefix<<'.'<<i+1<<"."<<outputSuffix();
      if (vWorking.write(ofname.str()))
      {
        if (verbosity>1) std::cout<<"Wrote image file "<<ofname.str()<<std::endl;
      }
      else
      {
        CommonErrors::cantWrite(ofname.str());
      }
    }
    if (!biasPrefix.empty())
    {
      std::ostringstream ofname;
      ofname<<biasPrefix<<'.'<<i+1<<".field."<<outputSuffix();
      if (vSpline.write(ofname.str()))
      {
        if (verbosity>1) std::cout<<"Wrote image file "<<ofname.str()<<std::endl;
      }
      else
      {
        CommonErrors::cantWrite(ofname.str());
      }
    }
  }
  ClampCopy::vcopy(vIn,vWorking);
  return true;
}
