// Copyright (c) 2016 The Regents of the University of California
// Created by David W. Shattuck
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include <iostream>
#include <string>
#include <sstream>
#include "cerebrohelperparser.h"

// this program requires the AIR library developed by Roger P. Woods, MD
// source code is available from http://www.bmap.ucla.edu/portfolio/software/AIR/

extern "C" {
#include "AIR.h"
}

CerebroHelperParser::CerebroHelperParser() : useCentroids(false),
  costFunction(1), linearConvergence(0.1f), warpLevel(5), warpConvergence(100.0f),
  step(1),
  verbosity(1)
{
}

std::ostream &operator<<(std::ostream &os, const CerebroHelperParser &p)
{
  os<<"ofname "<<p.ofname<<'\n';
  os<<"ifname "<<p.ifname<<'\n';
  os<<"atlasfile "<<p.atlasfile<<'\n';
  os<<"atlasToBrainAirFile "<<p.atlasToBrainAirFile<<'\n';
  os<<"atlasToBrainWarpFile "<<p.atlasToBrainWarpFile<<'\n';
  os<<"initfile "<<p.initfile<<'\n';
  os<<"labelfile "<<p.labelfile<<'\n';
  os<<"atlaslabelfile "<<p.atlaslabelfile<<'\n';
  os<<"useCentroids "<<p.useCentroids<<'\n';
  os<<"centroidInitializerFile "<<p.centroidInitializerFile<<'\n';
  os<<"costFunction "<<p.costFunction<<'\n';
  os<<"linearConvergence "<<p.linearConvergence<<'\n';
  os<<"warpLevel "<<p.warpLevel<<'\n';
  os<<"warpConvergence "<<p.warpConvergence<<'\n';
  os<<"step "<<p.step<<'\n';
  os<<"verbosity "<<p.verbosity<<'\n';
  return os;
}

template <class T>
void loadFromString(T &f, std::string x)
{
  std::istringstream(x)>>f;
}

bool CerebroHelperParser::validate()
{
  bool fail=false;
  if (ifname.empty()) { std::cerr<<"an input file is required (-i)"<<std::endl; fail=true; }
  if (ofname.empty()) { std::cerr<<"an output file is required (-o)"<<std::endl; fail=true; }
  if (atlasfile.empty()) { std::cerr<<"an MRI brain atlas file is required (--atlas)"<<std::endl; fail=true; }
  if (atlasToBrainAirFile.empty()) { std::cerr<<"an atlas to brain AIR filename is required (--air)"<<std::endl; fail=true; }
  if (step==1)
  {
    if (useCentroids && centroidInitializerFile.empty())
      { std::cerr<<"if --cntroids is used, a centroid initializer filename is required (--centinit)"<<std::endl; fail=true; }
  }
  if (step==2)
  {
    if (atlasToBrainWarpFile.empty()) { std::cerr<<"an atlas to brain warp filename is required (--warp)"<<std::endl; fail=true; }
    if (labelfile.empty()) { std::cerr<<"for step 2, a label file must be specified"<<std::endl; fail=true; }
  }
  if (costFunction<0||costFunction>2) { std::cerr<<"cost function must be 0, 1, or 2"<<std::endl; fail=true; }
  return (!fail);
}

void CerebroHelperParser::usage()
{
  std::cout<<"cerebrohelper : cerebrum/cerebellum labeling tool\n";
  std::cout<<"Copyright (C) 2016 The Regents of the University of California\n";
  std::cout<<"Developed by David W. Shattuck, UCLA Department of Neurology\n";
  std::cout<<
    "This program performs automated labeling of cerebellum and cerebrum in T1 MRI.\n"
    "Input MRI should be skull-stripped or a brain-only mask should be provided.\n"
    "This porgram is called by the BrainSuite GUI.\n"
    "CerebroHelper is part of the BrainSuite collection of tools.\n"
    "For more information, please see: http://brainsuite.org\n\n";
  std::cout<<"usage: \n"
    "cerebrohelper [options] -i input_filename -o output_image\n"
    "options:\n"
    "-c number                cost_function (0,1,2)\n"
    "-l filename              output label file (required for step 2)\n"
    "--step number            step to perform (1,2)\n"
    "--atlas filename         MRI brain atlas\n"
    "--atlaslabels filename   brain atlas label file\n"
    "--initfile filename      AIR initialization file\n"
    "--centroids              initialize registration with centroids\n"
    "--centinit filename      centroid initializer file\n"
    "--linconv number         linear convergence\n"
    "--air filename           atlas to brain air file\n"
    "--warplevel number       warpLevel (2,3,4,5,6,7,8)\n"
    "--warpconv number        warp convergence\n"
    "--warp filename          atlas to brain warp file \n"
    "--verbosity number       0=silent, 1=important info, 2=verbose\n";
}

bool CerebroHelperParser::parse(int argc, char *argv[])
{
  for (int i=1;i<argc;i++)
  {
    if (argv[i][0]=='-')
    {
      std::string param(&argv[i][1]);
      if (param.compare("-centroids")==0) { useCentroids=true; continue; }
      char n=argv[i][1];
      if ((i+1)>=argc) { std::cerr<<param<<" requires an argument"<<std::endl; return false; }
      std::string nextarg(argv[++i]);
      switch (n)
      {
        case 'i' : ifname=nextarg; break;
        case 'o' : ofname=nextarg; break;
        case 'l' : labelfile=nextarg; break;
        case 'c' : costFunction=stoi(nextarg); break;
        case '-' : // string follows
          {
            std::string param(&argv[i-1][2]); // remove -- at start
            if (param.compare("step")==0)        { loadFromString(step,nextarg); break; }
            if (param.compare("atlas")==0)       { atlasfile = nextarg; break; }
            if (param.compare("atlaslabels")==0) { atlaslabelfile = nextarg; break; }
            if (param.compare("initfile")==0)    { initfile = nextarg; break; }
            if (param.compare("centinit")==0)    { centroidInitializerFile = nextarg; break; }
            if (param.compare("linconv")==0)     { loadFromString(linearConvergence,nextarg); break; }
            if (param.compare("air")==0)         { atlasToBrainAirFile = nextarg; break; }
            if (param.compare("warplevel")==0)   { loadFromString(warpLevel,nextarg); break; }
            if (param.compare("warpconv")==0)    { loadFromString(warpConvergence,nextarg); break; }
            if (param.compare("warp")==0)        { atlasToBrainWarpFile = nextarg; break; }
            if (param.compare("verbosity")==0)   { loadFromString(verbosity,nextarg); break; }
            std::cout<<"unrecognized parameter "<<argv[i-1]<<std::endl;
            return false;
            break;
          }
        default: std::cerr<<"unrecognized parameter "<<argv[i-1]<<std::endl;
          return false;
      }
    }
    else
    {
      std::cout<<"unrecognized argument: "<<argv[i]<<std::endl;
      return false;
    }
  }
  return true;
}

bool stepReslice(std::string ofname, std::string atlasToBrainAirFile, int verbosity)
{
  unsigned int window[]={0,0,0};
  AIR_Error code=AIR_do_reslice("reslice",
       /* airfile */ (const char *) atlasToBrainAirFile.c_str(), /* outfile */ (const char *) ofname.c_str(), /*alternate_reslice_file*/(const char *) NULL,
       /*interp*/1,window,/*cubic*/FALSE,1.0f/*scale*/,NULL/*mult_scale_file*/,NULL/*div_scale_file*/,TRUE,
       0,0.0,0.0,0,0.0,0.0,0,0.0,0.0); // x_dimout,x_distance,x_shift, y_dimout,y_distance,y_shift, z_dimout,z_distance,z_shift
  if (verbosity>1) std::cout<<"process returned "<<code<<std::endl;
  return code;
}

int stepAlignLinear(CerebroHelperParser &parser)
{
  if (parser.costFunction==2&&parser.verbosity>1) std::cout<<"saving initfile: "<<parser.initfile.c_str()<<std::endl;
  int retcode= AIR_do_alignlinear("alignlinear",/*model*/12,parser.atlasToBrainAirFile.c_str(), TRUE,
    parser.ifname.c_str(), 1, 1.0f, 1.0f, 1.0f, /*partitions1*/ 1, /*dynamic1*/TRUE, /*standard_file_mask*/0, // data are masked in advance
    parser.atlasfile.c_str(), 1, 1.0f, 1.0f, 1.0f, /*partitions2*/ 1, /*dynamic2*/TRUE, /*reslice_file_mask*/0,
    parser.useCentroids ? parser.centroidInitializerFile.c_str() : 0,
    /*scaling_initialization_file*/ 0, /*termination_file*/0, /*overwrite_termination_file*/FALSE, // -fs filename [?]
    /*scaling_termination_file*/(parser.costFunction==2) ? parser.initfile.c_str() : 0, /*overwrite_scaling_termination_file*/FALSE, // -gs filename [y],
    /*samplefactor*/81, /*samplefactor2*/1, /*sffactor*/3, /*precision*/parser.linearConvergence, /*iterations*/25, /*noprogtries*/5,
    /*interaction*/ TRUE, parser.verbosity>1, /*zooming*/FALSE,
    parser.costFunction+1, /*posdefreq*/TRUE);
  if (retcode)
  {
    std::cerr<<"error in CerebrumHelper: alignlinear process failed."<<std::endl;
  }
  return retcode;
}

int step1(CerebroHelperParser &parser)
{
  int retcode=stepAlignLinear(parser);
  if (retcode) return retcode;
  retcode=stepReslice(parser.ofname, parser.atlasToBrainAirFile, parser.verbosity);
  return retcode;
}

int stepAlignWarp(CerebroHelperParser &parser)
{
  if (parser.verbosity>0)
    std::cout<<"Performing warp alignment (order="<<parser.warpLevel<<") of atlas brain to subject brain"<<std::endl;
  return AIR_do_align_warp("align_warp", 2, parser.warpLevel, parser.atlasToBrainWarpFile.c_str(), TRUE,
                        parser.ifname.c_str(), /*threshold1*/1, 0, 0, 0, /*standard_file_mask*/0,
                        parser.atlasfile.c_str(), /*threshold2*/1, 0, 0, 0, /*reslice_file_mask*/0,
                        /*lesion_file*/0, /*initialization_file*/parser.atlasToBrainAirFile.c_str(),
                        /*scaling_initialization_file*/(parser.costFunction==2) ? parser.initfile.c_str() : 0,
                        /*scaling_termination_file*/0, /*overwrite_scaling_termination_file*/0, 81, 9, 3,
                        /*precision*/parser.warpConvergence, /*iterations*/50, /*noprogtries*/2,
                        /*interaction*/TRUE, parser.verbosity>1, /*intermediate*/0, TRUE);
}

int resliceWarp(std::string ofname, std::string warpfile)
{
  unsigned int window[]={0,0,0};
  return AIR_do_reslice_warp("reslice_warp",ofname.c_str(),  0, warpfile.c_str(), /*interp*/0, window, /*scale*/1.0f, /*mult_scale_file*/NULL,/*div_scale_file*/NULL, TRUE);
}

int resliceWarp(std::string ofname, std::string atlasTemplate, std::string warpfile)
{
  unsigned int window[]={0,0,0};
  return AIR_do_reslice_warp("reslice_warp",ofname.c_str(),  atlasTemplate.c_str(), warpfile.c_str(), /*interp*/0, window, /*scale*/1.0f, /*mult_scale_file*/NULL,/*div_scale_file*/NULL, TRUE);
}

bool warpReslice(CerebroHelperParser &parser)
{
  unsigned int window[]={0,0,0};
  int code=AIR_do_reslice_warp("reslice_warp",parser.ofname.c_str(),  0, parser.atlasToBrainWarpFile.c_str(), /*interp*/0, window, /*scale*/1.0f, /*mult_scale_file*/NULL,/*div_scale_file*/NULL, TRUE);
  if (code)
  {
    std::cerr<<"error in CerebrumHelper: warp reslice process failed."<<std::endl;
  }
  return code;
}

int step2(CerebroHelperParser &parser)
{
  int retcode=stepAlignWarp(parser);
  if (retcode) return retcode;
  retcode=warpReslice(parser);
  if (retcode==0 && !parser.labelfile.empty())
  {
    if (parser.verbosity>1) std::cout<<"reslicing labels "<<parser.labelfile<<std::endl;
    int code=::resliceWarp(parser.labelfile,parser.atlaslabelfile,parser.atlasToBrainWarpFile);
  }
  return retcode;
}

int main(int argc, char *argv[])
{
  CerebroHelperParser parser;
  if (argc==1) { parser.usage(); return 0; }
  if (!parser.parse(argc,argv)) return 1;
  if (!parser.validate()) return 1;
  switch (parser.step)
  {
    case 1: return step1(parser); break;
    case 2: return step2(parser); break;
    default: std::cerr<<"step was not in range!"<<std::endl; break; // fail
  }
  return 1;
}
