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

#include <extractionstages.h>

std::string ExtractionStages::stageName(Stages stage)
{
  switch (stage)
  {
    case ExtractionStages::SkullStripping : return "Skull Stripping";
    case SkullAndScalp : return "Skull and Scalp Modeling";
    case ExtractionStages::BiasFieldCorrection : return "Nonuniformity Correction";
    case ExtractionStages::TissueClassification : return "Tissue Classification";
    case ExtractionStages::CerebrumLabeling : return "Cerebrum Labeling";
    case ExtractionStages::InnerCorticalMask : return "Inner Cortical Mask";
    case ExtractionStages::ScrubMask : return "Scrub Mask";
    case ExtractionStages::TopologyCorrection : return "Topology Correction";
    case ExtractionStages::WispFilter : return "Wisp Removal";
    case ExtractionStages::InnerCorticalSurface : return "Inner Cortical Surface";
    case ExtractionStages::PialSurface : return "Pial Surface";
    case ExtractionStages::SplitHemispheres : return "Split Hemispheres";
    case ExtractionStages::RegisterAndLabel : return "Register and Label Brain";
    case ExtractionStages::Finished : return "Finished";
    default: break;
  }
  return "Unknown Stage Code";
}

