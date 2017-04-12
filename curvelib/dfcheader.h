// Copyright (C) 2016 The Regents of the University of California
//
// Created by David W. Shattuck, Ph.D.
//
// This file is part of BrainSuite16a1.
//
// BrainSuite16a1 is free software; you can redistribute it and/or
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

#ifndef DFCHeader_H
#define DFCHeader_H

#include <vol3ddatatypes.h>
#include <iostream>
#define DFCMAGIC "DFC_LE\0\0"

class Version {
public:
  Version(const uint8 a_=1, const uint8 b_=0, const uint8 c_=0, const uint8 d_=0) :
    a(a_), b(b_), c(c_), d(d_) { }
  uint8 a,b,c,d;
};

class DFCHeader {
// curves are stored in real world coordinates.
public:
  DFCHeader() :
    version(1,0,0,3),
    hdrsize(sizeof(DFCHeader)),
    dataStart(0),
    mdoffset(0),
    pdoffset(0),
    nContours(0)
  {
    const char *DFCMagic=DFCMAGIC;
    std::fill(magic,magic+sizeof(magic),0);
    size_t np=sizeof(DFCMagic);
    if (np>=sizeof(magic)) np=np-1;
    for (std::size_t i=0;i<np;i++)
      magic[i]=DFCMagic[i];
  }
  char magic[8];     // 0 - 7
  Version version;   // 8 - 11
  uint32  hdrsize;   // 12 - 15
  uint32  dataStart; // 16 - 19
  uint32  mdoffset;  // 20 - 23
  uint32  pdoffset;  // 24 - 27
  uint32  nContours; // 28 - 31
};

#endif
