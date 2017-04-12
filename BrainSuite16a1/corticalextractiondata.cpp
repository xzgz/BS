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

#include <corticalextractiondata.h>
#include <volumeloader.h>
#include <cerebrumlabelerqt.h>
#include <surfacealpha.h>
#include <cerebroguitool.h>

extern CerebroGUITool cerebroGUITool;

CorticalExtractionData::CorticalExtractionData() : vBSE(0), vBFC(0), innerCorticalSurface(0), pialSurface(0),
  leftInnerCorticalSurface(0), rightInnerCorticalSurface(0), leftPialSurface(0), rightPialSurface(0),
  innerSkullSurface(0), outerSkullSurface(0), scalpSurface(0), brainSurface(0)
{
}

CorticalExtractionData::~CorticalExtractionData()
{
  delete vBFC; vBFC=0;
  delete vBSE; vBSE=0;
  delete innerCorticalSurface; innerCorticalSurface=0;
  delete pialSurface; pialSurface=0;
  delete leftInnerCorticalSurface; leftInnerCorticalSurface=0;
  delete rightInnerCorticalSurface; rightInnerCorticalSurface=0;
  delete leftPialSurface; leftPialSurface=0;
  delete rightPialSurface; rightPialSurface=0;
  delete innerSkullSurface; innerSkullSurface=0;
  delete outerSkullSurface; outerSkullSurface=0;
  delete scalpSurface; scalpSurface=0;
  delete brainSurface; brainSurface=0;
}

bool CorticalExtractionData::load(std::string basename, std::ostream &errstream, Stages stage)
{
  loadStream.str("loading from basename: " + basename);
  loadStream.clear();
  if (!vBrainMask.read(basename+".mask.nii.gz")) { errstream<<"unable to read "<<basename+".mask.nii.gz"<<"\n"; return false; }
  loadStream<<"loaded skull stripping mask file: "<<StrUtil::shortName(vBrainMask.filename)<<'\n';
  Vol3DBase *vBSEnew = VolumeLoader::load(basename+".bfc.nii.gz");
  if (!vBSEnew)
  {
    errstream<<"unable to read "<<basename+".bfc.nii.gz"<<"\n";
    return false;
  }
  delete vBSE;
  vBSE=vBSEnew;
  vBSEnew=0;
  loadStream<<"loaded skull stripped image file: "<<StrUtil::shortName(vBSE->filename)<<'\n';
  if (stage==BSE) return true;
  Vol3DBase *vBFCnew = VolumeLoader::load(basename+".bfc.nii.gz");
  if (!vBFCnew)
  {
    errstream<<"unable to read "<<basename+".bfc.nii.gz"<<"\n";
    return false;
  }
  delete vBFC;
  vBFC=vBFCnew;
  vBFCnew=0;
  loadStream<<"loaded bias corrected file: "<<StrUtil::shortName(vBFC->filename)<<'\n';
  if (stage==BFC) return true;
  if (!vPVCLabels.read(basename+".pvc.label.nii.gz")) { errstream<<"unable to read "<<basename+".pvc.label.nii.gz"<<"\n"; return false; }
  loadStream<<"loaded tissue classifcation label file: "<<StrUtil::shortName(vPVCLabels.filename)<<'\n';
  if (!vPVCFractions.read(basename+".pvc.frac.nii.gz")) { errstream<<"unable to read "<<basename+".pvc.frac.nii.gz"<<"\n"; return false; }
  loadStream<<"loaded tissue fraction file: "<<StrUtil::shortName(vPVCFractions.filename)<<'\n';
  if (stage==PVC) return true;
  if (!vCerebrumMask.read(basename+".cerebrum.mask.nii.gz")) { errstream<<"unable to read "<<basename+".cerebrum.mask.nii.gz"<<"\n"; return false; }
  loadStream<<"loaded cerebrum mask file: "<<StrUtil::shortName(vCerebrumMask.filename)<<'\n';
  if (!vHemiLabels.read(basename+".hemi.label.nii.gz")) { errstream<<"unable to read "<<basename+".hemi.label.nii.gz"<<"\n"; return false; }
  loadStream<<"loaded hemisphere label file: "<<StrUtil::shortName(vHemiLabels.filename)<<'\n';
  cerebroGUITool.cerebrumLabeler.vLabeledLobes.copy(vHemiLabels);
  if (stage==CB) return true;
  if (!vCortexInitMask.read(basename+".init.cortex.mask.nii.gz")) { errstream<<"unable to read "<<basename+".init.cortex.mask.nii.gz"<<"\n"; return false; }
  loadStream<<"loaded initial cortex mask file: "<<StrUtil::shortName(vCortexInitMask.filename)<<'\n';
  if (stage==CTX) return true;
  if (!vCortexScrubbedMask.read(basename+".cortex.scrubbed.mask.nii.gz")) { errstream<<"unable to read "<<basename+"cortex.scrubbed.mask.nii.gz"<<"\n"; return false; }
  loadStream<<"loaded scrubbed cortex mask file: "<<StrUtil::shortName(vCortexScrubbedMask.filename)<<'\n';
  if (stage==ScrubMask) return true;
  if (!vCortexTCAMask.read(basename+".cortex.tca.mask.nii.gz")) { errstream<<"unable to read "<<basename+"cortex.tca.mask.nii.gz"<<"\n"; return false; }
  loadStream<<"loaded topologically corrected mask file: "<<StrUtil::shortName(vCortexTCAMask.filename)<<'\n';
  if (stage==TCA) return true;
  if (!vCortexDewispMask.read(basename+".cortex.dewisp.mask.nii.gz")) { errstream<<"unable to read "<<basename+"cortex.dewisp.mask.nii.gz"<<"\n"; return false; }
  loadStream<<"loaded dewisped cortex mask file: "<<StrUtil::shortName(vCortexDewispMask.filename)<<'\n';
  if (stage==Dewisp) return true;
  SurfaceAlpha *newInnerCortex = new SurfaceAlpha;
  if (!newInnerCortex)
  {
    errstream<<"unable to allocate memory for inner cortical surface"<<std::endl;
    return false;
  }
  if (newInnerCortex->read(basename+".inner.cortex.dfs")==false)
  {
    errstream<<"unable to read "<<basename+".inner.cortex.dfs"<<"\n";
    return false;
  }
  delete innerCorticalSurface;
  innerCorticalSurface = newInnerCortex;
  newInnerCortex=0;
  loadStream<<"loaded inner cortical surface file: "<<StrUtil::shortName(innerCorticalSurface->filename)<<'\n';
  if (stage==InnerCorticalSurface) return true;
  SurfaceAlpha *newPialSurface = new SurfaceAlpha;
  if (!newPialSurface)
  {
    errstream<<"unable to allocate memory for pial surface"<<std::endl;
    return false;
  }
  if (newPialSurface->read(basename+".pial.cortex.dfs")==false)
  {
    errstream<<"unable to read "<<basename+".pial.cortex.dfs"<<"\n";
    return false;
  }
  delete pialSurface;
  pialSurface = newPialSurface;
  newPialSurface=0;
  loadStream<<"loaded pial cortical surface file: "<<StrUtil::shortName(pialSurface->filename)<<'\n';
  if (stage==PialSurface) return true;
  return true;
}
