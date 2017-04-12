// Copyright (C) 2016 The Regents of the University of California and
// the University of Southern California
//
// Created by David W. Shattuck, Ph.D.
//
// This file is part of BFC.
//
// BFC is free software; you can redistribute it and/or
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

#include <BFC/regularizedsplinef.h>
#include <BFC/bfcmessages.h>

void RegularizedSplineF::computeBias(float *bias)
// compute the bias field for the volume.
{
  int uz=0;
  int cpz=3;
  int index = 0;
  if (verbosity>1)
  {
    BFCMessages::describe("Computing Full Bias Field");
  }
  int z=0;
  for (z=0;z<cz;z++,uz++)
  {
    if (verbosity>1) BFCMessages::dot();
    if (uz>=dz) { uz -= dz; cpz++;}
    int uy=0;
    int cpy=3;
    for (int y=0;y<cy;y++,uy++)
    {
      if (uy>=dy) { uy -= dy; cpy++;}
      int ux=0;
      int cpx=3;
      for (int x=0;x<cx;x++,ux++)
      {
        if (ux>=dx) { ux -= dx; cpx++;}
        float32 val = 0.0f;
        for (int q=0;q<4;q++)
        {
          for (int r=0;r<4;r++)
          {
            float32 f = bz[q][uz] * by[r][uy];
            int pos = (cpz-q)*nx*ny + (cpy-r)*nx + cpx;
            for (int s=0;s<4;s++)
            {
              val += CP [pos--] * f * bx[s][ux];
            }
          }
        }
        bias[index] = val;
        index++;
      }
    }
  }
  if (verbosity>1) BFCMessages::reset();
}
