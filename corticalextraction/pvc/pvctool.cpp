// Copyright (C) 2016 The Regents of the University of California
//
// Created by David W. Shattuck, Ph.D.
//
// This file is part of PVC.
//
// PVC is free software; you can redistribute it and/or
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

#include <PVC/pvctool.h>
#include <DS/histogram.h>
#include <vol3d.h>
#include <volumescaler.h>
#include <PVC/tissuelabels.h>

PVCTool::Settings::Settings() : spatialPrior(0.1f), threeClass(false), maxICMIterations(100)
{
}

PVCTool::PVCTool(int verbosity) : partialVolumeModel(verbosity), state(PVCTool::MLCassify), verbosity(verbosity)
{
}

bool PVCTool::stepBack(Vol3D<uint8> &/*vLabel*/, Vol3DBase * /*imageVolume*/, const Vol3D<uint8> &/*vMask*/) // go back one step, return false if at beginning
{
  return false;
}

bool PVCTool::stepForward(Vol3D<uint8> &vLabel, Vol3DBase *vImage, const Vol3D<uint8> &vMask)
{
  switch (state)
  {
    case PVCTool::MLCassify : return initializeML(vLabel,vImage,vMask);
    case PVCTool::ICM : icm(vLabel,settings.spatialPrior,settings.maxICMIterations,settings.threeClass); return true;
    case PVCTool::Finished : return false;
  }
  return false;
}

std::string PVCTool::nextStepName()
{
  switch (state)
  {
    case PVCTool::MLCassify : return "ML classifier initialization";
    case PVCTool::ICM : return "MAP classification";
    case PVCTool::Finished : return "finished";
  }
  return "PVCTool : invalid state"; // should be impossible to reach this
}

bool PVCTool::initializeML(Vol3D<uint8> &vLabel, Vol3DBase *volume, const Vol3D<uint8> &vMask)
{
  if ((volume->typeID()==SILT::Uint16)||(volume->typeID()==SILT::Sint16))
  {
    VolumeScaler::scaleToUint8(vb,*(Vol3D<uint16> *)volume);
  }
  else if (volume->typeID()==SILT::Float32)
  {
    VolumeScaler::scaleToUint8(vb,*(Vol3D<float32> *)volume);
  }
  else if (volume->typeID()==SILT::Float64)
  {
    VolumeScaler::scaleToUint8(vb,*(Vol3D<float64> *)volume);
  }
  else if (volume->typeID()==SILT::Uint8)
  {
    vb.copy(*(Vol3D<uint8> *)volume);
  }
  else
  {
    std::cerr<<"Error: unable to classify data of type "<<volume->datatypeName()<<std::endl;
    return false;
  }

  if (vMask.size()>0)
  {
    if (!vMask.isCompatible(vb))
    {
      std::cerr<<"Error: mask does not have the same dimensions as the data volume"<<std::endl;
      return false;
    }
    const int ds = vb.size();
    for (int i=0;i<ds;i++) if (vMask[i]==0) vb[i]=0;
  }
  if (verbosity>1) { std::cout<<"performing initial labeling"<<std::endl; }
  Histogram h;
  h.compute(vb);
  float thetaX[10] = { 10, 150, 200, 20, 0.05f, 0.3f, 0.3f, 0.25f, 0.2f, 0};
  for (int i=0;i<10;i++) theta[i] = thetaX[i];
  std::vector<int>  hist(256);
  std::copy(h.count,h.count + 256,hist.begin());
  hist[0]=0;
  if (verbosity>2) { std::cout<<"parameters: "; for (int i=0;i<10;i++) std::cout<<theta[i]<<'\t'; std::cout<<std::endl; }
  partialVolumeModel.cluster3(hist,theta);
  if (verbosity>2) { std::cout<<"parameters: "; for (int i=0;i<10;i++) std::cout<<theta[i]<<'\t'; std::cout<<std::endl; }
  partialVolumeModel.cluster6(hist,theta);
  if (verbosity>2) { std::cout<<"parameters: "; for (int i=0;i<10;i++) std::cout<<theta[i]<<'\t'; std::cout<<std::endl; }
  partialVolumeModel.classify6(vb,vLabel,theta);
  state = ICM;
  return true;
}

int PVCTool::icm(Vol3D<uint8> &vLabel, const float lambda, const int iterations, bool threeClass)
{
  if (verbosity>2) std::cout<<"three class "<<threeClass<<std::endl;
  int n = partialVolumeModel.ICM(vb,vLabel,theta,lambda,iterations);
  if (threeClass) partialVolumeModel.crisp(vb,vLabel,theta);
  state = Finished;
  return n;
}

void PVCTool::computeFractions(Vol3D<float> &vFractions, Vol3D<uint8> &vb, Vol3D<uint8> &vLabel, float theta[])
{
  vFractions.makeCompatible(vb);
  if (verbosity>1) { std::cout<<"computing tissue fractions"<<std::endl; }
  uint8 *label = vLabel.start();
  uint8 *meas = vb.start();
  const int ds = vb.size();
  const float mc = theta[0];
  const float mg = theta[1];
  const float mw = theta[2];
  const float delta1 = mw - mg;
  const float delta0 = mg - mc;
  for (int i=0;i<ds;i++)
  {
    float f=0;
    switch (label[i])
    {
      case TissueLabels::CO :
      case TissueLabels::CSF:
        f = 1.0f;
        break;
      case TissueLabels::CG :
        {
          f = 1.0f + (meas[i] - mc)/delta0;
          if (f<1) f = 1;
          if (f>2) f = 2;
        }
        break;
      case TissueLabels::GM :
        f = 2.0f;
        break;
      case TissueLabels::WM :
        f = 3.0f;
        break;
      case TissueLabels::GW :
        f = 2.0f + (meas[i] - mg)/delta1;
        if (f<2) f = 2;
        if (f>3) f = 3;
        break;
    }
    vFractions[i] = f;
  }
}
