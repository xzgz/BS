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

#include <hardirenderer.h>
#include <iostream>
#include <vol3d_t.h>

HARDIRenderer::HARDIRenderer(HARDITool &hardiTool) : hardiTool(hardiTool), drawXY(true), drawXZ(true), drawYZ(true), radius(10), scale(1.0f), dirty(true)
{
}

void HARDIRenderer::updateGlyphs(bool recomputeBasis)
{
  if (hardiTool.vODF.size()>0)
  {
    if (recomputeBasis)
    {
      hardiTool.odfFactory.makeBasis(hardiTool.samplingRes);
    }
    if (drawXY) hardiTool.odfFactory.makeGlyphs(odfInstancesXY,hardiTool.vODF,pCenter-IPoint3D(radius,radius,0),pCenter+IPoint3D(radius,radius,0));
    if (drawXZ) hardiTool.odfFactory.makeGlyphs(odfInstancesXZ,hardiTool.vODF,pCenter-IPoint3D(radius,0,radius),pCenter+IPoint3D(radius,0,radius));
    if (drawYZ) hardiTool.odfFactory.makeGlyphs(odfInstancesYZ,hardiTool.vODF,pCenter-IPoint3D(0,radius,radius),pCenter+IPoint3D(0,radius,radius));
    dirty = false;
  }
}

void HARDIRenderer::updateGlyphs(Vol3D<uint8> &vMask, bool recomputeBasis)
{
  if (hardiTool.vODF.size()>0)
  {
    if (recomputeBasis)
    {
      hardiTool.odfFactory.makeBasis(hardiTool.samplingRes);
    }
    if (drawXY) hardiTool.odfFactory.makeGlyphs(odfInstancesXY,hardiTool.vODF,vMask,pCenter-IPoint3D(radius,radius,0),pCenter+IPoint3D(radius,radius,0));
    if (drawXZ) hardiTool.odfFactory.makeGlyphs(odfInstancesXZ,hardiTool.vODF,vMask,pCenter-IPoint3D(radius,0,radius),pCenter+IPoint3D(radius,0,radius));
    if (drawYZ) hardiTool.odfFactory.makeGlyphs(odfInstancesYZ,hardiTool.vODF,vMask,pCenter-IPoint3D(0,radius,radius),pCenter+IPoint3D(0,radius,radius));
    dirty = false;
  }
}

void HARDIRenderer::draw(IPoint3D &point)
{
  if (pCenter.x != point.x || pCenter.y != point.y || pCenter.z != point.z)
    dirty = true;
  pCenter = point;
  if (hardiTool.vODF.size())
  {
    if (dirty)
      updateGlyphs(false);
    if (drawXY) odfInstancesXY.draw();
    if (drawXZ) odfInstancesXZ.draw();
    if (drawYZ) odfInstancesYZ.draw();
  }
}

void HARDIRenderer::drawMasked(IPoint3D &point, Vol3D<uint8> &vMask)
{
  if (pCenter.x != point.x || pCenter.y != point.y || pCenter.z != point.z)
    dirty = true;
  pCenter = point;
  if (hardiTool.vODF.size())
  {
    if (dirty)
      updateGlyphs(vMask,false);
    if (drawXY) odfInstancesXY.draw();
    if (drawXZ) odfInstancesXZ.draw();
    if (drawYZ) odfInstancesYZ.draw();
  }
}

float HARDIRenderer::setScale(float f)
{
  hardiTool.odfFactory.scale = f;
  odfInstancesXY.scale = f;
  odfInstancesXZ.scale = f;
  odfInstancesYZ.scale = f;
  return f;
}

