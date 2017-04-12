// Copyright (C) 2016 The Regents of the University of California
//
// Created by David W. Shattuck, Ph.D.
//
// This file is part of TCA.
//
// TCA is free software; you can redistribute it and/or
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

#ifndef TCAParser_H
#define TCAParser_H

#include <argparser.h>

class TCAParser : public ArgParserBase {
public:
  enum CorrectionMode { FGBG = 0, BGOnly = 1, FGOnly = 2 };
  TCAParser() : ArgParserBase("tca"),
    minFix(1), maxFix(200), delta(0),
    mode(FGBG), verbosity(1), timer(false)
  {
    copyright = "Copyright (C) 2016 The Regents of the University of California and the University of Southern California";
    description="topological correction algorithm (TCA)\n"
                "This program removes topological handles from a binary object.\n"
                "Algorithm created by David W Shattuck and Richard M Leahy\n"
                "TCA is part of the BrainSuite collection of tools.\n"
                "For more information, please see: http://brainsuite.org";
    bind("i",ifname,"<input filename>","input mask volume",true);
    bind("o",ofname,"<output filename>","output mask volume",true);
    bind("m",maxFix,"<n>","maximum correction size",false);
    bind("n",minFix,"<n>","minimum correction size",false);
    bind("-delta",delta,"<delta>","foreground delta",false);
    bind("v",verbosity,"<level>","verbosity (0 = quiet)",false,false);
    bindFlag("-timer",timer,"timing function",false);
  }
  std::string ifname; // -i
  std::string ofname; // -o
  int minFix;					// -n
  int maxFix;					// -m
  int delta; // --delta
  CorrectionMode mode;
  int verbosity;			// -v
  bool timer;
};

#endif
