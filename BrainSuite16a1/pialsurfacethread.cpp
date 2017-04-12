// Copyright (C) 2016 The Regents of the University of California
//
// Created by David W. Shattuck, Ph.D.
//
// This file is part of BrainSuite16a1.
//
// BrainSuite16a1 is free software; you can redistribute it and/or
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

#include "pialsurfacethread.h"
#include <pialsurfacetool.h>
#include <pialsurfacethreaddialog.h>

PialSurfaceThread::PialSurfaceThread(PialSurfaceThreadDialog *parent, PialSurfaceTool *pialSurfaceTool) :
  QThread(parent), pialSurfaceTool(pialSurfaceTool), parent(parent)
{
}

void PialSurfaceThread::run()
{
  connect (this, SIGNAL(finished()),
           this, SLOT(whenFinished()));
  connect (this, SIGNAL(notifyParentFinished(PialSurfaceThread *)),
           parent, SLOT(notifyFinished(PialSurfaceThread *)));
  connect (this, SIGNAL(notifyParentStartingIteration(int )),
           parent, SLOT(notifyThreadStartingIteration(int )));

  if (!pialSurfaceTool) return;
  for (pialSurfaceTool->resetIterations(); !pialSurfaceTool->atEnd(); )
  {
    emit notifyParentStartingIteration((int)pialSurfaceTool->iterationNumber());
    pialSurfaceTool->performNextIteration();
  }
}

void PialSurfaceThread::whenFinished()
{
  if (parent)
  {
    emit notifyParentFinished(this);
  }
}
