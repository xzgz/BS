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

#include <topofixer.h>
#include <autocrop.h>

TopoFixer::TopoFixer() : verbosity(0), completed(false)
{
}

void TopoFixer::initialize(Vol3D<uint8> &vMask)
{
  autocrop(vSubMask, vMask, 4);
  completed = false;
}


int TopoFixer::fixDefects(Vol3D<uint8> &vMask, int minFix, int maxFix, int delta, int verbosity, std::ostream &outstream)
{
  initialize(vMask);
  if (minFix>maxFix) maxFix = minFix+1;
  if (verbosity>0) outstream<<"Largest correction will be "<<maxFix<<std::endl;
  completed = false;
  for (int i=1;i<maxFix;i++)
  {
    completed = iterate(i,delta,outstream);
    if (completed)
      break;
  }
  completed = wrapUp(vMask,outstream);
  if (!completed)
  {
    outstream<<"warning : topological defects remain in this object."<<std::endl;
  }
  else
  {
    if (verbosity>0)
      outstream<<"finished removing topological handles."<<std::endl;
  }
  return (completed) ? 0 : 1;
}

bool TopoFixer::iterate(int &i, int delta, std::ostream &outstream)
{
  if (verbosity>1) outstream<<"fixing background (size<="<<i<<"):\t";
  int mn = tcaBackground.correctTopology3Axes(vSubMask.vol,i);
  if (verbosity>1) outstream<<std::endl;
  int minFG = -1;
  if (i>=delta)
  {
    if (verbosity>1) outstream<<"fixing foreground (size<="<<i-delta<<"):\t";;
    minFG = topologicalFilter.loopFG(vSubMask.vol,i,delta);
    if (verbosity>1) outstream<<std::endl;
  }
  if (minFG>0)
  {
    if (minFG<mn) mn=minFG;
    if (mn<0) mn=minFG;
  }
  if (mn>0)
  {
    if ((mn-1)>i)
    {
      i = mn-1;
    }
  }
  else
  {
    if (i>delta)
    {
      completed = true;
      return true;
    }
    i = delta;
  }
  return false;
}

bool TopoFixer::wrapUp(Vol3D<uint8> &vMask, std::ostream &outstream)
{
  if (!completed)
  {
    if (verbosity>1)
      outstream<<"Checking Background ";
    int nBG = tcaBackground.checkTopology3Axes(vSubMask.vol);
    if (verbosity==2) outstream<<'\n';
    if (verbosity>1)
      outstream<<"Checking Foreground ";
    int nFG = topologicalFilter.checkTopology3Axes(vSubMask.vol);
    if (verbosity>1) outstream<<'\n';
    completed = ((nBG==0) && (nFG==0));
  }
  vSubMask.expand(vMask);
  return completed;
}
