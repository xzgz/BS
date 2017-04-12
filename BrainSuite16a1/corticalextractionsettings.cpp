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

#include "corticalextractionsettings.h"
#include <qdir.h>
#include <brainsuiteqtsettings.h>
#include <svregguitool.h>
#include <iostream>
#include <fstream>
#include <iostream>
#include <strutil.h>

CorticalExtractionSettings::CorticalExtractionSettings()
{
}

CorticalExtractionSettings::BSE::BSE() :
  diffusionIterations(3), diffusionConstant(25.0f), edgeConstant(0.64f),
  erosionSize(1), trimSpinalCord(true), dilateFinalMask(true)
{
}

CorticalExtractionSettings::Skullfinder::Skullfinder() :
  computeThresholds(true), skullThreshold(0), scalpThreshold(0)
{
}

CorticalExtractionSettings::BFC::BFC() :
  histogramRadius(12), biasEstimateSampleSpacing(16), controlPointSpacing(64),
  splineStiffness(0.0001f), roiType(Block), biasRange(0.5f,1.5f), iterative(false)
{
}

CorticalExtractionSettings::PVC::PVC() : spatialPrior(0.10f)
{
}

CorticalExtractionSettings::Cerebrum::Cerebrum() :
  useCentroids(false), linearConvergence(0.1f), warpConvergence(100.0f), warpLevel(5), costFunction(2), verbose(false), showAtlasOverlay(true)
{
  std::string atlasPath = BrainSuiteQtSettings::qtinstalldir;
  atlasFilename = QDir::toNativeSeparators(QDir::cleanPath(std::string(atlasPath + "/atlas/brainsuite.icbm452.lpi.v08a.img").c_str())).toStdString();
  labelFilename = QDir::toNativeSeparators(QDir::cleanPath(std::string(atlasPath + "/atlas/brainsuite.icbm452.v15a.label.img").c_str())).toStdString();
}

CorticalExtractionSettings::Cortex::Cortex() :
  tissueThreshold(0.50f), includeAllSubcorticalAreas(true)
{

}

CorticalExtractionSettings::ScrubMask::ScrubMask() :
  foregroundThreshold(0), backgroundThreshold(2), nScrubIterations(10)
{

}

CorticalExtractionSettings::TCA::TCA() :
  minimumCorrectionSize(0), maximumCorrectionSize(2500), fillOffset(20)
{

}

CorticalExtractionSettings::Dewisp::Dewisp() :
  threshold(15), maximumIterations(10)
{

}

CorticalExtractionSettings::InnerCorticalSurface::InnerCorticalSurface() :
  smoothingIterations(10), smoothingConstant(0.5f), curvatureWeight(5.0f)
{
}

CorticalExtractionSettings::Pial::Pial() :
  pialPresmoothIterations(80),
  pialIterations(100),
  pialCollisionDetectionRadius(1.0f),
  pialThicknessLimit(20.0f),
  pialStepSize(0.40f),
  pialTissueThreshold(1.05f),
  pialSmoothingConstant(0.025f),
  pialRadialConstant(0.20f),
  pialUpdateSurfaceInterval(10),
  pialConstrainWithCerebrumMask(true)
{

}

extern SVRegGUITool svregGUITool;

CorticalExtractionSettings::SVReg::SVReg() :
  surfaceOnly(false),
  refineSulci(false),
  extendLabels(false),
  computeStats(true),
  removeTempFiles(true),
  atlasIndex(0)
{
  if (atlasIndex>=0 && atlasIndex<(int)svregGUITool.svreg.svregManifest.atlases.size())
  {
    atlasName=svregGUITool.svreg.svregManifest.atlases[atlasIndex].name;
  }
}

bool CorticalExtractionSettings::saveAsPlainText(std::string ofname)
{
  std::ofstream ofile(ofname);
  if (!ofile) { std::cerr<<"error: could not open "<<ofname<<std::endl; return false; }
  ofile<<"BSE.diffusionIterations "<<bse.diffusionIterations<<'\n';
  ofile<<"BSE.diffusionConstant "<<bse.diffusionConstant<<'\n';
  ofile<<"BSE.edgeConstant "<<bse.edgeConstant<<'\n';
  ofile<<"BSE.erosionSize "<<bse.erosionSize<<'\n';
  ofile<<"BSE.trimSpinalCord "<<bse.trimSpinalCord<<'\n';
  ofile<<"BSE.dilateFinalMask "<<bse.dilateFinalMask<<'\n';
  ofile<<"Skullfinder.computeThresholds "<<skullfinder.computeThresholds<<'\n';
  ofile<<"Skullfinder.skullThreshold "<<skullfinder.skullThreshold<<'\n';
  ofile<<"Skullfinder.scalpThreshold "<<skullfinder.scalpThreshold<<'\n';
  ofile<<"BFC.histogramRadius "<<bfc.histogramRadius<<'\n';
  ofile<<"BFC.biasEstimateSampleSpacing "<<bfc.biasEstimateSampleSpacing<<'\n';
  ofile<<"BFC.controlPointSpacing "<<bfc.controlPointSpacing<<'\n';
  ofile<<"BFC.splineStiffness "<<bfc.splineStiffness<<'\n';
  ofile<<"BFC.roiType "<<bfc.roiType<<'\n';
  ofile<<"BFC.biasRange "<<bfc.biasRange.first<<" "<<bfc.biasRange.second<<'\n';
  ofile<<"BFC.iterative "<<bfc.iterative<<'\n';
  ofile<<"PVC.spatialPrior "<<pvc.spatialPrior<<"\n";
  ofile<<"Cerebrum.useCentroids "<<cbm.useCentroids<<"\n";
  ofile<<"Cerebrum.linearConvergence "<<cbm.linearConvergence<<"\n";
  ofile<<"Cerebrum.warpConvergence "<<cbm.warpConvergence<<"\n";
  ofile<<"Cerebrum.warpLevel "<<cbm.warpLevel<<"\n";
  ofile<<"Cerebrum.costFunction "<<cbm.costFunction<<"\n";
  ofile<<"Cerebrum.atlasFilename "<<cbm.atlasFilename<<"\n";
  ofile<<"Cerebrum.labelFilename "<<cbm.labelFilename<<"\n";
  ofile<<"Cerebrum.tempDirectory "<<cbm.tempDirectory<<"\n";

  ofile<<"Cortex.tissueThreshold "<<ctx.tissueThreshold<<"\n";
  ofile<<"Cortex.includeAllSubcorticalAreas "<<ctx.includeAllSubcorticalAreas<<"\n";
  ofile<<"ScrubMask.foregroundThreshold "<<scrubMask.foregroundThreshold<<"\n";
  ofile<<"ScrubMask.backgroundThreshold "<<scrubMask.backgroundThreshold<<"\n";
  ofile<<"ScrubMask.nScrubIterations "<<scrubMask.nScrubIterations<<"\n";
  ofile<<"TCA.minimumCorrectionSize "<<tca.minimumCorrectionSize<<"\n";
  ofile<<"TCA.maximumCorrectionSize "<<tca.maximumCorrectionSize<<"\n";
  ofile<<"TCA.fillOffset "<<tca.fillOffset<<"\n";

  ofile<<"Dewisp.threshold "<<dewisp.threshold<<"\n";
  ofile<<"Dewisp.maximumIterations "<<dewisp.maximumIterations<<"\n";

  ofile<<"InnerCorticalSurface.smoothingIterations "<<ics.smoothingIterations<<"\n";
  ofile<<"InnerCorticalSurface.smoothingConstant "<<ics.smoothingConstant<<"\n";
  ofile<<"InnerCorticalSurface.curvatureWeight "<<ics.curvatureWeight<<"\n";

  ofile<<"Pial.pialPresmoothIterations "<<pial.pialPresmoothIterations<<"\n";
  ofile<<"Pial.pialIterations "<<pial.pialIterations<<"\n";
  ofile<<"Pial.pialCollisionDetectionRadius "<<pial.pialCollisionDetectionRadius<<"\n";
  ofile<<"Pial.pialThicknessLimit "<<pial.pialThicknessLimit<<"\n";
  ofile<<"Pial.pialStepSize "<<pial.pialStepSize<<"\n";
  ofile<<"Pial.pialTissueThreshold "<<pial.pialTissueThreshold<<"\n";
  ofile<<"Pial.pialSmoothingConstant "<<pial.pialSmoothingConstant<<"\n";
  ofile<<"Pial.pialRadialConstant "<<pial.pialRadialConstant<<"\n";
  ofile<<"Pial.pialUpdateSurfaceInterval "<<pial.pialUpdateSurfaceInterval<<"\n";
  ofile<<"Pial.pialConstrainWithCerebrumMask "<<pial.pialConstrainWithCerebrumMask<<"\n";
  ofile<<"SVReg.surfaceOnly "<<svreg.surfaceOnly<<"\n";
  ofile<<"SVReg.refineSulci "<<svreg.refineSulci<<"\n";
  ofile<<"SVReg.extendLabels "<<svreg.extendLabels<<"\n";
  ofile<<"SVReg.computeStats "<<svreg.computeStats<<"\n";
  ofile<<"SVReg.removeTempFiles "<<svreg.removeTempFiles<<"\n";
  ofile<<"SVReg.atlasName "<<svreg.atlasName<<"\n";
  ofile<<"SVReg.atlasIndex "<<svreg.atlasIndex<<"\n";
  return true;
}

bool CorticalExtractionSettings::readFromPlainText(std::string ifname)
{
  std::ifstream ifile(ifname);
  if (!ifile) return 1;
  std::string line;
  ifile>>std::ws;
  while (!ifile.eof())
  {
    std::getline(ifile,line);
    std::istringstream istring(line);
    std::string key;
    istring>>key;
    if (key[0]=='#') {}
    else if (StrUtil::eqnocase(key,"BSE.diffusionIterations")) istring>>bse.diffusionIterations;
    else if (StrUtil::eqnocase(key,"BSE.diffusionConstant")) istring>>bse.diffusionConstant;
    else if (StrUtil::eqnocase(key,"BSE.edgeConstant")) istring>>bse.edgeConstant;
    else if (StrUtil::eqnocase(key,"BSE.erosionSize")) istring>>bse.erosionSize;
    else if (StrUtil::eqnocase(key,"BSE.trimSpinalCord")) istring>>bse.trimSpinalCord;
    else if (StrUtil::eqnocase(key,"BSE.dilateFinalMask")) istring>>bse.dilateFinalMask;
    else if (StrUtil::eqnocase(key,"Skullfinder.computeThresholds")) istring>>skullfinder.computeThresholds;
    else if (StrUtil::eqnocase(key,"Skullfinder.skullThreshold")) istring>>skullfinder.skullThreshold;
    else if (StrUtil::eqnocase(key,"Skullfinder.scalpThreshold")) istring>>skullfinder.scalpThreshold;
    else if (StrUtil::eqnocase(key,"BFC.histogramRadius")) istring>>bfc.histogramRadius;
    else if (StrUtil::eqnocase(key,"BFC.biasEstimateSampleSpacing")) istring>>bfc.biasEstimateSampleSpacing;
    else if (StrUtil::eqnocase(key,"BFC.controlPointSpacing")) istring>>bfc.controlPointSpacing;
    else if (StrUtil::eqnocase(key,"BFC.splineStiffness")) istring>>bfc.splineStiffness;
    else if (StrUtil::eqnocase(key,"BFC.roiType")) { int roitype=0; istring>>roitype; bfc.roiType=roitype ? BFC::Block : BFC::Ellipsoid; }
    else if (StrUtil::eqnocase(key,"BFC.biasRange")) istring>>bfc.biasRange.first>>std::ws>>bfc.biasRange.second;
    else if (StrUtil::eqnocase(key,"BFC.iterative")) istring>>bfc.iterative;
    else if (StrUtil::eqnocase(key,"PVC.spatialPrior")) istring>>pvc.spatialPrior;
    else if (StrUtil::eqnocase(key,"Cerebrum.useCentroids")) istring>>cbm.useCentroids;
    else if (StrUtil::eqnocase(key,"Cerebrum.linearConvergence")) istring>>cbm.linearConvergence;
    else if (StrUtil::eqnocase(key,"Cerebrum.warpConvergence")) istring>>cbm.warpConvergence;
    else if (StrUtil::eqnocase(key,"Cerebrum.warpLevel")) istring>>cbm.warpLevel;
    else if (StrUtil::eqnocase(key,"Cerebrum.costFunction")) istring>>cbm.costFunction;
    else if (StrUtil::eqnocase(key,"Cerebrum.atlasFilename")) istring>>cbm.atlasFilename;
    else if (StrUtil::eqnocase(key,"Cerebrum.labelFilename")) istring>>cbm.labelFilename;
    else if (StrUtil::eqnocase(key,"Cerebrum.tempDirectory")) istring>>cbm.tempDirectory;

    else if (StrUtil::eqnocase(key,"Cortex.tissueThreshold")) istring>>ctx.tissueThreshold;
    else if (StrUtil::eqnocase(key,"Cortex.includeAllSubcorticalAreas")) istring>>ctx.includeAllSubcorticalAreas;
    else if (StrUtil::eqnocase(key,"ScrubMask.foregroundThreshold")) istring>>scrubMask.foregroundThreshold;
    else if (StrUtil::eqnocase(key,"ScrubMask.backgroundThreshold")) istring>>scrubMask.backgroundThreshold;
    else if (StrUtil::eqnocase(key,"ScrubMask.nScrubIterations")) istring>>scrubMask.nScrubIterations;
    else if (StrUtil::eqnocase(key,"TCA.minimumCorrectionSize")) istring>>tca.minimumCorrectionSize;
    else if (StrUtil::eqnocase(key,"TCA.maximumCorrectionSize")) istring>>tca.maximumCorrectionSize;
    else if (StrUtil::eqnocase(key,"TCA.fillOffset")) istring>>tca.fillOffset;

    else if (StrUtil::eqnocase(key,"Dewisp.threshold")) istring>>dewisp.threshold;
    else if (StrUtil::eqnocase(key,"Dewisp.maximumIterations")) istring>>dewisp.maximumIterations;

    else if (StrUtil::eqnocase(key,"InnerCorticalSurface.smoothingIterations")) istring>>ics.smoothingIterations;
    else if (StrUtil::eqnocase(key,"InnerCorticalSurface.smoothingConstant")) istring>>ics.smoothingConstant;
    else if (StrUtil::eqnocase(key,"InnerCorticalSurface.curvatureWeight")) istring>>ics.curvatureWeight;

    else if (StrUtil::eqnocase(key,"Pial.pialPresmoothIterations")) istring>>pial.pialPresmoothIterations;
    else if (StrUtil::eqnocase(key,"Pial.pialIterations")) istring>>pial.pialIterations;
    else if (StrUtil::eqnocase(key,"Pial.pialCollisionDetectionRadius")) istring>>pial.pialCollisionDetectionRadius;
    else if (StrUtil::eqnocase(key,"Pial.pialThicknessLimit")) istring>>pial.pialThicknessLimit;
    else if (StrUtil::eqnocase(key,"Pial.pialStepSize")) istring>>pial.pialStepSize;
    else if (StrUtil::eqnocase(key,"Pial.pialTissueThreshold")) istring>>pial.pialTissueThreshold;
    else if (StrUtil::eqnocase(key,"Pial.pialSmoothingConstant")) istring>>pial.pialSmoothingConstant;
    else if (StrUtil::eqnocase(key,"Pial.pialRadialConstant")) istring>>pial.pialRadialConstant;
    else if (StrUtil::eqnocase(key,"Pial.pialUpdateSurfaceInterval")) istring>>pial.pialUpdateSurfaceInterval;
    else if (StrUtil::eqnocase(key,"Pial.pialConstrainWithCerebrumMask")) istring>>pial.pialConstrainWithCerebrumMask;
    else if (StrUtil::eqnocase(key,"SVReg.surfaceOnly")) istring>>svreg.surfaceOnly;
    else if (StrUtil::eqnocase(key,"SVReg.refineSulci")) istring>>svreg.refineSulci;
    else if (StrUtil::eqnocase(key,"SVReg.extendLabels")) istring>>svreg.extendLabels;
    else if (StrUtil::eqnocase(key,"SVReg.computeStats")) istring>>svreg.computeStats;
    else if (StrUtil::eqnocase(key,"SVReg.removeTempFiles")) istring>>svreg.removeTempFiles;
    else if (StrUtil::eqnocase(key,"SVReg.atlasName")) istring>>svreg.atlasName;
    else if (StrUtil::eqnocase(key,"SVReg.atlasIndex")) istring>>svreg.atlasIndex;
    else std::cerr<<"unrecognized key "<<key<<" with value "<<istring.str()<<std::endl;
    ifile>>std::ws;
  }
  return true;
}
