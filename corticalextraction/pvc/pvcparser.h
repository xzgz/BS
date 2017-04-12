// Copyright (C) 2016 The Regents of the University of California
//
// Created by David W. Shattuck, Ph.D.
//
// This file is part of PVC.
//
// PVC is free software; you can redistribute it and/or
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

#ifndef PVCParser_H
#define PVCParser_H

#include <argparser.h>

class PVCParser : public ArgParserBase { 
public:
  PVCParser() : ArgParserBase("pvc"), lambda(0.1f), threeClass(false), verbosity(1), timer(false)
  {
    copyright = "Copyright (C) 2016 The Regents of the University of California and the University of Southern California";
    description="partial volume classifier (PVC) tool.\n"
                "This program performs voxel-wise tissue classification T1-weighted MRI.\n"
                "Image should be skull-stripped and bias-corrected before tissue classification.\n"
                "PVC is part of the BrainSuite collection of tools.\n"
                "For more information, please see: http://brainsuite.org";

    bind("i",ifname,"<input MRI>","MRI file",true,false);
    bind("m",mfname,"<mask file>","brain mask file",false,false);
    bind("o",ofname,"<label file>","output label file",false,false);
    bind("f",fracfname,"<fraction file>","output tissue fraction file",false,false);
    bind("l",lambda,"<lambda>","spatial prior strength",false,false);
    bind("v",verbosity,"<level>","verbosity level (0 = silent)",false,false);
    bind("-init",initFilename,"<initfile>","initialization file",false,false);
    bindFlag("3",threeClass,"use a three-class (CSF=0,GM=1,WM=2) labeling",false);
    bindFlag("-timer",timer,"time processing",false);
    notes = "Input MRI should be skull stripped or a mask should be specified.\n"
            "Output tissue label volume is an 8-bit label file using the values:\n"
            "  0 (CSF),   1 (GM),        2 (WM), 3 (GM/CSF),\n"
            "  4 (GM/WM), 5 (CSF/Other), 8 (Background)\n"
            "Tissue fractions are saved as a 32-bit floating point volume:\n"
            "  0 (BKG) 1  (CSF) <-> 2 (GM) <-> 3 (WM)\n";

    example = progname + " -i mri.nii.gz -o mri.label.nii.gz -f mri.frac.nii.gz";
  }
  virtual bool validate()
  {
    bool code = ArgParserBase::validate();
    if (ofname.empty()&&fracfname.empty()) { std::cerr<<"no output file specified."<<std::endl; code = false; }
    if (threeClass && fracfname.empty()==false)
    {
      std::cerr<<"warning: options -3 and -f are not compatible.  ignoring -3."<<std::endl;
      threeClass = false;
    }
    return code;
  }
  std::string ifname;
  std::string ofname;
  std::string mfname;
  std::string initFilename;
  std::string fracfname;
  float lambda;
  bool threeClass;
  int verbosity;
  bool timer;
};

#endif
