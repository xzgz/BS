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

#include <list>
#include <fibertrackerthread.h>
#include <fibertrackset.h>
#include <htracker.h>
#include <vectorinterpolator.h>
#include <rungekutta4.h>
#include <alignfull.h>

DSPoint trackColor(const std::vector<DSPoint> &points) // TODO: move to library
{
  if (points.size()<1) return DSPoint(1,0,0);
  DSPoint color;
  for (size_t i=1;i<points.size();i++)
  {
    DSPoint dp=points[i]-points[i-1];
    color += abs(dp);
  }
  return color.unit();
}

int FiberTrackerThread::threadCount=0;

FiberTrackerThread::FiberTrackerThread(FiberTrackerThreadManager *parent, HTrackerBase *tracker, const Vol3D<EigenSystem3x3f> &vEig, const float dotProductThresh) :
  parent(parent), tracker(tracker), vEig(vEig), fibertrackDotProductThresh(dotProductThresh), threadNumber(threadCount++)
{
}

void FiberTrackerThread::run()
{
  DSPoint seedPoint;
  int np=parent->nextPoint(seedPoint);
  while (np>=0)
  {
    std::list<DSPoint> track;
    int seedIndex=-1;
    tracker->track(track,seedIndex,vEig,seedPoint,fibertrackDotProductThresh);
    parent->reportFinished(np,track,seedIndex);
    np=parent->nextPoint(seedPoint);
  }
}

FiberTrackerThreadManager::~FiberTrackerThreadManager()
{
}

FiberTrackerThreadManager::FiberTrackerThreadManager(FiberTrackSet &fiberTrackSet, const Vol3D<EigenSystem3x3f> &vEigIn, const Vol3D<uint8> &vMask, const float vStep, float stepsize, int nSteps, const float angleThresh) :
  fiberTrackSet(fiberTrackSet), vEigIn(vEigIn), vMask(vMask), pointIndex(0),
  zMax(0), fibertrackDotProductThresh(0.99), useMask(false),
  vStep(vStep), stepsize(stepsize), nSteps(nSteps), angleThresh(angleThresh), halt(false)
{
}

int FiberTrackerThreadManager::currentSlice() const { return currentPosition.z; }

int FiberTrackerThreadManager::nSlices() const { return vEig.cz; }

bool FiberTrackerThreadManager::stop()
{
  halt=true;
  return true;
}

void FiberTrackerThreadManager::initEigen()
{
  EigenSystem3x3f blank;
  vEig.makeCompatible(vEigIn);
  bool mask = vMask.isCompatible(vEig);
  if (mask)
  {
    const int ds = vMask.size();
    for (int i=0;i<ds;i++) vEig[i] = (vMask[i] && vEigIn[i].l0!=0) ? vEigIn[i] : blank;
  }
  else
  {
    const int ds = vMask.size();
    for (int i=0;i<ds;i++) vEig[i] = (vEigIn[i].l0!=0) ? vEigIn[i] : blank;
  }
}

bool FiberTrackerThreadManager::launch(int nThreads)
{
  if (nThreads<=0)
  {
    nThreads=std::thread::hardware_concurrency();
  }
  if (nThreads<1) nThreads=1;
  initEigen();
  fibertrackDotProductThresh=std::cos(angleThresh*M_PI/180);
  stepSizes=DSPoint(vEig.rx*vStep,vEig.ry*vStep,vEig.rz*vStep);
  HTrackerBase *tracker=new HTracker<VectorInterpolator<AlignFull>,RungeKutta4>();
  if (!tracker) return false;
  tracker->voxels = true;
  tracker->stepsize = stepsize;
  tracker->nSteps = nSteps;

  zMax=(float)(vEig.cz-1);
  useMask=vMask.isCompatible(vEig);
  currentPosition=DSPoint(0,0,0);
  if (nThreads==1)
  {
    std::cout<<"not using threading"<<std::endl;
    FiberTrackerThread runner(this,tracker,vEig,fibertrackDotProductThresh);
    runner.run();
  }
  else
  {
  std::vector<std::thread> trackingThreads;
  for (int n=0;n<nThreads;n++)
    trackingThreads.push_back(std::thread(&FiberTrackerThread::run, FiberTrackerThread(this,tracker,vEig,fibertrackDotProductThresh)));
  for (size_t n=0;n<trackingThreads.size();n++)
    trackingThreads[n].join();
  }
  return true;
}

void FiberTrackerThreadManager::reportFinished(int /*index*/, std::list<DSPoint> &track, int seedIndex)
{
  if (track.size()>1)
  {
    std::vector<DSPoint> points;
    points.reserve(track.size());
    for (auto i : track) points.push_back(i);
    DSPoint color=::trackColor(points);
    fiberMutex.lock();
    fiberTrackSet.newCurve();
    fiberTrackSet.current().seedIndex=seedIndex;
    fiberTrackSet.current().attributes.color = color;
    fiberTrackSet.current().points=std::move(points);
    fiberMutex.unlock();
  }
}

int FiberTrackerThreadManager::nextPoint(DSPoint &point)
{
  if (halt) return -1;
  fiberMutex.lock();
  int retval=-1;
  for (;;)
  {
    currentPosition.x+=stepSizes.x;
    if (currentPosition.x>=vEig.cx)
    {
      currentPosition.y+=stepSizes.y;
      if (currentPosition.y>=vEig.cy)
      {
        currentPosition.z+=stepSizes.z;
				if (currentPosition.z>=vEig.cz) break;
        currentPosition.y=0;
      }
      currentPosition.x=0;
    }
    IPoint3D ip((int)ceil(currentPosition.x),(int)ceil(currentPosition.y),(int)ceil(currentPosition.z));
		if (ip.x >= vEig.cx) continue;
		if (ip.y >= vEig.cy) continue;
		if (ip.z >= vEig.cz) continue;
    if (useMask && !vMask(ip)) continue;
    retval=pointIndex++;
    point=currentPosition;
    break;
  }
  fiberMutex.unlock();
  return retval;
}
