// Copyright (C) 2016 The Regents of the University of California
//
// Created by David W. Shattuck, Ph.D.
//
// This file is part of Scrubmask.
//
// Scrubmask is free software; you can redistribute it and/or
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

#include <vol3dsimple.h>
#include <argparser.h>
#include "maskscrubber.h"
#include <DS/timer.h>

class ScrubMaskParser : public ArgParserBase {
public:
  ScrubMaskParser() : ArgParserBase("scrubmask"),
    fgThresh(1), bgThresh(1), nIterations(10), verbosity(1), timer(false)
  {
    description="ScrubMask tool.\n"
                "scrubmask filters binary masks to trim loosely connected voxels that may\n"
                "result from segmentation errors and produce bumps on tessellated surfaces.\n"
                "Scrubmask is part of the BrainSuite collection of tools.\n"
                "For more information, please see: http://brainsuite.org";
    bind("i",ifname,"<mask file>","input structure mask file",true,false);
    bind("o",ofname,"<mask file>","output structure mask file",true,false);
    bind("b",bgThresh,"<threshold>","background fill threshold",false,false);
    bind("f",fgThresh,"<threshold>","foreground trim threshold",false,false);
    bind("n",nIterations,"<iterations>","number of iterations",false,false);
    bind("v",verbosity,"<level>","verbosity (0=silent)",false,false);
    bindFlag("-timer",timer,"timing function",false);
    bind("l",lfname,"<label file>","a label file for debugging",false,true);
  }
  std::string ifname;
  std::string ofname;
  std::string lfname;
  int fgThresh;
  int bgThresh;
  int nIterations;
  int verbosity;
  bool timer;
};

int main(int argc, char *argv[])
{
  Timer timer; timer.start();
  ScrubMaskParser parser;
  if (parser.parse(argc,argv)==false) return parser.usage();
  if (parser.validate()==false) return parser.usage();

  Vol3D<uint8> vIn;
  if (vIn.read(parser.ifname)==false)
  {
    std::cerr<<"unable to read "<<parser.ifname<<std::endl;
    return 1;
  }
  MaskScrubber scrubber;

  Vol3D<uint8> vLabel;
  vLabel.copy(vIn);
  const int ds = vLabel.size();
  for (int i=0;i<parser.nIterations;i++)
  {
    for (int j=0;j<ds;j++)
    {
      if (vLabel[j]>0 && vLabel[j]<128) vLabel[j]++;
      if (vLabel[j]<255 && vLabel[j]>128) vLabel[j]--;
    }
    int count = 0;
    if (parser.fgThresh>0) count += (scrubber.filterFGL(vIn,vLabel,parser.fgThresh,parser.verbosity));
    if (parser.bgThresh>0) count += (scrubber.filterBGL(vIn,vLabel,parser.bgThresh,parser.verbosity));
    if (parser.verbosity>0) { std::cout<<"Iteration: "<<i<<"\tchanged: "<<count<<std::endl; }
    if (count==0) break;
  }
  int retcode=0;
  if (!parser.lfname.empty())
  {
    if (vLabel.write(parser.lfname))
    {
      if (parser.verbosity>0) std::cout<<"Wrote label file "<<parser.lfname<<std::endl;
    }
    else
      retcode=CommonErrors::cantWrite(parser.lfname);
  }
  if (vIn.write(parser.ofname))
  {
    if (parser.verbosity>0) std::cout<<"Wrote output mask "<<parser.ofname<<std::endl;
  }
  else
  {
    retcode=CommonErrors::cantWrite(parser.ofname);
  }
  if (parser.timer)
  {
    timer.stop();
    std::cout<<"scrubmask took "<<timer.elapsed()<<std::endl;
  }
  return retcode;
}
