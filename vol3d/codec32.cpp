// Copyright (C) 2016 The Regents of the University of California and
// the University of Southern California
//
// Created by David W. Shattuck, Ph.D.
//
// This file is part of Vol3D.
//
// Vol3D is free software; you can redistribute it and/or
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

#include <DS/codec32.h>

void Codec32::encode(uint8 *data, uint32 *code, const int cx, const int cy, const int cz)
{
	uint32 *cptr  = code;
	for (int z=0;z<cz;z++)
	{
		for (int y=0;y<cy;y++)
		{
			uint8 *dptr = data + y*cx + cx*cy*z;
      uint32 val = 0;
			int p = 0;
			for (int x=0;x<cx;x++,dptr++)
			{
        val>>=1;
        val |= (uint32)((*dptr)&(0x01))<<31;
				if (((++p) &= 0x1F)==0) { *cptr++ = val; }
			}
			if ((p)!=0) { *cptr++ = val>>(32-p); }
		}
	}
}

void Codec32::decode(uint32 *code, uint8 *data, const int cx, const int cy, const int cz)
{
	uint32 *cptr  = code;
	for (int z=0;z<cz;z++)
	{
		for (int y=0;y<cy;y++)
		{
			uint8 *dptr = data + y*cx + cx*cy*z;
      uint32 val=0;
			int p = 0;
			for (int x=0;x<cx;x++)
			{
				if (p==0) { val = *(cptr++); }
				(++p) &= 0x1F;
        *dptr++ = 0xFF * (val & 1);
        val >>=1;
			}
		}
	}
}

