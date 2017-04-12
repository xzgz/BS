// Copyright (C) 2016 The Regents of the University of California and
// the University of Southern California
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

#include <iostream>
#include <BFC/biasfieldcorrector.h>

BiasFieldCorrector::BiasFieldCorrector() :
  histogramRadius(12),
  biasEstimateSpacing(16),
  controlPointSpacing(64),
  splineLambda(0.0001f),
  biasRange(0.5f,1.5f),
  roiType(Ellipsoid),
  debug(false), timer(false), verbosity(1),
  neighborhoodThreshold(1.0f),
  scaleVariance(true),
  varScale(-1.0f), // <= 0 means not specified
  epsilon(0.000001f),
  center(true), smooth(false),
  replaceWithMean(true),
  nBiasPoints(0), maskdata(0),
  correctWholeVolume(false),
  iterative(false),
  needFullField(false),
  outputType(NiftiGZ),
  halt(false)
{
}

BiasFieldCorrector::~BiasFieldCorrector()
{
}

void BiasFieldCorrector::computeSpline()
{
  float ryx = vol.rx/vol.ry;
  float rzx = vol.rx/vol.rz;
  const int cpx = controlPointSpacing;
  const int cpy=(int)(ryx * cpx);
  const int cpz=(int)(rzx * cpx);
  spline.verbosity = verbosity;
  spline.splineLambda = splineLambda;
  spline.create(vol.cx,vol.cy,vol.cz,cpx,cpy,cpz);
  spline.setMeasured(&biasPoints[0]);
  spline.setMask(biasEstimateMask.start());
  spline.stopCount = nBiasPoints;
  spline.cgm();
}

void BiasFieldCorrector::removeBias(Vol3D<uint8> &v)
{
  v.copy(vol);
  spline.removeBiasMasked(v.start(),v.start());
}


void BiasFieldCorrector::computeField(Vol3D<float> &vf)
{
  vf.makeCompatible(vol);
  spline.computeBias(vf.start());
}

void BiasFieldCorrector::computeFieldMasked(Vol3D<float> &vf)
{
  vf.makeCompatible(vol);
  spline.computeBiasMasked(vf.start(),vol.start());
}
