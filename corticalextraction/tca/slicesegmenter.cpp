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

#include <TCA/slicesegmenter.h>
#include <TCA/grapht.h>

SliceSegmenter::SliceSegmenter(int x, int y) : mode(D4), cx(x), cy(y), slicesize(cx*cy),
  nsymbols(0), runcount(0), info(0), map(0)
{
  runs      = memcheck(new RunLength[slicesize]);
  linestart = memcheck(new int[cy+1]);
}

SliceSegmenter::~SliceSegmenter()
{
  delete[] info;
  delete[] map;
  delete[] runs;
  delete[] linestart;
}

void SliceSegmenter::segment(uint8 *imageIn, LabelType *out, uint8 zero, uint8 one)
{
  runcount = 0;
  encode(imageIn,one);
  if (mode==D4)
    makeGraph();
  else
    makeGraph2();
  population();
  label(out,zero);
}

void SliceSegmenter::encode(const uint8 *imageIn, const uint8 fg)
{
  const uint8 code=fg;
  int index = 0;
  int state = 0;
  runcount = 0;
  RunLength newRun;
  int linecount = 0;
  for (int y=0; y<cy; y++)
  {
    linestart[linecount++] = runcount;
    state = (imageIn[index]==code);
    if (state)
    {
      newRun.start = 0;
    }
    index++;
    for (int x=1; x<cx; x++, index++)
    {
      if (imageIn[index]!=code)
      {
        if (state!=0) // terminate a code
        {
          newRun.stop = x-1;
          state = 0;
          runs[runcount++] = newRun;
        }
      }
      else
      {
        if (state==0) // start a New code
        {
          newRun.start = x;
          state = 1;
        }
      }
    }
    if (state!=0) // terminate the code.
    {
      newRun.stop = cx - 1;
      runs[runcount++] = newRun;
    }
  }
  linestart[linecount] = runcount;
}

int SliceSegmenter::label(LabelType *buffOut, const uint8 bg)
{
  for (int d=0;d<slicesize;d++) buffOut[d] = bg;
  int lab = 0;
  int linecount = 0;
  for (int y=0, index=0; y<cy; y++, index += cx)
  {
    int first = linestart[linecount];
    int last  = linestart[++linecount];
    for (int i=first; i<last; i++)
    {
      int start = index + runs[i].start;
      int stop = index + runs[i].stop;
      for (int j=start; j<=stop; j++) buffOut[j] = map[lab];
      lab++;
    }
  }
  return lab;
}

void SliceSegmenter::makeGraph() // D4 connectivity
{
  int linecount = 0;
  int linkcount = 0;
  GraphT<LabelType> graph(slicesize/10);
  graph.reset(runcount+1);
  // Since the first line of the slice (y==0) is not connected to anything
  // above it (there are no voxels for (y<0), there is nothing to link.
  linecount++;
  for (int y=1; y<cy; y++)
  {
    int first  = linestart[linecount];
    int runA   = linestart[linecount-1]; // previous
    int last   = linestart[linecount+1];
    linecount++;
    if (last<=first) continue;					// the line is empty
    int runB = first;
    for (;;)
    {
      if (runA>=first) break;
      if (runB>=last ) break;
      if (runs[runA].intersects(runs[runB]))
      {
        graph.link(runA,runB);
        graph.link(runB,runA);
        linkcount++;
      }
      int newA = runA;
      int newB = runB;
      if (runs[runA].stop<=runs[runB].stop) newA++;
      if (runs[runA].stop>=runs[runB].stop) newB++;
      runA = newA;
      runB = newB;
    }
  }
  delete[] map;
  map = memcheck(new LabelType[runcount+1]);
  nsymbols = graph.makemap(map);//,first);
}

void SliceSegmenter::makeGraph2() // D8 connectivity
{
  int linecount = 0;
  int linkcount = 0;
  GraphT<LabelType> graph(slicesize/10);
  graph.reset(runcount+1);
  // Since the first line of the slice (y==0) is not connected to anything
  // above it (there are no voxels for (y<0), there is nothing to link.
  linecount++;
  for (int y=1; y<cy; y++)
  {
    int first  = linestart[linecount];
    int runA   = linestart[linecount-1]; // previous
    int last   = linestart[linecount+1];
    linecount++;
    if (last<=first) continue;					// the line is empty
    int runB = first;
    for (;;)
    {
      if (runA>=first) break;
      if (runB>=last ) break;
      if (runs[runA].neighbors(runs[runB]))
      {
        graph.link(runA,runB);
        graph.link(runB,runA);
        linkcount++;
      }
      int newA = runA;
      int newB = runB;
      if (runs[runA].stop<=runs[runB].stop) newA++;
      if (runs[runA].stop>=runs[runB].stop) newB++;
      runA = newA;
      runB = newB;
    }
  }
  delete[] map;
  map = memcheck(new LabelType[runcount+1]);
  nsymbols = graph.makemap(map);//,1);
}

void SliceSegmenter::population()
{
  delete[] info;
  info = memcheck(new SliceRegionInfo[nsymbols+1]);
  int index = 0;
  int label = 0;
  int linecount = 0;
  for (int y=0; y<cy; y++)
  {
    int first = linestart[linecount];
    int last  = linestart[++linecount];
    for (int i=first; i<last; i++)
    {
      int length = (runs[i].stop - runs[i].start) + 1;
      if (length>0)
      {
        info[map[label]].count += length;
        info[map[label]].idx = index + runs[i].start;
      }
      label++;
    }
    index += cx;
  }
}
