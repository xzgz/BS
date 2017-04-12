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

#include <findodfmaximamanager.h>
#include <findodfmaximathread.h>
#include <harditool.h>

FindODFMaximaManager::FindODFMaximaManager(Vol3D<EigenSystem3x3f> &vEig, const Vol3D<uint8> &vMask, const HARDITool &hardiTool) :
  vEig(vEig), hardiTool(hardiTool), vMask(vMask), halt(false)
{
}

FindODFMaximaManager::~FindODFMaximaManager()
{
}

void FindODFMaximaManager::launch(int nThreads)
{
  halt=false;
  if (nThreads<=0)
  {
    nThreads=std::thread::hardware_concurrency();
  }
  if (nThreads<1) nThreads=1;
  vEig.makeCompatible(hardiTool.vODF);
  currentSlice=0;
  if (nThreads==1)
  {
    std::cout<<"not using threading"<<std::endl;
    FindODFMaximaThread runner(this,vEig,vMask,hardiTool);
    runner.run();
  }
  else
  {
    std::vector<std::thread> odfThreads;
    for (int n=0;n<nThreads;n++)
    {
      odfThreads.push_back(std::thread(&FindODFMaximaThread::run, FindODFMaximaThread(this,vEig,vMask,hardiTool)));
    }
    for (size_t n=0;n<odfThreads.size();n++)
      odfThreads[n].join();
  }
}

int FindODFMaximaManager::nextSlice()
{
  if (halt) return -1;
  sliceMutex.lock();
  int z=(currentSlice<vEig.cz) ? currentSlice++ : -1;
  sliceMutex.unlock();
  return z;
}
