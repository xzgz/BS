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

#include "maskscrubber.h"

int MaskScrubber::filterFG(Vol3D<uint8> &vIn, const int fgThresh)
{
  const int sx = vIn.cx-1;
  const int sy = vIn.cy-1;
  const int sz = vIn.cz-1;
  int gcount = 0;
  for (int z=1;z<sz;z++)
    for (int y=1;y<sy;y++)
      for (int x=1;x<sx;x++)
      {
        if (vIn(x,y,z))
        {
          int count = (vIn(x+1,y,z)!=0)+(vIn(x-1,y,z)!=0)+
                      (vIn(x,y+1,z)!=0)+(vIn(x,y-1,z)!=0)+(vIn(x,y,z+1)!=0)+(vIn(x,y,z-1)!=0);
          if (count<fgThresh)
          {
            gcount++;
            vIn(x,y,z)=0;
          }
        }
      }
  std::cout<<"changed "<<gcount<<std::endl;
  return gcount;
}

int MaskScrubber::filterBG(Vol3D<uint8> &vIn, int bgThresh)
{
  const int sx = vIn.cx-1;
  const int sy = vIn.cy-1;
  const int sz = vIn.cz-1;
  int gcount = 0;
  for (int z=1;z<sz;z++)
    for (int y=1;y<sy;y++)
      for (int x=1;x<sx;x++)
      {
        if (!vIn(x,y,z))
        {
          int count = (vIn(x+1,y,z)==0)+(vIn(x-1,y,z)==0)+
                      (vIn(x,y+1,z)==0)+(vIn(x,y-1,z)==0)+(vIn(x,y,z+1)==0)+(vIn(x,y,z-1)==0);
          if (count<bgThresh)
          {
            gcount++;
            vIn(x,y,z)=255;
          }
        }
      }
  std::cout<<"changed "<<gcount<<std::endl;
  return gcount;
}


int MaskScrubber::filterFGL(Vol3D<uint8> &vIn, Vol3D<uint8> &vLabel, const int fgThresh, const int verbosity)
{
  const int sx = vIn.cx-1;
  const int sy = vIn.cy-1;
  const int sz = vIn.cz-1;
  int gcount = 0;
  for (int z=1;z<sz;z++)
    for (int y=1;y<sy;y++)
      for (int x=1;x<sx;x++)
      {
        if (vIn(x,y,z))
        {
          int count = (vIn(x+1,y,z)!=0)+(vIn(x-1,y,z)!=0)+
                      (vIn(x,y+1,z)!=0)+(vIn(x,y-1,z)!=0)+(vIn(x,y,z+1)!=0)+(vIn(x,y,z-1)!=0);
          if (count<fgThresh)
          {
            gcount++;
            vIn(x,y,z)=0;
            vLabel(x,y,z)--;
          }
        }
      }
  if (verbosity>1) { std::cout<<"FG changed "<<gcount<<std::endl; }
  return gcount;
}

int MaskScrubber::filterBGL(Vol3D<uint8> &vIn, Vol3D<uint8> &vLabel, const int bgThresh, const int verbosity)
{
  const int sx = vIn.cx-1;
  const int sy = vIn.cy-1;
  const int sz = vIn.cz-1;
  int gcount = 0;
  for (int z=1;z<sz;z++)
    for (int y=1;y<sy;y++)
      for (int x=1;x<sx;x++)
      {
        if (!vIn(x,y,z))
        {
          int count = (vIn(x+1,y,z)==0)+(vIn(x-1,y,z)==0)+
                      (vIn(x,y+1,z)==0)+(vIn(x,y-1,z)==0)+(vIn(x,y,z+1)==0)+(vIn(x,y,z-1)==0);
          if (count<bgThresh)
          {
            gcount++;
            vIn(x,y,z)=255;
            vLabel(x,y,z)++;
          }
        }
      }
  if (verbosity>1) { std::cout<<"BG changed "<<gcount<<std::endl; }
  return gcount;
}

