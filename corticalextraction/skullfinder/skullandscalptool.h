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


#ifndef SkullAndScalpTool_H
#define SkullAndScalpTool_H

#include <vol3d.h>

class SkullAndScalpTool {
public:
  enum State { Initialize, FindScalp, FindOuterSkull, FindInnerSkull, GenerateSurfaces, Finished };
  SkullAndScalpTool();
  void lessThan(Vol3D<uint8> &vMask, const Vol3D<uint8> &vIn, const uint8 t);
  void applyMask(Vol3D<uint8> &vImage, const Vol3D<uint8> &vMask);
  template <class T> void zerofillMaskedArea(Vol3D<T> &vImage, const Vol3D<uint8> &vMask);
  template <class T> void applyThreshold(Vol3D<uint8> &vol, const T threshold);
  template <class T> void thresholdMaskFromImage(Vol3D<T> &vIn, int threshold);
  template <class T> double nonzeroMeanT(const Vol3D<T> &vs);
  template <class T> double gteThresholdMeanT(const Vol3D<T> &vs, const int threshold); // mean of voxels >= threshold
  template <class T> bool maskToLabel(Vol3D<T> &vLabel, const Vol3D<uint8> &vMask, const int labelID);
  bool initialize(Vol3DBase *volume);
  void autothresh(const Vol3D<uint8> &vMRI);
  bool findScalp();
  bool findInnerSkull();
  bool findOuterSkull();
  Vol3D<uint8> vMRI;
  Vol3D<uint8> vBrainMask;
  Vol3D<uint8> vHeadLabels;
  Vol3D<uint8> vScalp, vOuterSkull, vInnerSkull;
  State state;
  int scalpThreshold;
  int lowerSkullThreshold;
  int upperSkullThreshold;
  int backgroundLabelValue;
  int scalpLabelValue;
  int skullLabelValue;
  int spaceLabelValue;
  int brainLabelValue;
  bool finalOpening;
  int verbosity;
};

#endif
