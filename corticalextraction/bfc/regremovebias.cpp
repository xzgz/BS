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

void RegularizedSplineF::removeBias(uint8 *img)
// Divides the img data by the spline specified by the control points.
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
}



template <class T> T tclamp(const float &f) { return (f>0) ? f : 0; }
inline unsigned char tclamp(const uint8 &f) { return (f<255) ? (unsigned char)((f>0) ? f : 0) : 255; }
inline unsigned char tclamp(const uint16 &f) { return (f<255) ? (unsigned char)(f) : 255; }

template <class T>
void RegularizedSplineF::divideInto(Vol3D<T> &vIn)
// Divides the img data by the spline specified by the control points.
{
  T *img = vIn.start();

  int uz=0;
  int cpz=3;
  int index = 0;
  for (int z=0;z<cz;z++,uz++)
  {
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
        T v = 0;
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
          v = tclamp(img[index]/val);
        }
        img[index] = v;
        index++;
      }
    }
  }
}
