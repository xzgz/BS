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

#include <math.h>
#include <BFC/means3.h>

namespace BFCStats {
// TODO: these are hard coded assuming a 256 bin histogram; need to update
float centroid(float *c)
{
  float sum=0,count=0;
  for (int i=0;i<256;i++) { sum += i*c[i]; count += c[i]; }
  return sum/count;
}

float variance(float c[], float mean)
{
  float sum=0,count=0;
  for (int i=0;i<256;i++) { sum += square(i-mean)*c[i]; count += c[i]; }
  return sum/count;
}

void findPct(float c[], float h[], float low, float high)
{
  float sum = 0;
  int a=0,b=256;
  for (int i=0;i<256;i++)
  {
    sum += h[i];
    if (sum>=low) { a=i+1; break; }
  }
  for (int i=a;i<256;i++)
  {
    sum += h[i];
    if (sum>=high) { b=i; break; }
  }
  for (int i=0  ;i<a  ;i++) c[i] = 0;
  for (int i=a  ;i<=b ;i++) c[i] = h[i];
  for (int i=b+1;i<256;i++) c[i] = 0;
}

Means3 histTest(Histogram &hIn)
{
  float h[256];
  float csf[256];
  float gm[256];
  float wm[256];
  int sum = 0;
  for (int i=0;i<256;i++) sum += hIn.count[i];
  for (int i=0;i<256;i++) h[i] = float(hIn.count[i])/sum;
  findPct(csf,h,0.01f,0.09f); // TODO: these numbers should be parameters
  findPct(gm ,h,0.25f,0.55f);
  findPct(wm ,h,0.65f,0.95f);
  float mc = centroid(csf);
  float mg = centroid(gm);
  float mw = centroid(wm);
  findPct(wm, h,0.65f,0.99f);
  for (int i=0;i<mw;i++) wm[i] = 0;
  float vr = variance(wm,mw);
  Means3 M;
  M.mean0 = mc;
  M.mean1 = mg;
  M.mean2 = mw;
  M.var = vr;
  M.std = (float)sqrt(vr);
  return M;
}

} // end of bfc stats namespace
