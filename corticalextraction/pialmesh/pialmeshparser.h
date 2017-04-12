// Copyright (C) 2016 The Regents of the University of California
//
// Created by David W. Shattuck, Ph.D.
//
// This file is part of Pialmesh.
//
// Pialmesh is free software; you can redistribute it and/or
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

#ifndef PialMeshParser_H
#define PialMeshParser_H

#include <argparser.h>

class PialMeshParser : public ArgParser {
public:
  PialMeshParser() : ArgParser("pialmesh"), timer(false), nSteps(10),
    smoothingConstant(0.1f), stepSize(0.1f), searchRadius(2.0f),
    maximumThickness(10.0f), tissueThreshold(1.1f), firstVertex(0), lastVertex(-1), outputFrequency(0)
  {
    bind("f",fracfname,"<fraction file>","floating point (32) tissue fraction image",true);
    bind("n",nSteps,"<n>","number of iterations",false);
    bind("r",searchRadius,"<radius>","search radius",false);
    bind("s",stepSize,"<step size>","step size",false);
    bind("m",maskfname,"<mask file>","restrict growth to mask file region",true);
    bind("-max",maximumThickness,"<thickness>","maximum allowed tissue thickness",false);
    bind("t",tissueThreshold,"<threshold>","tissue threshold",false);
    bind("-first",firstVertex,"<vertex ID>","first vertex",false,true);
    bind("-last",lastVertex,"<vertex ID>","last vertex",false,true);
    bind("-interval",outputFrequency,"<n>","output interval",false,false);
    bind("-prefix",prefix,"<prefix>","prefix for exporting surfaces if interval is set",false,false);
    bind("-smooth",smoothingConstant,"<smoothing constant>","apply Laplacian smoothing",false,false);
    bindFlag("-timer",timer,"show timing",false);
  }
  std::string maskfname;
  std::string fracfname;
  std::string prefix;
  bool timer;
  int nSteps;
  float smoothingConstant;
  float stepSize;
  float searchRadius;
  float maximumThickness;
  float tissueThreshold;
  int firstVertex;
  int lastVertex;
  int outputFrequency;
};

#endif
