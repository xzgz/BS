// Copyright (C) 2016 The Regents of the University of California
//
// Created by David W. Shattuck, Ph.D.
//
// This file is part of Pialmesh.
//
// Pialmesh is free software; you can redistribute it and/or
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

#include <cmath>
#include <DS/timer.h>
#include <vol3dsimple.h>
#include <surflib.h>
#include <surfaceexpander.h>
#include "pialmeshparser.h"
#include <colormap.h>

bool colorThickness(SILT::Surface &surface1, const SILT::Surface &surface2,
                    const float lower=0.0f, const float upper=5.0f, uint32 *LUT = ColorMap::jetLUT)
// colors by linked distance
{
  if (!LUT) LUT = ColorMap::jetLUT;
  if (surface1.nv() != surface2.nv())
  {
    std::cerr<<"surfaces must have equal number of vertices."<<std::endl;
    return false;
  }
  const size_t nv=surface1.nv();
  surface1.vertexAttributes.resize(nv);
  for (size_t i=0;i<nv;i++)
  {
    surface1.vertexAttributes[i] = (surface1.vertices[i] - surface2.vertices[i]).mag();
  }
  surface1.vertexColor.resize(nv);
  for (size_t i=0;i<nv;i++)
  {
    surface1.vertexColor[i] = ColorMap::colormap(LUT,surface1.vertexAttributes[i],lower,upper);
  }
  return true;
}

void copySurf(SILT::Surface &second, const SILT::Surface &first)
{
  second.vertices=first.vertices;
  second.triangles=first.triangles;
  second.vertexColor=first.vertexColor;
  second.computeConnectivity();
  second.computeNormals();
  second.filename = "<gm surface>";
}

int main(int argc, char *argv[])
{
  SurfaceExpander surfaceExpander;
  PialMeshParser parser;
  parser.description = "pialmesh computes a pial surface model using an inner WM/GM mesh and a tissue fraction map.\n"
                       "pialmesh is part of the BrainSuite collection of tools.\n"
                       "For more information, please see: http://brainsuite.org";
  parser.bindFlag("-norm",surfaceExpander.computeNormals,"recompute normals at each iteration");
  parser.bind("-nc",surfaceExpander.radConstant,"normal_constant","strength of normal smoother.");
  parser.bind("-tc",surfaceExpander.tanConstant,"tangential_constant","strength of tangential smoother.");

  if (!parser.parseAndValidate(argc, argv)) return parser.usage();
  Timer t;
  t.start();
  SILT::Surface sIn,sOut;
  Vol3D<float> vTissue;
  Vol3D<uint8> vMask;

  surfaceExpander.firstVertex = parser.firstVertex;
  surfaceExpander.lastVertex = parser.lastVertex;
  surfaceExpander.maximumThickness = parser.maximumThickness;
  surfaceExpander.nSteps = parser.nSteps;
  surfaceExpander.searchRadius = parser.searchRadius;
  surfaceExpander.smoothingConstant = parser.smoothingConstant;
  surfaceExpander.stepSize = parser.stepSize;
  surfaceExpander.timer = parser.timer;
  surfaceExpander.tissueThreshold = parser.tissueThreshold;
  surfaceExpander.verbosity = parser.verbosity;

  if (!vTissue.read(parser.fracfname))  return CommonErrors::cantRead(parser.fracfname);
  if (!parser.maskfname.empty())
  {
    if (!vMask.read(parser.maskfname)) return CommonErrors::cantRead(parser.maskfname);
    if (!vMask.isCompatible(vTissue))
    {
      std::cerr<<vMask.filename<<" is incompatible with "<<vTissue.filename<<std::endl;
      return 1;
    }
    vTissue.maskWith(vMask);
  }
  if (!sIn.read(parser.ifname))  return CommonErrors::cantRead(parser.ifname);
  sIn.computeConnectivity();
  copySurf(sOut,sIn);
  SurfLib::laplaceSmoothCurvatureWeighted(sOut,80,0.75f,5.0f);
  sOut.vertexColor.resize(sOut.nv());
  std::fill(sOut.vertexColor.begin(),sOut.vertexColor.end(),DSPoint(1,0,1));
  sOut.computeNormals();
  SILT::Surface sSave;
  for (int i=0;i<parser.nSteps;i++)
  {
    if (parser.verbosity>0) std::cout<<i<<" : "<<surfaceExpander.status.c_str()<<std::endl;
    if ((parser.outputFrequency>0)&&parser.prefix.empty()==false)
    {
      if ((i % parser.outputFrequency)	== 0)
      {
        std::ostringstream stepofname;
        stepofname<<parser.prefix<<"_"<<i<<".dfs";
        copySurf(sSave,sOut);
        sSave.computeNormals();
        sSave.write(stepofname.str());
      }
    }
    surfaceExpander.step(sOut,sIn,vTissue);
  }
  if ((parser.outputFrequency>0)&&parser.prefix.empty()==false)
  {
    std::ostringstream stepofname;
    stepofname<<parser.prefix<<"_"<<parser.nSteps<<".dfs";
    sOut.write(stepofname.str());
  }
  colorThickness(sOut,sIn);
  sOut.computeNormals();
  sOut.write(parser.ofname);
  t.stop();
  if (parser.timer) std::cout<<"Elapsed time: "<<t.elapsed()<<std::endl;
  return 0;
}
