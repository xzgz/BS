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

#include <vol3dsimple.h>
#include <fibertrackset.h>
#include <slice2d.h>
#include <connectivityproperties.h>
#include <labeldescriptionset.h>
#include <trackfilter.h>
#include <sstream>

void computeConnectivity(ConnectivityMap &connectivityMap, FiberTrackSet &tracks, const Vol3D<uint16> &vLabel, std::vector<int> &roiList, bool mergeLabels)
{
  LabelDescriptionSet labelDescriptionSet;
  ConnectivityProperties connectivityProperties;
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
  connectivityProperties.vMergedLabels.copy(vLabelMerged);
  uint16 max = 1+*std::max_element(vLabelMerged.begin(),vLabelMerged.begin()+vLabelMerged.size());
  connectivityProperties.computeLabelCentroids(vLabelMerged);

  if (roiList.empty())
  {
    for (int i=1;i<=max;i++)
    {
      if (connectivityProperties.count[i]>0)
      {
        roiList.push_back(i);
      }
    }
  }
  const size_t nLabels = roiList.size();
  std::cout<<"computing connectivity for "<<nLabels<<" ROIs."<<std::endl;
  connectivityMap.labelIDs.resize(nLabels);
  connectivityMap.nodeNames.resize(nLabels);
  Slice2D<float> &vCount(connectivityMap.adjacencyMatrix);
  vCount.setsize((int)nLabels,(int)nLabels);
  for (size_t i=0;i<nLabels;i++)
  {
    int id = roiList[i];
    std::cout<<"computing connectivity for label "<<id<<std::endl;
    connectivityMap.labelIDs[i] = id;
    if ((id>=0)&&(id<(int)labelDescriptionSet.labelDetails.size()))
    {
      connectivityMap.nodeNames[i] = labelDescriptionSet.labelDetails[id].descriptor;
    }
    else
    {
      std::ostringstream ostr;
      ostr<<"L"<<roiList[i];
      connectivityMap.nodeNames[i] = ostr.str();
    }
    FiberTrackSet *subset = TrackFilter::filterTractsByLabelAndLength(vLabelMerged,&tracks,roiList[i],0);
    vCount((int)i,(int)i) = (float)subset->nCurves();
    for (size_t j=0;j<i;j++)
    {
      vCount((int)i,(int)j) = vCount((int)j,(int)i) = (float)TrackFilter::filterTractsByLabelAndLengthCountOnly(vLabelMerged,subset,roiList[j],0);
    }
    delete subset;
  }
  connectivityMap.computeMaxValue();
  connectivityProperties.sumWeights(connectivityMap,vLabelMerged);
}

bool saveConMap(std::string ofname, ConnectivityMap &connectivityMap)
{
  std::ofstream ofile(ofname.c_str());
  if (!ofile) return false;
  size_t nElements = connectivityMap.size();
  ofile<<'0';
  for (size_t i=0;i<nElements;i++)
  {
    ofile<<'\t'<<connectivityMap.getLabelID(i);
  }
  ofile<<'\n';
  for (size_t i=0;i<nElements;i++)
  {
    ofile<<connectivityMap.getLabelID(i);
    for (size_t j=0;j<nElements;j++)
    {
      float value = connectivityMap.adjacencyMatrix((int)i,(int)j);
      ofile<<'\t'<<value;
    }
    ofile<<'\n';
  }
  return true;
}


bool saveConMapNII(std::string ofname, ConnectivityMap &connectivityMap)
{
  Vol3D<float> vOut;
  const Slice2D<float> &am=connectivityMap.adjacencyMatrix;
  vOut.setsize(am.cx,am.cy,1);
  const int ds=am.cx*am.cy;
  for (int y=0;y<am.cy;y++)
    for (int x=0;x<am.cx;x++)
      vOut(x,y,0)=am(x,y);
  return vOut.write(ofname);
}

template<class LineItem>
bool scanROIfile(std::vector<LineItem> &itemList, std::string ifname)
{
  std::ifstream ifile(ifname.c_str());
  if (!ifile) return false;
  char buffer[4096];
  while (!ifile.eof())
  {
    buffer[0]=0;
    ifile.getline(buffer,4096);
    if (ifile.gcount()==0) continue;
    if (buffer[0]=='#') continue;
    std::istringstream istr(buffer);
    if (buffer[0]==0) continue;
    LineItem item;
    istr>>item;
    itemList.push_back(item);
  }
  return true;
}

int main(int argc, char *argv[])
{
  ArgParser ap("conmat");
  ap.description =
      "conmat computes a connectivity matrix based on a labeled ROI volume and a \n"
      "tractography file. For more information, please see http://brainsuite.org";
  std::string lfname,nfname,rfname;
  bool mergeCortical=false;
  std::vector<int> roiList;
  ap.bind("l",lfname,"<filename>","ROI label file (must be 16-bit)",true);
  ap.bind("n",nfname,"<filename>","save matrix as nifti",false);
  ap.bind("f",rfname,"<filename>","ROI list file (plain text)",false);
  ap.bindVector("r",roiList,"roi1 [ roi2 ... roiN ]",false);
  ap.bindFlag("-merge",mergeCortical,"merge cortical ROIs");
  if (!ap.parseAndValidate(argc,argv)) return ap.usage();
  if (!rfname.empty() && !scanROIfile(roiList,rfname)) return CommonErrors::cantRead(rfname);
  FiberTrackSet fiberTrackSet;
  Vol3D<uint16> vLabel;
  ConnectivityMap connectivityMap;
  if (!fiberTrackSet.read(ap.ifname)) return CommonErrors::cantRead(ap.ifname);
  if (!vLabel.read(lfname)) return CommonErrors::cantRead(lfname);
  computeConnectivity(connectivityMap,fiberTrackSet, vLabel, roiList, mergeCortical);
  saveConMap(ap.ofname,connectivityMap);
  if (!nfname.empty())
    if (!saveConMapNII(nfname,connectivityMap))
      return CommonErrors::cantWrite(nfname);
  return 0;
}

