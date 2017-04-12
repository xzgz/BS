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

#include "dualregistrationprocess.h"
#include <dualregistrationprocessdialog.h>
#include <iostream>

DualRegistrationProcess::DualRegistrationProcess(DualRegistrationProcessDialog *parent) :
  QProcess(parent), parent(parent)
{
}

bool DualRegistrationProcess::launch(QString program, QStringList arguments, QString directory)
{
  setWorkingDirectory(directory);
  setProcessChannelMode(QProcess::MergedChannels);
  connect (this, SIGNAL(readyReadStandardOutput()),
           this, SLOT(readOutput()));
  connect (this, SIGNAL(finished(int)),
           this, SLOT(whenFinished()));
  connect (this, SIGNAL(notifyParentFinished(DualRegistrationProcess *)),
           parent, SLOT(notifyFinished(DualRegistrationProcess *)));
  start(program,arguments);
  return true;
}

DualRegistrationProcess::~DualRegistrationProcess()
{
  terminate();
}

void DualRegistrationProcess::readOutput()
{
  QByteArray bytes = readAllStandardOutput();
  QStringList lines = QString(bytes).split("\n");
  foreach (QString line, lines)
  {
    if (parent)
      parent->parseOutput(this,line);
  }
}

void DualRegistrationProcess::whenFinished()
{
  if (parent)
  {
    emit notifyParentFinished(this);
  }
}

