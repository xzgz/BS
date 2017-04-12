// Copyright (C) 2016 The Regents of the University of California
//
// Created by David W. Shattuck, Ph.D.
//
// This file is part of Cortex.
//
// Cortex is free software; you can redistribute it and/or
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

#include "cortexmodeler.h"
#include <vbit.h>
#include <DS/runlengthsegmenter.h>
#include <DS/morph32.h>

bool CortexModeler::findVentricles(SurfaceType surfaceType, Vol3D<float> &vFractions, Vol3D<uint8> &vMask, Vol3D<uint8> &vLabeledLobes, float ventricleThreshold)
{
  greyMatterFromFractions(surfaceType,vFractions,vMask,vLabeledLobes);
  const int ds = vMask.size();
  for (int i=0;i<ds;i++)
  {
    if (vFractions[i]>ventricleThreshold) vMask[i] = 0;
  }
  Vol3D<VBit> vFill;
  vFill.encode(vMask);
  RunLengthSegmenter rls;
  rls.segmentFG(vFill);
  rls.segmentBG(vFill);
  vFill.decode(vMask);
  return true;
}

void CortexModeler::formPHWM(Vol3D<uint8> &vPHWM,Vol3D<uint8> &vLabel, Vol3D<float32> &vFractions)
// parahippocampal white matter
{
  vPHWM.makeCompatible(vLabel);
  vPHWM.set(0);
  Morph32 m32;
  const int ds=vLabel.size();
  for (int i=0;i<ds;i++)
  {
    switch (vLabel[i])
    {
      case 15:
      case 16:
        vPHWM[i] = (vFractions[i]>2) ? 255 : 0;
        break;
      default:
        break;
    }
  }
  Vol3D<VBit> vBit;
  vBit.encode(vPHWM);
  m32.dilateC(vBit);
  m32.dilateR(vBit);
  m32.erodeC(vBit);
  m32.erodeR(vBit);
  vBit.decode(vPHWM);
}

bool CortexModeler::whiteMatterFromFractions(SurfaceType surfaceType, Vol3D<float> &vFractions, Vol3D<uint8> &vMask, Vol3D<uint8> &vLabeledLobes, bool includeAllSubcorticalAreas)
{
  errorString = "";
  bool flag = false;
  std::ostringstream ostr;
  if (vFractions.size()==0)
  {
    ostr<<"Tissue fractions have not been computed.\n";
    flag = true;
  }
  if (vLabeledLobes.size()==0)
  {
    ostr<<"Hemisphere labeling has not been performed.\n";
    flag = true;
  }
  if (vLabeledLobes.isCompatible(vFractions)==false)
  {
    if (!flag)
    {
      ostr<<"Tissue fraction volume is not compatible with hemisphere label volume.\n";
      flag = true;
    }
  }
  errorString = ostr.str();
  if (flag) return false;
  vMask.makeCompatible(vFractions);
  uint8 *mask = vMask.start();
  float *fracs = vFractions.start();
  const int ds = vFractions.size();
  const float tissueThreshold = 2.0f + thresh;
  const float lowerThreshold = 1.5f; // don't include non-GM
  int subcorticalDefault = (includeAllSubcorticalAreas) ? 255 : 0;
  if (surfaceType==Cerebrum)
  {
    Vol3D<uint8> vPHWM; // parahippocampal white matter
    formPHWM(vPHWM,vLabeledLobes,vFractions);
    for (int i=0;i<ds;i++)
    {
      switch (vLabeledLobes[i])
      {
        case 7:
        case 8: mask[i] = 0; break;
        case 1:
        case 2:
          {
            mask[i] = (fracs[i]>=tissueThreshold) ? 255 : 0;
          }
          break;
        case 5:
        case 6:
          {
            mask[i] = (fracs[i]>=lowerThreshold) ? 255 : subcorticalDefault;
          }
          break;
        case 15:
        case 16:
          {
            mask[i] = (fracs[i]>2) ? 255 : 0;
          }
          break;
        case 0:
        case 3:
        case 4:
        default: mask[i] = 0; break;
      }
      if (vPHWM[i]) mask[i]=255;
    }
    if (vVentricles.isCompatible(vMask))
    {
      const int nv = vVentricles.size();
      Vol3D<VBit> vVent;
      vVent.encode(vVentricles);
      Morph32 m32;
      m32.dilateR(vVent);
      Vol3D<uint8> vM2;
      vVent.decode(vM2);
      for (int i=0;i<nv;i++) vMask[i] |= vM2[i];
    }
  }
  else
  {
    for (int i=0;i<ds;i++)
    {
      switch (vLabeledLobes[i])
      {
        case 3:
        case 4:
          mask[i] = (fracs[i]>=tissueThreshold) ? 255 : 0;
          break;
        case 0:
        case 1:
        case 2:
        case 5:
        case 6:
        case 7:
        case 8:
        default: mask[i] = 0; break;
      }
    }
  }
  Vol3D<VBit> vFill;
  vFill.encode(vMask);
  RunLengthSegmenter rls;
  rls.segmentFG(vFill);
  rls.segmentBG(vFill);
  vFill.decode(vMask);
  return true;
}

bool CortexModeler::greyMatterFromFractions(SurfaceType surfaceType, Vol3D<float> &vFractions, Vol3D<uint8> &vMask, Vol3D<uint8> &vLabeledLobes)
{
  if (vLabeledLobes.size()!=vFractions.size()) return false;
  vMask.makeCompatible(vFractions);
  uint8 *mask = vMask.start();
  uint8 *cereb = vLabeledLobes.start();
  float *fracs = vFractions.start();
  const int ds = vFractions.size();
  const float tissueThreshold = 1.0f + thresh;
  if (surfaceType==Cerebrum)
  {
    for (int i=0;i<ds;i++)
    {
      switch (cereb[i])
      {
        case 1:
        case 2:
          {
            mask[i] = (fracs[i]>=tissueThreshold) ? 255 : 0;
          }
          break;
        case 15:
        case 16:
          {
            mask[i] = (fracs[i]>=tissueThreshold) ? 255 : 0;
          }
          break;
        case 5:
        case 6:
          {
            mask[i] = 255; break;
          }
        case 0:
        case 3:
        case 4:
        case 7:
        case 8:
        default: mask[i] = 0; break;
      }
    }
  }
  else
  {
    for (int i=0;i<ds;i++)
    {
      switch (cereb[i])
      {
        case 3:
        case 4:
          {
            mask[i] = (fracs[i]>=tissueThreshold) ? 255 : 0;
          }
          break;
        case 1:
        case 2:
        case 5:
        case 6:
        case 7:
        case 8:
        case 0:
        default: mask[i] = 0; break;
      }
    }
  }
  Vol3D<VBit> vFill;
  vFill.encode(vMask);
  RunLengthSegmenter rls;
  rls.segmentFG(vFill);
  rls.segmentBG(vFill);
  vFill.decode(vMask);
  return true;
}
