// Copyright (C) 2016 The Regents of the University of California and
// the University of Southern California
//
// Created by David W. Shattuck, Ph.D.
//
// This file is part of Vol3D.
//
// Vol3D is free software; you can redistribute it and/or
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

#include "graph.h"

const int Graph::SENTINEL = 0;

int Graph::makemap(LabelT *map)
{
  int nLabels = 0;
  for (int i=0;i<nlists;i++) map[i] = SENTINEL;
  for (int i=0;i<nlists;i++)
  {
    if (map[i]==SENTINEL)
    {
      ++nLabels;
      visit(map, i, nLabels);
    }
  }
  return nLabels;
}

void Graph::visit(LabelT *map, int iNode, int labels)
{
  GraphNode *node;
  int stackpos = 0;
  stack[stackpos++] = iNode;
  while (stackpos>0)
  {
    iNode = stack[--stackpos];
    map[iNode] = labels;
    for (node = lists[iNode]; node != 0; node = node->next)
    {
      if (map[node->data]==SENTINEL)
      {
        stack[stackpos++] = node->data;
        if (stackpos>=stacksize)
        {
          // TODO: should add additional stack; reset stackpos to 1;
          std::cerr<<"Fatal error: out of stack in Graph::visit()"<<std::endl;
          std::cerr<<"Stack position is "<<stackpos<<std::endl;
          return;
        }
        map[node->data] = -1;
      }
    }
  }
}
