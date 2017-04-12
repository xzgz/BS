// Copyright (C) 2016 The Regents of the University of California and
// the University of Southern California
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

#include <TCA/tcafixchecker.h>
#include <iostream>
#include <DS/runlengthsegmenter.h>

bool TCAFixChecker::checkMaskFG(Vol3D<uint8> &vm, int /*slice*/)
{
  RunLengthSegmenter checkerRLS;
  Vol3D<VBit> vbit;
  vbit.encode(vm);
  checkerRLS.mode = RunLengthSegmenter::D6;
  checkerRLS.segmentFG(vbit);
  return true;
}

bool TCAFixChecker::checkMaskBG(Vol3D<uint8> &vm, int /*slice*/)
// checks if the volume is disconnected into multiple pieces
{
  Vol3D<VBit> vb;
  vb.encode(vm);

  RunLengthSegmenter rls2;
  rls2.mode = RunLengthSegmenter::D18;
  rls2.segmentBG(vb);
  int bg = rls2.nRegions();
  std::cout<<"bg checker: "<<bg<<std::endl;
  if (bg>1)
  {
    for (int i=1;i<bg;i++)
    {
      std::cout<<'\t'<<i<<'\t'<<rls2.regionCount(i)<<std::endl;
    }
    for (int i=1;i<bg;i++)
      if (rls2.regionCount(i)>0)
        return false;
  }
  return true;
}
