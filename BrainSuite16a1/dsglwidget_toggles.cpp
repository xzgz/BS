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

#include "dsglwidget.h"
#include <brainsuitewindow.h>
#include <brainsuitesettings.h>
#include <diffusiontoolboxform.h>

void DSGLWidget::toggleShowSurfaces()
{
  BrainSuiteSettings::showSurfaces ^= true;
  update();
}

void DSGLWidget::toggleShowFibers()
{
  BrainSuiteSettings::showFibers ^= true;
  update();
}

void DSGLWidget::toggleShowTensorGlyphs()
{
  if (!BrainSuiteSettings::showTensors)
  {
    BrainSuiteSettings::showTensors=true;
    if (BrainSuiteSettings::glyphQuality>3||BrainSuiteSettings::glyphQuality<0)
      BrainSuiteSettings::glyphQuality = 1;
  }
  else
  {
    BrainSuiteSettings::glyphQuality++;
    if (BrainSuiteSettings::glyphQuality>3)
    {
      BrainSuiteSettings::glyphQuality = 0;
      BrainSuiteSettings::showTensors = false;
    }
    else
      BrainSuiteSettings::showTensors = true;
  }
  if (brainSuiteWindow->toolManager.diffusionToolboxForm)
    brainSuiteWindow->toolManager.diffusionToolboxForm->updateDiffusionProperties();
  update();
}

void DSGLWidget::toggleShowHARDI()
{
  BrainSuiteSettings::showODFs ^= true;
  if (brainSuiteWindow->toolManager.diffusionToolboxForm)
    brainSuiteWindow->toolManager.diffusionToolboxForm->updateDiffusionProperties();
  update();
}

void DSGLWidget::toggleShowVolumes()
{
  surfaceViewProperties.showXYPlane ^= true;
  surfaceViewProperties.showXZPlane ^= true;
  surfaceViewProperties.showYZPlane ^= true;
  update();
}


void DSGLWidget::resetView()
{
  glViewController.properties.xShift = 0.0f;
  glViewController.properties.yShift = 0.0f;
  BrainSuiteSettings::clipXPlane = BrainSuiteSettings::Off;
  BrainSuiteSettings::clipYPlane = BrainSuiteSettings::Off;
  BrainSuiteSettings::clipZPlane = BrainSuiteSettings::Off;
  BrainSuiteSettings::clipXPlaneOffset = 0.05f;
  BrainSuiteSettings::clipYPlaneOffset = 0.05f;
  BrainSuiteSettings::clipZPlaneOffset = 0.05f;
  update();
}

