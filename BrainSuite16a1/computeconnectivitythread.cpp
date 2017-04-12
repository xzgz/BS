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

#include "computeconnectivitythread.h"
#include <connectivitymap.h>
#include <fibertrackset.h>
#include <computeconnectivitythreaddialog.h>
#include <trackfilter.h>

ComputeConnectivityThread::ComputeConnectivityThread(ComputeConnectivityThreadDialog *parent,
                                                     ConnectivityMap &connectivityMap, ConnectivityProperties &connectivityProperties,
                                                     FiberTrackSet &tracts,
                                                     Vol3D<uint16> &vLabel, LabelDescriptionSet &labelDescriptionSet,
                                                     bool mergeLabels, bool connectivityExcludeWMROI) :
  parent(parent), connectivityMap(connectivityMap),
  tracts(tracts), vLabel(vLabel), labelDescriptionSet(labelDescriptionSet),
  connectivityProperties(connectivityProperties),
  mergeLabels(mergeLabels), connectivityExcludeWMROI(connectivityExcludeWMROI)
{  
}

void ComputeConnectivityThread::computeConnectivity()
// TODO: move to connectivity map class
{
  Vol3D<uint16> vLabelMerged;
  vLabelMerged.copy(vLabel);
  if (mergeLabels)
  {
    const size_t ds=vLabelMerged.size();
    std::vector<uint16> labelTable(65536);
    for (int i=0;i<65536;i++) labelTable[i]=i;
    for (int i=1001;i<2000;i++) labelTable[i]=i-1000;
    for (int i=2001;i<3000;i++) labelTable[i]=i-2000;
    for (size_t i=0;i<ds;i++) vLabelMerged[i]=labelTable[vLabelMerged[i]];
  }
  if (connectivityExcludeWMROI)
  {
    const size_t ds=vLabelMerged.size();
    for (size_t i=0;i<ds;i++)
      if (vLabelMerged[i]==2000) vLabelMerged[i]=0;
  }
  connectivityProperties.vMergedLabels.copy(vLabelMerged);
  uint16 max = 1+*std::max_element(vLabelMerged.begin(),vLabelMerged.begin()+vLabelMerged.size());
	connectivityProperties.computeLabelCentroids(vLabelMerged);
  std::vector<int> labels;
  for (size_t i=1;i<=max;i++)
  {
    if (i>connectivityProperties.count.size())
    {
      std::cout<<"oops!"<<i<<'\t'<<connectivityProperties.count.size()<<std::endl;
      break;
    }
    if (connectivityProperties.count[i]>0)
    {
      labels.push_back(i);
    }
  }
  const size_t nLabels = labels.size();
  connectivityMap.labelIDs.resize(nLabels);
  connectivityMap.nodeNames.resize(nLabels);
  Slice2D<float> &vCount(connectivityMap.adjacencyMatrix);
  vCount.setsize((int)nLabels,(int)nLabels);
  for (size_t i=0;i<nLabels;i++)
  {
    emit notifyParentStartingIteration((int)i);
    int id = labels[i];
    connectivityMap.labelIDs[i] = id;
    if ((id>=0)&&(id<(int)labelDescriptionSet.labelDetails.size()))
    {
      connectivityMap.nodeNames[i] = labelDescriptionSet.labelDetails[id].descriptor;
    }
    else
    {
      std::ostringstream ostr;
      ostr<<"L"<<labels[i];
      connectivityMap.nodeNames[i] = ostr.str();
    }
    FiberTrackSet *subset = TrackFilter::filterTractsByLabelAndLength(vLabelMerged,&tracts,labels[i],0);
    vCount((int)i,(int)i) = (float)subset->nCurves();
    for (size_t j=0;j<i;j++)
    {
      vCount((int)i,(int)j) = vCount((int)j,(int)i) = (float)TrackFilter::filterTractsByLabelAndLengthCountOnly(vLabelMerged,subset,labels[j],0);
    }
    delete subset;
  }
  connectivityMap.computeMaxValue();
  connectivityProperties.sumWeights(connectivityMap,vLabelMerged);
}

void ComputeConnectivityThread::run()
{
  connect (this, SIGNAL(finished()),
           this, SLOT(whenFinished()));
  connect (this, SIGNAL(notifyParentFinished(ComputeConnectivityThread *)),
           parent, SLOT(notifyFinished(ComputeConnectivityThread *)));
  connect (this, SIGNAL(notifyParentStartingIteration(int )),
           parent, SLOT(notifyThreadStartingIteration(int )));
  computeConnectivity();
}

void ComputeConnectivityThread::whenFinished()
{
  if (parent)
  {
    emit notifyParentFinished(this);
  }
}
