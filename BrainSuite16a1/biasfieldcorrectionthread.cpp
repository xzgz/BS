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

#include <biasfieldcorrectionthread.h>
#include <bfcthreaddialog.h>
#include <bfcguitool.h>
#include <BFC/clampcopy.h>
#include <iostream>

BFCThread::BFCThread(BFCThreadDialog *parent, BFCGUITool *tool, Vol3D<float32> &vWorking, Vol3D<float32> &vSpline, Vol3D<uint8> &vMask) :
  QThread(parent), parent(parent), tool(tool), vWorking(vWorking), vSpline(vSpline), vMask(vMask)
{
}

void BFCThread::run()
{
  connect (this, SIGNAL(finished()),
           this, SLOT(whenFinished()));
  connect (this, SIGNAL(notifyParentFinished(BFCThread *)),
           parent, SLOT(notifyFinished(BFCThread *)));
  connect (this, SIGNAL(notifyParentStartingIteration(int )),
           parent, SLOT(notifyThreadStartingIteration(int )));
  if (!tool) return;
  for (tool->resetIterations(); !tool->atEnd(); )
  {
    emit notifyParentStartingIteration((int)tool->iterationNumber());
    tool->performNextIteration(vWorking,vSpline,vMask);
    if (tool->bfcTool.bfc.halt)
    {
      break;
    }
  }
}
