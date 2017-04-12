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

#include "floatslider.h"

FloatSlider::FloatSlider(QWidget *parent) :
  QSlider(parent), rescale(false),
  minf(0),maxf(1),scale(0.1)
{
}

bool FloatSlider::setFloatRange(float min, float max, int nLevels)
{
  float delta = max-min;
  if (delta<=0) return false;
  rescale=true;
  setRange(0,nLevels);
  scale = delta/nLevels;
  minf=min;
  maxf=max;
  return true;
}

bool FloatSlider::setIntRange(int min, int max)
{
  rescale=false;
  setRange(min,max);
  return true;
}

float FloatSlider::valuef()
// returns the value used, which is constrained to the range of the slider
{
  int position=value();
  if (rescale)
  {
    return minf + scale * position;
  }
  else
    return (float)position;
}

float FloatSlider::setFloatValue(float f)
{
  if (rescale&&scale!=0)
  {
    setValue((int)((f-minf)/scale));
  }
  else
    setValue((int)f);
  return f;
}
