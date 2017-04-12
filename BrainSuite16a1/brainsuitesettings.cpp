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

#include "brainsuitesettings.h"

bool BrainSuiteSettings::noConsole = false;
bool BrainSuiteSettings::showAnnotations = true;
bool BrainSuiteSettings::showCurves = true;
bool BrainSuiteSettings::showSurfaces = true;
bool BrainSuiteSettings::showFibers = true;
bool BrainSuiteSettings::showCurvesOnImageSlices = true;
bool BrainSuiteSettings::showFibersAsTubes = false;
bool BrainSuiteSettings::showCurvesAsTubes = true;
float BrainSuiteSettings::curveTubeSize = 0.2f;
float BrainSuiteSettings::defaultCurveWidth=2.0f;
bool BrainSuiteSettings::advancedDebugging = false;
bool BrainSuiteSettings::pretrack = false;
bool BrainSuiteSettings::autofitOnResize = true;
float BrainSuiteSettings::ellipseRadius = 10.0f;
int BrainSuiteSettings::glyphQuality = 1;
int BrainSuiteSettings::glyphMode = 3;
bool BrainSuiteSettings::drawTensorsXY=true;
bool BrainSuiteSettings::drawTensorsXZ=true;
bool BrainSuiteSettings::drawTensorsYZ=true;

bool BrainSuiteSettings::curveProtocolTrackPosition = true;
bool BrainSuiteSettings::curveProtocolDrawLabels = true;
int BrainSuiteSettings::curveProtocolBrushSize = 2;
bool BrainSuiteSettings::useSliceOpacity=false;
float BrainSuiteSettings::sliceOpacity=0.5f;

BrainSuiteSettings::Clipping BrainSuiteSettings::clipXPlane = BrainSuiteSettings::Off;
BrainSuiteSettings::Clipping BrainSuiteSettings::clipYPlane = BrainSuiteSettings::Off;
BrainSuiteSettings::Clipping BrainSuiteSettings::clipZPlane = BrainSuiteSettings::Off;

float BrainSuiteSettings::clipXPlaneOffset = 0.075f;
float BrainSuiteSettings::clipYPlaneOffset = 0.075f;
float BrainSuiteSettings::clipZPlaneOffset = 0.075f;
DSPoint BrainSuiteSettings::pendingCurveColor = DSPoint(1,1,1);
int BrainSuiteSettings::sliderWidth=24;
int BrainSuiteSettings::coordWidth = 16;
int BrainSuiteSettings::textOffset=16;
bool BrainSuiteSettings::showCoords = false;
bool BrainSuiteSettings::showOrientation = true;
uint8 BrainSuiteSettings::textureAlpha=128;
float BrainSuiteSettings::rotationSpeed=0;
bool BrainSuiteSettings::cycling=false;
UVPoint BrainSuiteSettings::uvSpeed(0,0);
bool BrainSuiteSettings::cycleSurfacesFlag = true;

float BrainSuiteSettings::material_shininess = 0;
bool BrainSuiteSettings::showTensors=true;
bool BrainSuiteSettings::clipTensors=false;
bool BrainSuiteSettings::showODFs=true;
bool BrainSuiteSettings::clipODFs=false;
bool BrainSuiteSettings::useColorFA=true;
bool BrainSuiteSettings::clipFibers=true;

int BrainSuiteSettings::alphasortMode=1;
bool BrainSuiteSettings::ignoreVolumePosition=true;
bool BrainSuiteSettings::showTensorAxes = false;
bool BrainSuiteSettings::wireframeTensors = false;
bool BrainSuiteSettings::animated=false;

float BrainSuiteSettings::tubeSize=0.2f;
int BrainSuiteSettings::nCylinderPanels=10;
bool BrainSuiteSettings::fiberWireframe=false;
bool BrainSuiteSettings::useLighting=true;
BrainSuiteSettings::TrackColorModel BrainSuiteSettings::trackColorModel=BrainSuiteSettings::AverageAngle;
bool BrainSuiteSettings::renderSlices=true;
bool BrainSuiteSettings::maskUndoDisabled=false;
bool BrainSuiteSettings::disableInternetChecking=false;
bool BrainSuiteSettings::zoomBestFitOnResize=true;

DSPoint BrainSuiteSettings::defaultFiberColor(0.5f,0.5f,0.5f);
float BrainSuiteSettings::fiberLineWidth=1;
bool BrainSuiteSettings::showConnectivityNodesUsingLabelColors=false;
bool BrainSuiteSettings::showTrackSubset = false;
std::string BrainSuiteSettings::defaultCerebrumAtlasBrain;
std::string BrainSuiteSettings::defaultCerebrumAtlasTemplate;
bool BrainSuiteSettings::autosaveCorticalExtraction=true;
bool BrainSuiteSettings::depthSortSurfaceTriangles=true;
std::string BrainSuiteSettings::autosavePrefix="";
bool BrainSuiteSettings::dontAutoScale=false;
