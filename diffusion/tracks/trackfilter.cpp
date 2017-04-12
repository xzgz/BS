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

#include <trackfilter.h>
#include <vol3d.h>

FiberTrackSet *TrackFilter::filterTracksByLength(const FiberTrackSet *tractSet, const float lengthThreshold)
{
  FiberTrackSet *subset = new FiberTrackSet;
  for (std::vector<FiberTrack>::const_iterator curve=tractSet->curves.begin(); curve!=tractSet->curves.end(); curve++)
  {
    if (curve->points.size()>0)
    {
      float length=fiberLength(curve->points);
      if (length>lengthThreshold)
      {
        subset->curves.push_back(*curve);
      }
    }
  }
  return subset;
}

inline bool TrackFilter::hasPointInSphere(const std::vector<DSPoint>  &points,  const DSPoint center, const float radius)
{
  const float r2=radius*radius;
  for (std::vector<DSPoint>::const_iterator point=points.begin(); point != points.end(); point++)
  {
    if ((center-*point).pwr()<=r2) return true;
  }
  return false;
}

FiberTrackSet *TrackFilter::sphericalFilter(const FiberTrackSet *trackSet, const DSPoint center, const float radius)
{
  FiberTrackSet *subset = new FiberTrackSet;
  for (std::vector<FiberTrack>::const_iterator curve=trackSet->curves.begin(); curve!=trackSet->curves.end(); curve++)
  {
    if (curve->points.size()>0)
    {
      if (hasPointInSphere(curve->points,center,radius))
      {
        subset->curves.push_back(*curve);
      }
    }
  }
  return subset;
}

FiberTrackSet *TrackFilter::reduceTrackSet(const FiberTrackSet *trackSet, const std::vector<bool> &keepList)
{
  if (!trackSet) return 0;
  const size_t nc=trackSet->curves.size();
  if (keepList.size()!=nc) return 0;
  FiberTrackSet *subset = new FiberTrackSet;
  if (!subset) return 0;
  for (size_t i=0;i<nc;i++)
  {
    if (keepList[i])
      subset->curves.push_back(trackSet->curves[i]);
  }
  return subset;
}

FiberTrackSet *TrackFilter::sphericalFilterExclude(const FiberTrackSet *tractSet, const DSPoint center, const float radius)
{
  FiberTrackSet *subset = new FiberTrackSet;
  for (std::vector<FiberTrack>::const_iterator curve=tractSet->curves.begin(); curve!=tractSet->curves.end(); curve++)
  {
    if (curve->points.size()>0)
    {
      if (!hasPointInSphere(curve->points,center,radius))
      {
        subset->curves.push_back(*curve);
      }
    }
  }
  return subset;
}

bool TrackFilter::sphericalFilterIndex(std::vector<bool> &keep, const FiberTrackSet *trackSet, const DSPoint center, const float radius)
{
  const size_t nc=trackSet->curves.size();
  if (keep.size() != nc)
  {
    keep=std::vector<bool>(nc,false);
  }
  size_t i=0;
  for (std::vector<FiberTrack>::const_iterator curve=trackSet->curves.begin(); curve!=trackSet->curves.end(); curve++, i++)
  {
    if (curve->points.size()>0)
    {
      if (hasPointInSphere(curve->points,center,radius))
      {
        keep[i]=true;
      }
    }
  }
  return true;
}

template <class T>
int TrackFilter::findTrackIntersections(std::vector<bool> &matchList, const FiberTrackSet *trackSet, const Vol3D<T> &vLabel, const std::vector<int> &labelList)
{
  int count=0;
  size_t nc=trackSet->curves.size();
  matchList.resize(nc);
  std::fill(matchList.begin(),matchList.end(),false);
  for (size_t i=0;i<nc;i++)
  {
    if (trackSet->curves[i].points.size()>0)
    {
      for (size_t j=0;j<labelList.size();j++)
      {
        size_t n = TrackFilter::labelCount(vLabel,trackSet->curves[i].points,labelList[j]);
        if (n>0) { matchList[i]=true; count++; break; }
      }
    }
  }
  return count;
}

FiberTrackSet *TrackFilter::filterExcludedLabels(const FiberTrackSet *fiberTrackSet, const Vol3DBase *labelVolume, std::vector<int> excludeLabels)
{
  if (!labelVolume) return 0;
  if (!fiberTrackSet) return 0;
  FiberTrackSet *newSubset=0;
  if (excludeLabels.size())
  {
    std::vector<bool> matchList;
    int nMatches=0;
    bool ableToProcess=true;
    switch (labelVolume->typeID())
    {
      case SILT::Uint8 : nMatches=findTrackIntersections(matchList,fiberTrackSet,*(Vol3D<uint8>*)labelVolume,excludeLabels); break;
      case SILT::Uint16: nMatches=findTrackIntersections(matchList,fiberTrackSet,*(Vol3D<uint16>*)labelVolume,excludeLabels); break;
      case SILT::Sint16: nMatches=findTrackIntersections(matchList,fiberTrackSet,*(Vol3D<sint16>*)labelVolume,excludeLabels); break;
      case SILT::Uint32: nMatches=findTrackIntersections(matchList,fiberTrackSet,*(Vol3D<uint32>*)labelVolume,excludeLabels); break;
      case SILT::Sint32: nMatches=findTrackIntersections(matchList,fiberTrackSet,*(Vol3D<sint32>*)labelVolume,excludeLabels); break;
      default: ableToProcess=false;
    }
    if (ableToProcess)
    {
      for (auto i=0U;i<matchList.size();i++) matchList[i]=(!matchList[i]);
      newSubset=TrackFilter::reduceTrackSet(fiberTrackSet,matchList);
    }
  }
  return newSubset;
}
