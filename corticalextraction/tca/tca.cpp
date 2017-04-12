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

#include <DS/timer.h>
#include <vol3dsimple.h>
#include "tcaparser.h"
#include <topofixer.h>

int main(int argc, char *argv[])
{
  Timer timer; timer.start();
  TCAParser parser;
  if (!parser.parseAndValidate(argc,argv)) return parser.usage();
  Vol3D<uint8> vMask;
  if (parser.minFix>parser.maxFix) parser.maxFix = parser.minFix+1;
  if (vMask.read(parser.ifname)==false)
  {
    std::cout<<"unable to open input file "<<parser.ifname<<std::endl;
    return 1;
  }
  if (parser.verbosity>0) std::cout<<"Largest correction will be "<<parser.maxFix<<std::endl;
  bool completed = false;
  TopoFixer topoFixer;
  topoFixer.verbosity=parser.verbosity;
  topoFixer.initialize(vMask);
  for (int i=1;i<parser.maxFix;i++)
  {
    std::cout<<"Fixing topological defects of size "<<i<<std::endl;
    completed = topoFixer.iterate(i,parser.delta,std::cout);
    if (completed) break;
  }
  completed = topoFixer.wrapUp(vMask,std::cout);
  if (!completed)
  {
    std::cerr<<"warning : topological defects remain in this object."<<std::endl;
  }
  else
  {
    if (parser.verbosity>1)
      std::cout<<"finished removing topological handles."<<std::endl;
  }
  int retcode=0;
  if (vMask.write(parser.ofname))
  {
    if (parser.verbosity>1)
      std::cout<<"wrote topologically corrected mask "<<parser.ofname<<std::endl;
  }
  else
  {
    retcode = CommonErrors::cantWrite(parser.ofname);
  }
  timer.stop();
  if (parser.timer) std::cout<<"tca took "<<timer.elapsed()<<std::endl;
  return retcode;
}
