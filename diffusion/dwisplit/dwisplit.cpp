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

#define NOMINMAX
#include <vector>
#include <iostream>
#include <fstream>
#include <niftiparser.h>
#include <zstream.h>
#include <endianswap.h>
#include <argparser.h>
#include <strutil.h>
#include <algorithm>
#include <vol3dlib.h>

template <class T>
bool inlist(const T &value, const std::vector<T> &v)
{
	auto i=std::find(v.begin(),v.end(),value);
	return (i != v.end());
}


void swapNIFTIHeader(nifti_1_header &hdr)
{
	SILT::endian_swap(hdr.sizeof_hdr);
	for (int i=0;i<8;i++)
		SILT::endian_swap(hdr.dim[i]);
	SILT::endian_swap(hdr.datatype);
	SILT::endian_swap(hdr.bitpix);
	SILT::endian_swap(hdr.slice_start);
	for (int i=0;i<8;i++)
		SILT::endian_swap(hdr.pixdim[i]);
	SILT::endian_swap(hdr.vox_offset);
	SILT::endian_swap(hdr.scl_slope);
	SILT::endian_swap(hdr.scl_inter);
	SILT::endian_swap(hdr.slice_end);
	SILT::endian_swap(hdr.cal_max);
	SILT::endian_swap(hdr.cal_min);
	SILT::endian_swap(hdr.slice_duration);
	SILT::endian_swap(hdr.toffset);
	SILT::endian_swap(hdr.glmax);
	SILT::endian_swap(hdr.glmin);
	SILT::endian_swap(hdr.qform_code);
	SILT::endian_swap(hdr.sform_code);
	SILT::endian_swap(hdr.quatern_b);
	SILT::endian_swap(hdr.quatern_c);
	SILT::endian_swap(hdr.quatern_d);
	SILT::endian_swap(hdr.qoffset_x);
	SILT::endian_swap(hdr.qoffset_y);
	SILT::endian_swap(hdr.qoffset_z);
	for (int i=0;i<4;i++)
		SILT::endian_swap(hdr.srow_x[i]);
	for (int i=0;i<4;i++)
		SILT::endian_swap(hdr.srow_y[i]);
	for (int i=0;i<4;i++)
		SILT::endian_swap(hdr.srow_z[i]);
}

int main(int argc, char *argv[])
{
	std::string bvecfname;
	std::string bvalfname;
	std::vector<double> keepList;
	ArgParser ap("dwisplit");
	ap.verbosity=1;
	ap.bind("-bvec",bvecfname,"bvec_filename","bvec filename (plain text)",true);
	ap.bind("-bval",bvalfname,"bval_filename","bval filename (plain text)",true);
	ap.bindVector("b",keepList,"bval0 [ bval1 ... bvalN]");
	if (!ap.parseAndValidate(argc,argv)) return ap.usage();
	if (ap.verbosity>1)
	{
		std::cout<<"keeping b-values:";
		for (auto k : keepList) std::cout<<"\t"<<k;
		std::cout<<std::endl;
	}
	SILT::izstream ifile(ap.ifname);
	if (!ifile) return CommonErrors::cantRead(ap.ifname);
	nifti_1_header hdr;
	ifile.read((char *)&hdr,sizeof(hdr));
	int nd = hdr.dim[0];
	if ((nd<0) || (nd>7))
	{
		::swapNIFTIHeader(hdr);
	}
	const int cx=hdr.dim[1];
	const int cy=hdr.dim[2];
	const int cz=hdr.dim[3];
	const int ct=hdr.dim[4];
	if (ap.verbosity>0) std::cout<<ap.ifname<<" has "<<cx<<'x'<<cy<<'x'<<cz<<'x'<<ct<<std::endl;

	std::ifstream bvalfile(bvalfname);
	if (!bvalfile) return CommonErrors::cantRead(bvalfname);
	std::ifstream bvecfile(bvecfname);
	if (!bvecfile) return CommonErrors::cantRead(bvecfname);

	std::vector<bool> keepSet(ct,false);
	std::string iBasename=StrUtil::gzStrip(ap.ifname);
	iBasename=StrUtil::extStrip(iBasename,"nii");
	iBasename=StrUtil::extStrip(iBasename,"img");
	//	iBasename=StrUtil::extStrip(iBasename,"dwi");
	if (ap.verbosity>0) std::cout<<"using ibasename "<<iBasename<<std::endl;
	std::string oBasename=StrUtil::gzStrip(ap.ofname);
	oBasename=StrUtil::extStrip(oBasename,"nii");
	oBasename=StrUtil::extStrip(oBasename,"img");
	//	oBasename=StrUtil::extStrip(oBasename,"dwi");
	if (ap.verbosity>0) std::cout<<"using obasename "<<oBasename<<std::endl;
	std::vector<std::string> bvals(ct);
	int nc=0;
	{
		std::string obvalfname(oBasename+".bval");
		std::ofstream obval(obvalfname);
		if (!obval) return CommonErrors::cantWrite(obvalfname);
		for (int i=0;i<ct;i++)
		{
			std::string bval;
			bvalfile>>std::ws>>bval;
			bvals[i]=bval;
			double value=0;
			std::istringstream istr(bval);
			istr>>value;
			keepSet[i]=inlist(value,keepList);
			if (keepSet[i])
			{
				if (nc++) obval<<' ';
				obval<<bval;
			}
			if (ap.verbosity>1)
			{
				std::cout<<i<<"\t"<<bval<<"\t"<<(keepSet[i] ? "keeping" : "removing")<<std::endl;
			}
		}
		if (nc) obval<<'\n';
	}
	{
		std::string obvecfname(oBasename+".bvec");
		std::ofstream obvec(obvecfname);
		if (!obvec) return CommonErrors::cantWrite(obvecfname);
		for (int row=0;row<3;row++)
		{
			int ncx=0;
			for (int i=0;i<ct;i++)
			{
				std::string bvecComponent;
				bvecfile>>std::ws>>bvecComponent;
				if (keepSet[i])
				{
					if (ncx++) obvec<<' ';
					obvec<<bvecComponent;
				}
			}
			if (ncx) obvec<<'\n';
		}
	}
	if (ap.verbosity>0) std::cout<<ap.ofname<<" has "<<cx<<'x'<<cy<<'x'<<cz<<'x'<<nc<<std::endl;
	std::vector<char> buffer;
	float vsize=cx*cy*cz*hdr.bitpix/8;
	int ds=vsize;
	if (ds!=vsize) return 1;
	if (ap.verbosity>1) std::cout<<"reading chunks of "<<ds<<" bytes."<<std::endl;
	buffer.resize(cx*cy*cz*hdr.bitpix/8);
	ifile.seekg(hdr.vox_offset,0);

	SILT::ozstream ofile(ap.ofname);
	if (!ofile) return CommonErrors::cantWrite(ap.ofname);
	hdr.dim[4]=nc;
	hdr.vox_offset=352;
	ofile.write((char *)&hdr,sizeof(hdr));
	char buf[4]={0,0,0,0};
	ofile.write(buf,4);
	ifile.seekg(hdr.vox_offset,0);

	for (int i=0;i<ct;i++)
	{
    ifile.read(&buffer[0],buffer.size());
		if (keepSet[i])
		{
      if (ap.verbosity>2) std::cout<<"reading volume "<<i+1<<'/'<<ct<<'\t'<<bvals[i]<<std::endl;
			ofile.write(&buffer[0],buffer.size());
		}
		else
		{
//			ifile.seekg(buffer.size(),SEEK_CUR);
      if (ap.verbosity>2) std::cout<<"skipping volume "<<i+1<<'/'<<ct<<'\t'<<bvals[i]<<std::endl;
		}
	}
	return 0;
}
