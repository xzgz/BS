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

#include <surflib.h>
#include <vol3d.h>
#include <brainsplitter.h>
#include <DS/timer.h>
#include "hemisplitparser.h"
#include <vol3dlib.h>

int main(int argc, char *argv[])
{
  Timer timer; timer.start();
  HemisplitParser parser;
  if (!parser.parseAndValidate(argc,argv)) return parser.usage();
  Surface surf,left,right,pialSurface;

  if (surf.read(parser.ifname)==false)
  {
    return CommonErrors::cantRead(parser.ifname);
  }
  if (surf.nt()==0)
  {
    std::cerr<<"Error: surface has no triangles."<<std::endl;
    return 1;
  }
  if (surf.nv()==0)
  {
    std::cerr<<"Error: surface has no vertices."<<std::endl;
    return 1;
  }
  if (parser.pialSurfaceFile.empty()==false)
  {
    if (!pialSurface.read(parser.pialSurfaceFile))
      return CommonErrors::cantRead(parser.pialSurfaceFile);
    if (pialSurface.nv()!=surf.nv())
    {
      std::cerr<<"Input surface and pial surface must have equal number of vertices."<<std::endl;
      return 1;
    }
  }
  if (parser.verbosity>1) std::cout<<"read "<<surf.filename<<std::endl;
  Vol3D<uint8> vLabel;
  if (vLabel.read(parser.lfname)==false)
  {
    std::cerr<<"unable to read "<<parser.lfname<<std::endl;
    return 1;
  }
  if (parser.verbosity>1) std::cout<<"read "<<vLabel.filename<<std::endl;
  std::vector<int> leftMap;
  std::vector<int> rightMap;
  BrainSplitter::split(surf,vLabel,left,right,leftMap,rightMap);

  if (left.write(parser.ofnameL)==false)
  {
    std::cerr<<"unable to write "<<parser.ofnameL<<std::endl;
    return 1;
  }
  if (parser.verbosity>0) std::cout<<"Wrote "<<parser.ofnameL<<std::endl;
  if (right.write(parser.ofnameR)==false)
  {
    std::cerr<<"unable to write "<<parser.ofnameR<<std::endl;
    return 1;
  }
  if (parser.verbosity>0) std::cout<<"Wrote "<<parser.ofnameR<<std::endl;
  if (parser.timer) { timer.stop(); std::cout<<"hemisplit took "<<timer.elapsed()<<std::endl; }

  if (pialSurface.nv()==surf.nv())
  {
    if (parser.verbosity>0) std::cout<<"mapping vertices."<<std::endl;
    BrainSplitter::mapVertices(left,pialSurface,leftMap);
    BrainSplitter::mapVertices(right,pialSurface,rightMap);
    left.computeNormals();
    right.computeNormals();
    if (left.write(parser.pfnameL)==false)
    {
      std::cerr<<"unable to write "<<parser.pfnameL<<std::endl;
      return 1;
    }
    if (parser.verbosity>0) std::cout<<"Wrote "<<parser.pfnameL<<std::endl;
    if (right.write(parser.pfnameR)==false)
    {
      std::cerr<<"unable to write "<<parser.pfnameR<<std::endl;
      return 1;
    }
    if (parser.verbosity>0) std::cout<<"Wrote "<<parser.pfnameR<<std::endl;
    if (parser.timer) { timer.stop(); std::cout<<"hemisplit took "<<timer.elapsed()<<std::endl; }
  }
  return 0;
}
