// Copyright (C) 2016 The Regents of the University of California
//
// Created by David W. Shattuck, Ph.D.
//
// This file is part of Skullfinder.
//
// Skullfinder is free software; you can redistribute it and/or
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

#ifndef SkullFinder_H
#define SkullFinder_H

#include <vol3d.h>
#include <DS/morph32.h>
#include <DS/runlengthsegmenter.h>

class SkullFinder {
public:
  SkullFinder();
  bool findScalp(const Vol3D<uint8> &vIn, const Vol3D<uint8> &vBrain);
  bool findOuterSkull(const Vol3D<uint8> &vIn, const Vol3D<uint8> &vBrain);
  bool findInnerSkull(const Vol3D<uint8> &vIn, const Vol3D<uint8> &vBrain);
  bool process(Vol3D<uint8> &vIn, Vol3D<uint8> &vBrain);
  int autothresh(const Vol3D<uint8> &vMRI);//, int &lowerthr, int &upperthr);
  void setDifference(Vol3D<uint8> &v, const Vol3D<uint8> &br);
  void masking(Vol3D<uint8> &v, const Vol3D<uint8> &mask);
  void absDifference(Vol3D<uint8> &v1, const Vol3D<uint8> &v2);
  void lessThan(Vol3D<uint8> &vMask, const Vol3D<uint8> &vIn, const uint8 t);
  void suggest(const Vol3D<uint8> &vIn, const Vol3D<uint8> &vBrain);
  template <class T>
  void thresh(Vol3D<T> &vol, int threshval);
  template <class T>
  void applyThreshold(Vol3D<uint8> &vol, const T threshold);

  RunLengthSegmenter rls;
  Morph32 m32;

  Vol3D<VBit> vbitScalp, vbitoutSkull, vbitinSkull, vbitBrain, vbitOpen_Scalp;
  Vol3D<uint8> vScalp, vOuterSkull, vInnerSkull, diffout_in, diffbrain, diffscalp;
  Vol3D<uint8> vmidscalp;
  int scalpThreshold;
  int lowerSkullThreshold;
  int upperSkullThreshold;

};

#endif
