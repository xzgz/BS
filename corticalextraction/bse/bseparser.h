// Copyright (C) 2016 The Regents of the University of California
//
// Created by David W. Shattuck, Ph.D.
//
// This file is part of BSE.
//
// BSE is free software; you can redistribute it and/or
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

#ifndef CorticalExtraction_BSEParser_H
#define CorticalExtraction_BSEParser_H

#include <argparser.h>

class BSETool;

class BSEParser : public ArgParserBase {
public:
  BSEParser(BSETool &bseTool);
  virtual bool validate()
  {
    if (mfname.empty() && ofname.empty() && hiresMask.empty() && cortexFilename.empty() && adfFilename.empty() && edgeFilename.empty())
    {
      std::cerr<<"error: no output files specified."<<std::endl;
      errcode=2;
      return false;
    }
    return true;
  }
  std::string ifname;
  std::string ofname;
  std::string mfname;
  std::string adfFilename;
  std::string edgeFilename;
  std::string cortexFilename;
  std::string hiresMask;
  std::string noneckFilename;
};

#endif
