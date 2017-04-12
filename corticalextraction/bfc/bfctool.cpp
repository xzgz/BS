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

#include <BFC/bfctool.h>
#include <volumescaler.h>

std::string BFCTool::nextStepName() const
{
  switch (state)
  {
    case BFCTool::Init : return "Initialize";
    case BFCTool::ComputePoints : return "Compute pointwise spline estimates";
    case BFCTool::ComputeSpline : return "Estimate correction field";
    case BFCTool::RemoveBias : return "Apply correction";
    case BFCTool::Finished : return "Finished";
  }
  return "invalid step"; // impossible
}

std::string BFCTool::runningStepName() const
{
  switch (state)
  {
    case BFCTool::Init : return "initializing";
    case BFCTool::ComputePoints : return "computing pointwise bias estimates";
    case BFCTool::ComputeSpline : return "estimating correction field";
    case BFCTool::RemoveBias : return "applying correction";
    case BFCTool::Finished : return "finished";
  }
  return "invalid step"; // impossible
}

bool BFCTool::removeBias(Vol3DBase *vIn)
{
  switch (vIn->typeID())
  {
    case SILT::Uint8 :
      bfc.removeBias(*(Vol3D<uint8> *)vIn);
      return true;
    case SILT::Sint16 :
    case SILT::Uint16 :
      bfc.removeBias(*(Vol3D<uint16> *)vIn);
      return true;
    default:
      break;
  }
  state = BFCTool::Finished;
  return false;
}

bool BFCTool::removeBias(Vol3DBase *vIn, Vol3D<uint8> &vMask)
{
  switch (vIn->typeID())
  {
    case SILT::Uint8 :
      {
        Vol3D<uint8> &v = *(Vol3D<uint8> *)vIn;
        v.maskWith(vMask);
        bfc.removeBias(v);
      }
      state = BFCTool::Finished;
      return true;
    case SILT::Sint16 :
    case SILT::Uint16 :
      {
        Vol3D<uint16> &v = *(Vol3D<uint16> *)vIn;
        v.maskWith(vMask);
        bfc.removeBias(v);
      }
      state = BFCTool::Finished;
      return true;
      break;
    case SILT::Float32 :
      {
        Vol3D<float32> &v = *(Vol3D<float32> *)vIn;
        v.maskWith(vMask);
        bfc.removeBias(v);
      }
      state = BFCTool::Finished;
      return true;
      break;
    default: break;
  }
  return false;
}

bool BFCTool::initialize(Vol3DBase *volume, Vol3D<uint8> &vMask)
{
  if (bfc.initialize(volume,vMask)==false) return false;
  state = BFCTool::ComputePoints;
  return true;
}

bool BFCTool::computePointwiseBiasEstimates()
{
  bfc.computeBiasPoints();
  state = BFCTool::ComputeSpline;
  return true;
}

bool BFCTool::estimateCorrectionField()
{
  bfc.computeSpline();
  state = BFCTool::RemoveBias;
  return true;
}
