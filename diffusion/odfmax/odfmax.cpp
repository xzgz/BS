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
#include <harditool.h>
#include <findodfmaximathread.h>
#include <findodfmaximamanager.h>
#include <DS/timer.h>

int main(int argc, char *argv[])
{
	std::string mfname;
	HARDITool hardiTool;
	ArgParser ap("odfmax");
	int samplingRes=20;
  int nThreads=1;
	ap.description="odfmax computes the three maximal directions of the ODF in each voxel.";
	ap.bind("m",mfname,"mask_file","input mask file",false);
	ap.bind("n",samplingRes,"#Samples","number of samples in Az/El for SHCs",false);
	ap.bind("j",nThreads,"#threads","number of threads to use (0=use 1/core)");
	ap.bindFlag("-flipx",hardiTool.odfFactory.flipX,"flip X dimension of ODFs");
	ap.bindFlag("-flipy",hardiTool.odfFactory.flipY,"flip Y dimension of ODFs");
	ap.bindFlag("-flipz",hardiTool.odfFactory.flipZ,"flip Z dimension of ODFs");
	if (!ap.parseAndValidate(argc,argv)) return ap.usage();
	Vol3D<EigenSystem3x3f> vEig;
	Timer t1;t1.start();
	if (!hardiTool.loadSHClist(ap.ifname)) return CommonErrors::cantRead(ap.ifname);
	t1.stop();
	if (ap.verbosity>1) std::cout<<"loading odfs took "<<t1.elapsed()<<std::endl;
	if (!hardiTool.filecount) { std::cerr<<"No ODF files have been loaded."<<std::endl; return 1; }
	Vol3D<uint8> vMask;
	if (mfname.empty()==false)
	{
		if (!vMask.read(mfname)) return CommonErrors::cantRead(mfname);
		if (!vMask.isCompatible(hardiTool.vODF)) return CommonErrors::incompatibleVolumes(mfname,"odf volumes");
	}
	Timer t2;t2.start();
	hardiTool.odfFactory.makeBasis(samplingRes);
	t2.stop();
	if (ap.verbosity>1) std::cout<<"making basis took "<<t2.elapsed()<<std::endl;
	FindODFMaximaManager odfMan(vEig,vMask,hardiTool);
	Timer t3;t3.start();
	odfMan.launch(nThreads);
	t3.stop();
	if (ap.verbosity>1) std::cout<<"finding maxima took "<<t3.elapsed()<<std::endl;
	if (!vEig.write(ap.ofname)) return CommonErrors::cantWrite(ap.ofname);
	return 0;
}
