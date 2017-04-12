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

#include <limits.h>
#include <BFC/clampcopy.h>

bool ClampCopy::vcopy(Vol3D<float> &vOut, Vol3DBase *vIn)
{
  switch (vIn->typeID())
  {
    case SILT::Sint8:   return copy(vOut,*(Vol3D<uint8> *)vIn); break;
    case SILT::Uint8:   return copy(vOut,*(Vol3D<uint8> *)vIn); break;
    case SILT::Sint16:  return copy(vOut,*(Vol3D<sint16> *)vIn); break;
    case SILT::Uint16:  return copy(vOut,*(Vol3D<uint16> *)vIn); break;
    case SILT::Sint32:  return copy(vOut,*(Vol3D<sint16> *)vIn); break;
    case SILT::Uint32:  return copy(vOut,*(Vol3D<uint16> *)vIn); break;
    case SILT::Float32: return copy(vOut,*(Vol3D<float32> *)vIn); break;
    case SILT::Float64: return copy(vOut,*(Vol3D<float64> *)vIn); break;
    default:
      std::cerr<<"ClampCopy cannot process data of type "<<vIn->datatypeName()<<'\n';
      return false;
  }
}

bool ClampCopy::vcopy(Vol3DBase *vOut, Vol3D<float> &vIn)
{
  switch (vOut->typeID())
  {
    case SILT::Sint8:   return copy(*(Vol3D<uint8> *)vOut,vIn); break;
    case SILT::Uint8:   return copy(*(Vol3D<uint8> *)vOut,vIn); break;
    case SILT::Sint16:  return copy(*(Vol3D<sint16> *)vOut,vIn); break;
    case SILT::Uint16:  return copy(*(Vol3D<uint16> *)vOut,vIn); break;
    case SILT::Sint32:  return copy(*(Vol3D<sint16> *)vOut,vIn); break;
    case SILT::Uint32:  return copy(*(Vol3D<uint16> *)vOut,vIn); break;
    case SILT::Float32: return copy(*(Vol3D<float32> *)vOut,vIn); break;
    case SILT::Float64: return copy(*(Vol3D<float64> *)vOut,vIn); break;
    default:
      std::cerr<<"BFC cannot process data of type "<<vOut->datatypeName();
      return false;
  }
  return true;
}
