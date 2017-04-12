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

#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vol3dsimple.h>
#include <DS/timer.h>
#include <vol3d_t.h>
#include <fibertrackset.h>
#include <htracker.h>
#include <fibertrackerthread.h>

inline float fractionalAnisotropy(const EigenSystem3x3f &e)
{
  return sqrtf(0.5 *
               (square(e.l0-e.l1)+square(e.l1-e.l2)+square(e.l2-e.l0))
               /(e.l0*e.l0+e.l1*e.l1+e.l2*e.l2)
               );
}

int main(int argc, char *argv[])
{
  std::string mfname;
  float angleThreshold=10.0f;
  int nSteps=500;
  float seedsPerVoxel=1.0f;
  float stepsize=0.25f;
  float faThresh=0.0f;
  std::string ffname;
  bool sortTracks=false;
  int nThreads=1;
  ArgParser ap("htrack");
  ap.description="htrack performs deterministic tractography on a DTI or ODF maxima file.";
  ap.bind("m",mfname,"<filename>","mask filename (must match dimensions and resolution of input)");
  ap.bind("f",ffname,"<filename>","fractional anisotropy filename (must match dimensions and resolution of input)");
  ap.bind("a",angleThreshold,"<number>","angle threshold [degrees]");
  ap.bind("n",nSteps,"<number>","maximum number of steps");
  ap.bind("s",seedsPerVoxel,"<number>","seeds per voxel (in each dimension)");
  ap.bind("-stepsize",stepsize,"<number>","stepsize (mm)");
  ap.bind("-fathresh",faThresh,"<number>","fractional anisotropy threshold (use file if provided)");
  ap.bind("j",nThreads,"#threads","number of threads to use (0=use 1/core)");
  ap.bindFlag("-sort",sortTracks,"sort fiber tracks");
  if (!ap.parseAndValidate(argc,argv)) return ap.usage();
  float vStep=1.0f/seedsPerVoxel;

  Vol3D<EigenSystem3x3f> vEig;
  Vol3D<uint8> vMask;
  Vol3D<float> vFA;
  if (!vEig.read(ap.ifname)) return CommonErrors::cantRead(ap.ifname);
	std::cout<<"read eig file "<<ap.ifname<<std::endl;
  if (mfname.empty()==false)
  {
    if (!vMask.read(mfname)) return CommonErrors::cantRead(mfname);
    if (!vMask.isCompatible(vEig)) return CommonErrors::incompatibleVolumes(mfname,ap.ifname);
	std::cout<<"read mask file "<<mfname<<std::endl;
  }
  if (ffname.empty()==false)
  {
    if (!vFA.read(ffname)) return CommonErrors::cantRead(ffname);
    if (!vFA.isCompatible(vEig)) return CommonErrors::incompatibleVolumes(ffname,ap.ifname);
  }
  if (faThresh>0)
  {
		std::cout<<"masking with fa thresh: "<<faThresh<<std::endl;
    const size_t ds=vEig.size();
    if (vFA.size()==0)
    {
			vFA.makeCompatible(vEig);
      for (size_t i=0;i<ds;i++)
      {
        vFA[i]=fractionalAnisotropy(vEig[i]);
      }
    }
    if (vMask.size())
    {
      for (size_t i=0;i<ds;i++)
        vMask[i]=vMask[i] ? (vFA[i]>faThresh) : 0;
    }
    else
    {
			vMask.makeCompatible(vFA);
      for (size_t i=0;i<ds;i++)
        vMask[i]=(vFA[i]>faThresh) ? 255 : 0;
    }
  }
	std::cout<<"starting tracking"<<std::endl;
  FiberTrackSet fiberTrackSet;
  FiberTrackerThreadManager fttm(fiberTrackSet,vEig,vMask,vStep,stepsize,nSteps,angleThreshold);
  fttm.launch(nThreads);
  if (sortTracks) fiberTrackSet.sortFiberTracks();
  fiberTrackSet.write(ap.ofname);
  return 0;
}
