// Copyright (C) 2016 The Regents of the University of California
//
// Created by David W. Shattuck, Ph.D.
//
// This file is part of ScrubMask.
//
// ScrubMask is free software; you can redistribute it and/or
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

#ifndef MaskScrubber_H
#define MaskScrubber_H

#include <vol3d.h>

class MaskScrubber {
public:
	int filterFG(Vol3D<uint8> &vIn, const int fgThresh);
	int filterBG(Vol3D<uint8> &vIn, int bgThresh);
	int filterFGL(Vol3D<uint8> &vIn, Vol3D<uint8> &vLabel, const int fgThresh, const int verbosity);
	int filterBGL(Vol3D<uint8> &vIn, Vol3D<uint8> &vLabel, const int bgThresh, const int verbosity);
};

#endif
