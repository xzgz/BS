// Copyright (C) 2016 The Regents of the University of California
//
// Created by David W. Shattuck, Ph.D.
//
// This file is part of Dewisp.
//
// Dewisp is free software; you can redistribute it and/or
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
#include <dewisptool.h>
#include <DS/timer.h>

int main(int argc, char *argv[])
{
  DewispTool dewispTool;
  int maxIterations=10;
  bool timer=false;
  ArgParser ap("dewisp");
  ap.description = "dewisp -- removes wispy tendril structures from cortex model binary masks.\n"
                   "It does so based on graph theoretic analysis of connected components,\n"
                   "similar to TCA. Each branch of the structure graph is analyzed to determine\n"
                   "pinch points that indicate a likely error in segmentation that attaches noise\n"
                   "to the image. The pinch threshold determines how many voxels the cross-section\n"
                   "can be before it is considered part of the image.\n\n"
                   "Dewisp is part of the BrainSuite collection of tools.\n"
                   "For more information, please see: http://brainsuite.org";
  ap.bind("t",dewispTool.sizeThreshold,"<threshold>","size threshold");
  ap.bind("n",maxIterations,"<iterations>","maximum number of iterations");
  ap.bindFlag("-timer",timer,"time processing");
  if (!ap.parseAndValidate(argc,argv)) return ap.usage();

  Vol3D<uint8> vIn;
  Vol3D<uint8> vOut;
  if (!vIn.read(ap.ifname)) return CommonErrors::cantRead(ap.ifname);
  vOut.copy(vIn);

  Timer t;
  if (timer)
    t.start();
  if (ap.verbosity>0)
  {
    std::cout<<"using threshold "<<dewispTool.sizeThreshold<<std::endl;
  }
  int dVoxel=0;
  for (int i=0;i<maxIterations;i++)
  {
    int n = dewispTool.loop(vOut,vIn,ap.verbosity);
    if (ap.verbosity>1)
      std::cout<<"Iteration "<<i+1<<"/"<<maxIterations<<". Cumulative changes: "<<n<<std::endl;
    if (n==dVoxel) break;
    dVoxel = n;
  }
  if (timer)
  {
    t.stop();
    std::cout<<"dewisp took "<<t.elapsed()<<std::endl;
  }
  if (!vOut.write(ap.ofname)) return CommonErrors::cantWrite(ap.ofname);
  if (ap.verbosity>0) std::cout<<"wrote "<<ap.ofname<<std::endl;
  return 0;
}
