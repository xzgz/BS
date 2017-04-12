// Copyright (C) 2016 The Regents of the University of California
//
// Created by David W. Shattuck, Ph.D.
//
// This file is part of Hemisplit.
//
// Hemisplit is free software; you can redistribute it and/or
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

#ifndef HemisplitParser_H
#define HemisplitParser_H

#include <argparser.h>

class HemisplitParser : public ArgParserBase {
public:
  HemisplitParser() : ArgParserBase("hemisplit"), verbosity(1), timer(false)
  {
    description=
        "Hemisphere splitter.\n"
        "Hemisplit is part of the BrainSuite collection of tools.\n"
        "For more information, please see: http://brainsuite.org";
    notes="Splits a surface object into two separate surfaces given an input label volume.\n"
          "Each vertex is labeled left or right based on the labels being odd (left) or even (right).\n"
          "The largest contour on the split surface is then found and used as the separation between\n"
          "left and right.";

    bind("i",ifname,"<surface>","input surface",true);
    bind("l",lfname,"<volume>","input hemisphere label volume",true);
    bind("-left", ofnameL,"<surface>","output surface file, left hemisphere",true);
    bind("-right",ofnameR,"<surface>","output surface file, right hemisphere",true);
    bind("p",pialSurfaceFile,"<input_pial_surface>","pial surface file -- must have same geometry as input surface",false);
    bind("pl", pfnameL,"<surface>","output pial surface file, left hemisphere",false,true);
    bind("-pialleft", pfnameL,"<surface>","output pial surface file, left hemisphere",false);
    bind("pr",pfnameR,"<surface>","output pial surface file, right hemisphere",false,true);
    bind("-pialright",pfnameR,"<surface>","output pial surface file, right hemisphere",false);

    bind("v",verbosity,"<level>","verbosity (0 = silent)");
    bindFlag("-timer",timer,"timing function",false);
  }
  std::string ifname;  // input surface
  std::string ofnameL; // left surface
  std::string ofnameR; // right surface
  std::string lfname;  // label file
  std::string pialSurfaceFile;
  std::string pfnameL; // left surface
  std::string pfnameR; // right surface
  int verbosity;
  bool timer;
};

#endif
