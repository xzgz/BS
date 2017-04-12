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

#include <vol3d.h>
#include <vbit.h>
#include <vol3dquery.h>
#include "cortexparser.h"
#include <cortexmodeler.h>
#include <DS/timer.h>
#include <vol3dsimple.h>

template <class T>
bool typeconvert(Vol3D<uint8> &volume, std::string ifname)
{
  Vol3D<T> vOriginal;
  if (!vOriginal.read(ifname)) return false;
  volume.makeCompatible(vOriginal);
  const int ds=vOriginal.size();
  for (int i=0;i<ds;i++)
    volume[i] = (uint8)vOriginal[i]; // TODO: ignores range problems
  return true;
}

bool readAs8bit(Vol3D<uint8> &volume, std::string ifname)
{
  Vol3DQuery vq;
  if (!vq.query(ifname)) return false;
  switch (vq.datatype)
  {
    case SILT::Uint8  :
    case SILT::Sint8  : return volume.read(ifname);
    case SILT::Uint16 : return typeconvert<uint16>(volume,ifname);
    case SILT::Sint16 : return typeconvert<sint16>(volume,ifname);
    default:
      break; // TODO : unhandled data types
  }
  std::cerr<<"cannot read datatype "<<vq.datatype<<std::endl;
  return false;
}

int main(int argc, char *argv[])
{
  Timer timer; timer.start();
  CortexParser parser;
  if (!parser.parseAndValidate(argc,argv)) return parser.usage();

  Vol3D<uint8> vMask;
  Vol3D<float> vFractions;
  Vol3D<uint8> vLabeledLobes;

  if (!readAs8bit(vLabeledLobes,parser.lobelabelfilename))
  {
    std::cerr<<"error reading "<<parser.lobelabelfilename<<std::endl;
    return 1;
  }
  if (vFractions.read(parser.fractionfilename)==false)
  {
    std::cerr<<"error reading "<<parser.fractionfilename<<std::endl;
    return 1;
  }
  if (vFractions.isCompatible(vLabeledLobes)==false)
  {
    std::cerr<<"Error: fraction file and hemisphere label file have different dimensions."<<std::endl;
    return 1;
  }
  CortexModeler cm;
  CortexModeler::SurfaceType surfaceType = (parser.findCerebellum) ? CortexModeler::Cerebellum : CortexModeler::Cerebrum;
  cm.verbosity = parser.verbosity;
  cm.thresh = parser.percentage / 100.0f;
  int retcode = 0;
  if (vFractions.size()>0)
  {
    cm.findVentricles(surfaceType, vFractions, cm.vVentricles, vLabeledLobes,parser.ventricleThreshold);
  }
  if (!parser.ventriclefilename.empty())
  {
    if (cm.vVentricles.write(parser.ventriclefilename))
    {
      if (parser.verbosity>0)
        std::cout<<"Wrote ventricle file "<<parser.ventriclefilename<<std::endl;
    }
    else
    {
      retcode |= ::CommonErrors::cantWrite(parser.ventriclefilename);
    }
  }
  if (parser.computeWM)
  {
    cm.whiteMatterFromFractions(surfaceType, vFractions, vMask, vLabeledLobes, parser.includeAllSubcorticalAreas);
  }
  else
  {
    cm.greyMatterFromFractions(surfaceType, vFractions, vMask, vLabeledLobes);
  }
  if (vMask.write(parser.maskfilename))
  {
    if (parser.verbosity>0)
      std::cout<<"Wrote cortex mask file "<<parser.maskfilename<<std::endl;
  }
  else
  {
    std::cerr<<"error writing "<<parser.maskfilename<<std::endl;
    return 1;
  }
  if (parser.timer)
  {
    timer.stop();
    std::cout<<"cortex took "<<timer.elapsed()<<std::endl;
  }
  return 0;
}
