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

#include <vol3dsimple.h>
#include <BFC/biasfieldcorrector.h>
#include <DS/timer.h>
#include <volumeloader.h>
#include <vol3dutils.h>
#include <volumedivisor.h>
#include "bfcparser.h"

int main(int argc, char *argv[])
{
  Timer t; t.start();
  BiasFieldCorrector bfc;
  bfc.iterative = false;
  bfc.roiType = BiasFieldCorrector::Block;
  BFCParser parser(bfc);
  if (parser.parseAndValidate(argc,argv)==false) return parser.usage();
  if (bfc.iterative)
  {
    if (bfc.roiType == BiasFieldCorrector::Block)
    {
      std::cerr<<"setting blocktype to ellipsoid for iterative processing mode"<<std::endl;
      bfc.roiType = BiasFieldCorrector::Ellipsoid;
    }
  }
  Vol3DBase *vIn = VolumeLoader::load(parser.ifname);
  if (!vIn) return CommonErrors::cantRead(parser.ifname);
  Vol3D<uint8> vMask;
  if (!parser.mfname.empty())
  {
    if (!vMask.read(parser.mfname))
      return CommonErrors::cantRead(parser.ifname);
    if (!vIn->dimensionsMatch(vMask))
    {
      std::cerr<<"dimensions of "<<vMask.filename<<" and "<<vIn->filename<<" differ."<<std::endl;
      return 1;
    }
  }
  if (bfc.verbosity>1)
  {
    std::cout
        <<"histogram radius:             "<<bfc.histogramRadius<<'\n'
        <<"bias estimate sample spacing: "<<bfc.biasEstimateSpacing<<'\n'
        <<"control point spacing:        "<<bfc.controlPointSpacing<<'\n'
        <<"spline stiffness multiplier:  "<<bfc.splineLambda<<'\n'
        <<"allowed bias estimate range:  ["<<bfc.biasRange.first<<','<<bfc.biasRange.second<<"]\n";
  }
  bfc.needFullField = (!parser.bfname.empty())||bfc.correctWholeVolume;
  Vol3D<float> vSpline;
  if (parser.correctionSchedule.empty()==false)
  {
    if (!bfc.readSchedule(parser.correctionSchedule))
      return CommonErrors::cantRead(parser.correctionSchedule);
    bfc.iterative=true;
  }

  if (bfc.iterative)
  {
    bfc.performAllIterations(vIn,vMask,vSpline);
  }
  else
  {
    bfc.correct(vSpline,vIn,vMask);
  }
  // output the results
  int retcode = 0;
  if (vIn->write(parser.ofname))
  {
    if (bfc.verbosity>0) std::cout<<"Wrote corrected image "<<parser.ofname<<std::endl;
  }
  else
  {
    retcode = CommonErrors::cantWrite(parser.ofname);
  }
  if (!parser.bfname.empty())
  {
    if (vSpline.write(parser.bfname))
    {
      if (bfc.verbosity>0) std::cout<<"Wrote bias field image "<<parser.bfname<<std::endl;
    }
    else
    {
      retcode |= CommonErrors::cantWrite(parser.bfname);
    }
  }
  if (!parser.mbfname.empty())
  {
    if (bfc.needFullField)
    {
      if (vMask.isCompatible(vSpline))
        vSpline.maskWith(vMask);
      else
        vSpline.maskWith(bfc.vol);
    }
    if (vSpline.write(parser.mbfname))
    {
      if (bfc.verbosity>0) std::cout<<"Wrote masked bias field image "<<parser.mbfname<<std::endl;
    }
    else
    {
      retcode |= CommonErrors::cantWrite(parser.mbfname);
    }
  }

  if (bfc.timer)
  {
    t.stop();
    std::cout<<"BFC took "<<t.elapsed()<<std::endl;
  }
  return 0;
}

