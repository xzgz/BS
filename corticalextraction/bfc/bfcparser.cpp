// Copyright (C) 2016 The Regents of the University of California
//
// Created by David W. Shattuck, Ph.D.
//
// This file is part of BFC.
//
// BFC is free software; you can redistribute it and/or
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

#include <BFC/biasfieldcorrector.h>
#include "bfcparser.h"

BFCParser::BFCParser(BiasFieldCorrector &bfc) : ArgParserBase("bfc")
{
// inputs and outputs remain local
  copyright = "Copyright (C) 2015 The Regents of the University of California and the University of Southern California";
  description="bias field corrector (BFC)\n"
              "This program corrects gain variation in T1-weighted MRI.\n"
              "BFC is part of the BrainSuite collection of tools.\n"
              "For more information, please see: http://brainsuite.org";
  bind("i",ifname,"<input filename>","input skull-stripped MRI volume",true);
  bind("o",ofname,"<output filename>","output bias-corrected MRI volume",true);
  bind("m",mfname,"<mask filename>","mask file",false);
  bind("-bias",bfname,"<output filename>","save bias field estimate",false);
  bind("-maskedbias",mbfname,"<output filename>","save bias field estimate (masked)",false);
  bind("r",bfc.histogramRadius,"<radius>","histogram radius (voxels)",false);
  bind("s",bfc.biasEstimateSpacing,"<spacing>","bias sample spacing (voxels)",false);
  bind("c",bfc.controlPointSpacing,"<spacing>","control point spacing (voxels)",false);
  bind("w",bfc.splineLambda,"<strength>","spline stiffness weighting parameter",false);
  bindEnum("-ellipse",bfc.roiType,BiasFieldCorrector::Ellipsoid,"use ellipsoid for ROI histogram",false);
  bindEnum("-block",bfc.roiType,BiasFieldCorrector::Block,"use block for ROI histogram",false);
  bindFlag("-iterate",bfc.iterative,"iterative mode (overrides -r, -s, -c, -w settings)",false);
  bind("-schedule",correctionSchedule,"<schedule_file>","list of parameters ",false,false);
  bind("-biasprefix",bfc.biasPrefix,"<prefix>","save iterative bias field estimates as <prefix>.n.field.nii.gz",false,false);
  bind("-prefix",bfc.imagePrefix,"<prefix>","save iterative corrected images as <prefix>.n.bfc.nii.gz",false,false);
  bindFlag("-extrapolate",bfc.correctWholeVolume,"apply correction field to entire volume",false);

  bind("L",bfc.biasRange.first,"<lower>","minimum allowed bias value",false,false);
  bind("U",bfc.biasRange.second,"<upper>","maximum allowed bias value",false,false);
  bindEnum("-low",bfc.biasRange,std::pair<float,float>(0.95f,1.05f),"small bias model [0.95,1.05]",false);
  bindEnum("-medium",bfc.biasRange,std::pair<float,float>(0.90f,1.10f),"medium bias model [0.90,1.10]",false);
  bindEnum("-high",bfc.biasRange,std::pair<float,float>(0.80f,1.20f),"high bias model [0.80,1.20]",false);

  bindEnum("-analyze",bfc.outputType,BiasFieldCorrector::Analyze,"generate intermediate files in Analyze format",false);
  bindEnum("-nifti",bfc.outputType,BiasFieldCorrector::Nifti,"generate intermediate files in Nifti format",false);
  bindEnum("-analyzegz",bfc.outputType,BiasFieldCorrector::AnalyzeGZ,"generate intermediate files in gzipped Analyze format",false);
  bindEnum("-niftigz",bfc.outputType,BiasFieldCorrector::NiftiGZ,"generate intermediate files in gzipped Nifti format",false);

  bind("-eps",bfc.epsilon,"epsilon","convergence threshold",false,false);
  bind("-beps",bfc.biasEpsilon,"bias_epsilon","bias estimate convergence threshold\n"
                                              "                       (values > 0.1 disable)",false,false);

  bind("v",bfc.verbosity,"<number>","verbosity level (0=silent)",false); // was q, warninglevel
  bindFlag("g",bfc.debug,"debug mode",true);
  bindFlag("-timer",bfc.timer,"display timing information",false);

  notes="For anisotropic voxels, distances are scaled based on the smallest dimension.\n"
        "Iterative mode will run multiple passes of BFC using a range of settings\n"
        "to correct for severe artifacts.\n";
}
