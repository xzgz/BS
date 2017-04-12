// Copyright (C) 2016 The Regents of the University of California
//
// Created by David W. Shattuck, Ph.D.
//
// This file is part of Vol3D.
//
// Vol3D is free software; you can redistribute it and/or
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

#include <volumeloader.h>
#include <vol3dquery.h>
#include <vol3d_t.h>
#include <vbit.h>

Vol3DBase *VolumeLoader::load(std::string ifname)
{
  Vol3DBase *volume = 0;
  Vol3DQuery vq;
  if (vq.query(ifname)==false) return 0;
  if (vq.headerType == HeaderType::DICOM) return 0; // DICOM not currently supported
  switch (vq.datatype)
  {
    case SILT::Uint8						: volume = new Vol3D<uint8>; break;
    case SILT::Sint8						: volume = new Vol3D<signed char>; break;
    case SILT::Uint16						: volume = new Vol3D<unsigned short>; break;
    case SILT::Sint16						: volume = new Vol3D<signed short>; break;
    case SILT::Uint32						: volume = new Vol3D<unsigned int>; break;
    case SILT::Sint32						: volume = new Vol3D<signed int>; break;
    case SILT::Float32					: volume = new Vol3D<float>; break;
    case SILT::Float64					: volume = new Vol3D<double>; break;
    case SILT::RGB8							: volume = new Vol3D<rgb8>; break;
    case SILT::Eigensystem3x3f	: volume = new Vol3D<EigenSystem3x3f>; break;
    case SILT::Vector3F					: volume = new Vol3D<DSPoint>; break;
    case SILT::Unknown :
    default:
      std::cerr<<"File datatype ("<<vq.datatype<<")is unknown."<<std::endl;
      return 0;
      break;
  }
  if (volume) volume->read(vq,Vol3DBase::RotateToRAS);
  return volume;
}

Vol3DInstance(sint8)
Vol3DInstance(uint8)
Vol3DInstance(sint16)
Vol3DInstance(uint16)
Vol3DInstance(sint32)
Vol3DInstance(uint32)
Vol3DInstance(float32)
Vol3DInstance(float64)
Vol3DInstance(EigenSystem3x3f)
Vol3DInstance(rgb8)

template bool Vol3D<VBit>::read(std::string, Vol3DBase::AutoRotateCode);
template bool Vol3D<VBit>::read(const Vol3DQuery &, Vol3DBase::AutoRotateCode);
template bool Vol3D<VBit>::copyCast(Vol3DBase *&dest) const;
