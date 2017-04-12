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

void RegularizedSplineF::removeBiasMasked(uint8 *img, uint8 *imask)
// Divides the img data by the spline specified by the control points.
// Masked by imask.
{
  int uz=0;
  int cpz=3;
  int index = 0;
  if (verbosity>1)
  {
    std::cout<<"Removing bias"<<std::endl;
  }
  for (int z=0;z<cz;z++,uz++)
  {
    if (verbosity>1) { std::cout<<'.'; std::cout.flush(); }
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
        uint8 v = 0;
        if (imask[index]!=0)
        {
          float32 val = 0.0f;
          for (int q=0;q<4;q++)
            for (int r=0;r<4;r++)
            {
              float32 f = bz[q][uz] * by[r][uy];
              for (int s=0;s<4;s++)
              {
                val += CP [(cpz-q)*nx*ny + (cpy-r)*nx + (cpx-s)]
                    * f * bx[s][ux];
              }
            }
          v = dclamp(img[index]/val);
        }
        img[index] = v;
        index++;
      }
    }
  }
  if (verbosity>1) std::cout<<std::endl;
}

void RegularizedSplineF::removeBiasMasked(short *img, uint8 *imask)
// Divides the img data by the spline specified by the control points.
// Masked by imask.
{
  int uz=0;
  int cpz=3;
  int index = 0;
  if (verbosity>1)
  {
    std::cout<<"Removing bias"<<std::endl;
  }
  for (int z=0;z<cz;z++,uz++)
  {
    if (verbosity>1) { std::cout<<'.'; std::cout.flush(); }
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
        short v = 0;
        if (imask[index]!=0)
        {
          float32 val = 0.0f;
          for (int q=0;q<4;q++)
            for (int r=0;r<4;r++)
            {
              float32 f = bz[q][uz] * by[r][uy];
              for (int s=0;s<4;s++)
              {
                val += CP [(cpz-q)*nx*ny + (cpy-r)*nx + (cpx-s)]
                    * f * bx[s][ux];
              }
            }
          v = sclamp(img[index]/val);
        }
        img[index] = v;
        index++;
      }
    }
  }
  if (verbosity>1) std::cout<<std::endl;
}

void RegularizedSplineF::removeBiasMasked(float *img, uint8 *imask)
// Divides the img data by the spline specified by the control points.
// Masked by imask.
{
  int uz=0;
  int cpz=3;
  int index = 0;
  if (verbosity>1)
  {
    std::cout<<"Removing bias"<<std::endl;
  }
  for (int z=0;z<cz;z++,uz++)
  {
    if (verbosity>1) { std::cout<<'.'; std::cout.flush(); }
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
        float v = 0;
        if (imask[index]!=0)
        {
          float32 val = 0.0f;
          for (int q=0;q<4;q++)
            for (int r=0;r<4;r++)
            {
              float32 f = bz[q][uz] * by[r][uy];
              for (int s=0;s<4;s++)
              {
                val += CP [(cpz-q)*nx*ny + (cpy-r)*nx + (cpx-s)]
                    * f * bx[s][ux];
              }
            }
          v = (img[index]/val);
        }
        img[index] = v;
        index++;
      }
    }
  }
  if (verbosity>1) std::cout<<std::endl;
}
