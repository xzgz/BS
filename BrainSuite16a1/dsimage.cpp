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

#include "dsimage.h"

DSImage::DSImage(bool alpha) : rx(1),ry(1), scale(1), alpha(alpha)
{
}

void DSImage::blank()
{
  image.fill(0);
}

void DSImage::setAlpha(int alpha)
{
  unsigned char *bits = image.bits();
  const int np = image.width()*image.height();
  for (int i=0;i<np;i++,bits+=4)
  {
    bits[3] = alpha;
  }
}


void DSImage::resize(const int cx, const int cy)
{
  if (image.width()!=cx||image.height()!=cy)
  {
    image= QImage(cx,cy,alpha ? QImage::Format_ARGB32 : QImage::Format_RGB32);
  }
}

void DSImage::setRes(const float rx_, const float ry_, const float scale_)
{
  rx = rx_;
  ry = ry_;
  scale = scale_;
}

