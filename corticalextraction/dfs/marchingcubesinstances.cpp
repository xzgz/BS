// Copyright (C) 2016 The Regents of the University of California
//
// Created by David W. Shattuck, Ph.D.
//
// This file is part of DFS.
//
// DFS is free software; you can redistribute it and/or
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
#include <marchingcubes_t.h>
#include <operators.h>

#define MarchingCubesInstanceOp(T,Op) \
template bool MarchingCubes::cubes(Vol3D<T> &, SILT::Surface &, const Op<T>);

#define MarchingCubesInstance(T) \
MarchingCubesInstanceOp(T,NZ) \
MarchingCubesInstanceOp(T,EQ) \
MarchingCubesInstanceOp(T,GT) \
MarchingCubesInstanceOp(T,LT)

MarchingCubesInstance(sint8 )
MarchingCubesInstance(uint8 )
MarchingCubesInstance(sint16)
MarchingCubesInstance(uint16)
MarchingCubesInstance(sint32)
MarchingCubesInstance(uint32)
MarchingCubesInstance(float32)
MarchingCubesInstance(float64)

