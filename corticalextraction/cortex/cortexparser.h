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

#ifndef CortexParser_H
#define CortexParser_H

#include <argparser.h>

class CortexParser : public ArgParserBase {
public:
  CortexParser() : ArgParserBase("cortex"),
    percentage(50.0f), computeWM(false), computeGM(false), findCerebellum(false),
    ventricleThreshold(1.5f), includeAllSubcorticalAreas(false),
    verbosity(1), timer(false)
  {
    description="cortex extractor\n"
                "This program produces a cortical mask using tissue fraction estimates\n"
                "and a co-registered cerebellum/hemisphere mask.\n"
                "Cortex is part of the BrainSuite collection of tools.\n"
                "For more information, please see: http://brainsuite.org";
    bind("h",lobelabelfilename,"<label file>","hemisphere / lobe label volume",true);
    bind("o",maskfilename,"<mask file>","output structure mask",true);
    bind("f",fractionfilename,"<fraction file>","tissue fraction file (32-bit float)",true);
    bind("p",percentage,"<percentage>","tissue fraction threshold (percentage)");
    bindFlag("w",computeWM,"compute WM/GM boundary");
    bindFlag("g",computeGM,"compute GM/CSF boundary");
    bindFlag("a",includeAllSubcorticalAreas,"include all subcortical areas in WM mask");
    bindFlag("-cerebellum",findCerebellum,"find cerebellum instead of cerebrum",true);
    bind("-vent",ventriclefilename,"<ventricle filename>","save mask file for ventricles",false,true);
    bind("-vthresh",ventricleThreshold,"<ventricle threshold>","ventricle threshold",false,true);
    bind("v",verbosity,"<verbosity>","verbosity level",false);
    bindFlag("-timer",timer,"timing function",false);
  }
  float percentage;
  bool computeWM; // -w
  bool computeGM; // -g
  bool findCerebellum;
  std::string maskfilename;						//
  std::string fractionfilename;				// -f
  std::string lobelabelfilename;			// -h
  std::string ventriclefilename;			// -v
  float ventricleThreshold;						// --vthresh
  bool includeAllSubcorticalAreas;
  int verbosity;
  bool timer;
};

#endif
