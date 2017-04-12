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

#include "brainsuitedatamanager.h"
#include <surfacealpha.h>
#include <fibertrackset.h>
#include <protocolcurveset.h>

BrainSuiteDataManager::SurfaceHandle::~SurfaceHandle()
{
  surface=0 ;
}

BrainSuiteDataManager::BrainSuiteDataManager() :
  volume(0), imageOverlay1Volume(0), imageOverlay2Volume(0), vLabel(0), fiberTrackSet(0), fiberTrackSubset(0), protocolCurveSet(0)
{
}

BrainSuiteDataManager::~BrainSuiteDataManager()
{
  delete volume;
  delete imageOverlay1Volume;
  delete imageOverlay2Volume;
  delete vLabel;
  delete fiberTrackSet;
  delete fiberTrackSubset;
  delete protocolCurveSet;
}

inline bool nonZero(BrainSuiteDataManager::SurfaceHandle &handle)
{
  return (handle.surface!=0);
}

inline bool isShown(BrainSuiteDataManager::SurfaceHandle &handle)
{
  return (handle.surface && handle.surface->show);
}

inline void hide(BrainSuiteDataManager::SurfaceHandle &handle)
{
  if (handle.surface) handle.surface->show = false;
}

bool BrainSuiteDataManager::addVolume(Vol3DBase *newVolume)
{
  if (newVolume==volume) return true;
  delete volume;
  volume = newVolume;
  return true;
}

bool BrainSuiteDataManager::addSurface(SurfaceAlpha *surface, bool giveOwnership)
{
  if (!surface) return false;
  surfaces.push_back(SurfaceHandle(surface,giveOwnership));
  return true;
}

bool BrainSuiteDataManager::removeSurface(SurfaceAlpha *surface)
{
  if (!surface) return false;
  bool owned=false;
  for (std::list<SurfaceHandle>::iterator surfaceHandle=surfaces.begin();surfaceHandle!=surfaces.end();)
  {
    if (surfaceHandle->surface == surface)
    {
      owned |= surfaceHandle->ownedByList;
      surfaceHandle=surfaces.erase(surfaceHandle);
    }
    else
    {
      surfaceHandle++;
    }
  }
  if (owned)
  {
    delete surface;
  }
  return true;
}

bool BrainSuiteDataManager::nextSurface()
{
  if (surfaces.size()>1)
  {
    std::list<SurfaceHandle>::iterator first=std::find_if(surfaces.begin(),surfaces.end(),isShown);
    if (first != surfaces.end()) first++;
    std::list<SurfaceHandle>::iterator next=std::find_if(first,surfaces.end(),nonZero);
    if (next == surfaces.end()) next=std::find_if(surfaces.begin(),first,nonZero); // this goes one too far?
    if (next == surfaces.end()) return false; // nothing to do
    if (next->surface)
    {
      std::for_each(surfaces.begin(),surfaces.end(),hide);
      next->surface->show = true;
      return true;
    }
  }
  return false;
}

template<class IteratorType, class PredicateType>
inline IteratorType rfind_if(IteratorType first, IteratorType last, PredicateType predicate)
{
  IteratorType iter(last);
  while (iter != first)
  {
    --iter;
    if (predicate(*iter)) { last=iter; break; }
  }
  return (last);
}

bool BrainSuiteDataManager::previousSurface()
{
  if (surfaces.size()<=1) return false;
  std::list<SurfaceHandle>::iterator first=std::find_if(surfaces.begin(),surfaces.end(),isShown);
  std::list<SurfaceHandle>::iterator next=rfind_if(surfaces.begin(),first,nonZero);
  if (next==first) next=rfind_if(first,surfaces.end(),nonZero);
  if (next == surfaces.end()||next==first)	return false;
  if (next->surface)
  {
    std::for_each(surfaces.begin(),surfaces.end(),hide);
    next->surface->show = true;
    return true;
  }
  return false;
}
