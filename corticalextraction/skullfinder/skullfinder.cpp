// Copyright (C) 2016 The Regents of the University of California
//
// Created by David W. Shattuck, Ph.D.
//
// This file is part of Skullfinder.
//
// Skullfinder is free software; you can redistribute it and/or
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

#include "skullandscalptool.h"
#include "volumeloader.h"
#include <vol3dsimple.h>

#include <marchingcubes_t.h>
#include <surflib.h>

template bool MarchingCubes::cubes(Vol3D<uint8> &, SILT::Surface &, const NZ<uint8>);
template bool MarchingCubes::cubes(Vol3D<sint16> &, SILT::Surface &, const NZ<sint16>);
template bool MarchingCubes::cubes(Vol3D<uint16> &, SILT::Surface &, const NZ<uint16>);

bool tessellateMask(Surface &surface, Vol3D<uint8> &vMask)
{
  MarchingCubes marchingCubes;
  const int nIter = 10;
  marchingCubes.cubes(vMask,surface);
  SurfLib::laplaceSmooth(surface,nIter,0.5f);
  return true;
}

bool writeOrFail(SILT::Surface &surface, std::string ofname, int verbosity)
{
  if (surface.write(ofname))
  {
    if (verbosity>0) std::cout<<"wrote "<<ofname<<'\n';
    return true;
  }
  else
  {	std::cerr<<"error writing "<<ofname<<'\n'; return false; }
}

int main(int argc, char *argv[])
{
  std::string maskfilename;
  std::string surfacePrefix;
  SkullAndScalpTool skullAndScalpTool;
  ArgParser ap("skullfinder");
// attach the application specific variables to the parser
  ap.copyright = "Copyright (C) 2016 The Regents of the University of California and the University of Southern California";
  ap.bind("m",maskfilename,"<mask file>","A brain mask file, 8-bit image (0=non-brain, 255=brain)",true);
  ap.bind("l",skullAndScalpTool.lowerSkullThreshold,"<lower threshold>","Lower threshold for segmentation");
  ap.bind("u",skullAndScalpTool.upperSkullThreshold,"<upper threshold>","Upper threshold for segmentation");
  ap.bind("s",surfacePrefix,"surface_prefix","if specified, generate surface files for brain, skull, and scalp");
  ap.bind("-bglabel",skullAndScalpTool.backgroundLabelValue,"<background_label>","background label value (0-255)");
  ap.bind("-scalplabel",skullAndScalpTool.scalpLabelValue,"<scalp_label>","scalp label value (0-255)");
  ap.bind("-skulllabel",skullAndScalpTool.skullLabelValue,"<skull_label>","skull label value (0-255)");
  ap.bind("-spacelabel",skullAndScalpTool.spaceLabelValue,"<space_label>","space label value (0-255)");
  ap.bind("-brainlabel",skullAndScalpTool.brainLabelValue,"<brain_label>","brain label value (0-255)");
  ap.bindFlag("-finalOpening",skullAndScalpTool.finalOpening,"perform a final opening operation on the scalp mask");
  ap.description = "Skull and scalp segmentation algorithm. For details see Dogdas B, Shattuck DW, and Leahy RM (2005)\n"
                   "Segmentation of Skull and Scalp in 3D Human MRI Using Mathematical Morphology, Human Brain Mapping 26(4):273-85\n"
                   "http://dx.doi.org/10.1002/hbm.20159";
  ap.notes = "Supported filetype(s): Nifti, Analyze Image";

  if (!ap.parseAndValidate(argc,argv)) return ap.usage();
  skullAndScalpTool.verbosity = ap.verbosity;
  Vol3DBase *vIn = VolumeLoader::load(ap.ifname);
  if (!vIn)
    return CommonErrors::cantRead("input file: "+ap.ifname);
  if (!skullAndScalpTool.vBrainMask.read(maskfilename))
    return CommonErrors::cantRead("mask file: "+maskfilename);

  if (!skullAndScalpTool.initialize(vIn))
  {
    std::cerr<<"error initializing skull and scalp tool."<<std::endl;
    return 1;
  }
  if (!skullAndScalpTool.findScalp())
  {
    std::cerr<<"error performing scalp identification."<<std::endl;
    return 1;
  }
  if (!skullAndScalpTool.findOuterSkull())
  {
    std::cerr<<"error performing outer skull identification."<<std::endl; return 1;
  }
  if (!skullAndScalpTool.findInnerSkull())
  {
    std::cerr<<"error performing inner skull identification."<<std::endl; return 1;
  }
  if (!skullAndScalpTool.vHeadLabels.write(ap.ofname))
    return CommonErrors::cantWrite("output label file: "+ap.ofname);
  if (surfacePrefix.empty()==false)
  {
    SILT::Surface surface;
    tessellateMask(surface,skullAndScalpTool.vBrainMask);
    writeOrFail(surface,surfacePrefix + ".brain.dfs",ap.verbosity);
    tessellateMask(surface,skullAndScalpTool.vInnerSkull);
    writeOrFail(surface,surfacePrefix + ".inner_skull.dfs",ap.verbosity);
    tessellateMask(surface,skullAndScalpTool.vScalp);
    writeOrFail(surface,surfacePrefix + ".outer_skull.dfs",ap.verbosity);
    tessellateMask(surface,skullAndScalpTool.vOuterSkull);
    writeOrFail(surface,surfacePrefix + ".scalp.dfs",ap.verbosity);
  }
  return 0;
}
