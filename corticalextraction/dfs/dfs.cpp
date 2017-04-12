// Copyright (C) 2016 The Regents of the University of California and
// the University of Southern California
//
// Created by David W. Shattuck, Ph.D.
//
// This file is part of DFS.
//
// DFS is free software; you can redistribute it and/or
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

#include <marchingcubes.h>
#include <volumeloader.h>
#include <vol3dcrop.h>
#include <argparser.h>
#include <surflib.h>
#include <vol3dlib.h>
#include <DS/timer.h>

bool colorSurface(SILT::Surface &dfs, const char *ifname, bool zp, const float frac=0.99f);

class DFSParser : public ArgParserBase { 
public:
  DFSParser() : ArgParserBase("dfs"),
    smoothIterations(10), smoothAlpha(0.5f), curvatureWeighting(0.0f),
    noNormals(false), postsmooth(false), zeropad(false),
    colorFrac(0.999f),
    op(NonZero),
    upper(256), lower(254), match(255),
    verbosity(1), timer(false)
  {
    copyright = "Copyright (C) 2015 The Regents of the University of California and the University of Southern California";
    description="Surface Generator\nGenerates mesh surfaces using an isosurface algorithm.\n"
                "DFS is part of the BrainSuite collection of tools.\n"
                "For more information, please see: http://brainsuite.org";
    bind("i",ifname,"<input volume>","input 3D volume",true);
    bind("o",ofname,"<output surface>","output surface mesh file",true);
    bind("c",cfname,"<image volume>","shade surface model with data from image volume",false);
    bind("n",smoothIterations,"<value>","number of smoothing iterations",false);
    bind("a",smoothAlpha,"<value>","smoothing constant",false);
    bind("w",curvatureWeighting,"<value>","curvature weighting",false);
    bind("f",colorFrac,"<value>","scaling percentile",false,false);
    bindEnum("nz",op,NonZero,"tessellate non-zero voxels",false);
    bindArgState("gt",op,GreaterThan,upper,"<threshold>","tessellate voxels greater than <threshold>",false);
    bindArgState("lt",op,LessThan,lower,"<threshold>","tessellate voxels less than <threshold>",false);
    bindArgState("eq",op,Equals,match,"<value>","tessellate voxels equal to <value>",false);
    bindEnum("z",zeropad,true,"zero-pad volume (avoids clipping at edges)",false);
    bindEnum("-nonormals",noNormals,true,"do not compute vertex normals",false);
    bindEnum("-postsmooth",postsmooth,true,"smooth vertices after coloring",false);
    bind("v",verbosity,"<level>","verbosity (0 = quiet)",false,false);
    bindFlag("-timer",timer,"timing function",false);

    notes = "if the -c option is specified, a set of vertex colors is generated within the file.\n\n";
  }
  enum Operators { NonZero, Equals, GreaterThan, LessThan };
  std::string ifname; // -i
  std::string ofname; // -o
  std::string cfname; // -c
  int smoothIterations; // -n
  float smoothAlpha; // -a
  float curvatureWeighting; // -w
  bool noNormals;  // --nonormals
  bool postsmooth; // --postsmooth
  bool zeropad; // -z
  float colorFrac; // -f
  // modes
  Operators op;
  double upper;
  double lower;
  double match;
  int verbosity;
  bool timer;
};

template <class T>
bool PF(SILT::Surface &dfs, Vol3D<T> &vol, DFSParser &ap)
{
  MarchingCubes marchingCubes;
  if (ap.zeropad)
  {
    Vol3D<T> vol2;
    zeropad(vol2,vol,2,2,2);
    switch (ap.op)
    {
      case DFSParser::NonZero:     marchingCubes.cubes(vol2,dfs,NZ<T>()); break;
      case DFSParser::GreaterThan: marchingCubes.cubes(vol2,dfs,GT<T>((T)ap.upper)); break;
      case DFSParser::Equals:      marchingCubes.cubes(vol2,dfs,EQ<T>((T)ap.match)); break;
      case DFSParser::LessThan:    marchingCubes.cubes(vol2,dfs,LT<T>((T)ap.lower)); break;
    }
  }
  else
  {
    switch (ap.op)
    {
      case DFSParser::NonZero:     marchingCubes.cubes(vol,dfs,NZ<T>()); break;
      case DFSParser::GreaterThan: marchingCubes.cubes(vol,dfs,GT<T>((T)ap.upper)); break;
      case DFSParser::Equals:      marchingCubes.cubes(vol,dfs,EQ<T>((T)ap.match)); break;
      case DFSParser::LessThan:    marchingCubes.cubes(vol,dfs,LT<T>((T)ap.lower)); break;
    }
  }
  if (StrUtil::hasExtension(ap.ofname,".obj"))
  {
    if (ap.zeropad)
    {
      DSPoint shift(-2,-2,-2);
      const size_t nv = dfs.vertices.size();
      DSPoint *pts = &dfs.vertices[0];
      for (size_t i=0;i<nv;i++) pts[i] += shift;
    }
    dfs.computeNormals();
    if (dfs.writeOBJ(ap.ofname))
    {
      if (ap.verbosity>0) std::cout<<"wrote surface file "<<ap.ofname<<std::endl;
    }
    else
    {
      CommonErrors::cantWrite(ap.ofname);
      return false;
    }
  }
  else
  {
    if (!ap.postsmooth)
    {
      if (ap.curvatureWeighting==0)
        SurfLib::laplaceSmooth(dfs,ap.smoothIterations,ap.smoothAlpha);
      else
        SurfLib::laplaceSmoothCurvatureWeighted(dfs,ap.smoothIterations,ap.smoothAlpha,ap.curvatureWeighting);
    }
    if (ap.cfname.empty()==false)
    {
      colorSurface(dfs,ap.cfname.c_str(),ap.zeropad,ap.colorFrac);
    }
    if (ap.zeropad)
    {
      DSPoint shift(-2,-2,-2);
      const size_t nv = dfs.vertices.size();
      DSPoint *pts = &dfs.vertices[0];
      for (size_t i=0;i<nv;i++) pts[i] += shift;
    }
    if (ap.postsmooth)
    {
      if (ap.curvatureWeighting==0)
        SurfLib::laplaceSmooth(dfs,ap.smoothIterations,ap.smoothAlpha);
      else
        SurfLib::laplaceSmoothCurvatureWeighted(dfs,ap.smoothIterations,ap.smoothAlpha,ap.curvatureWeighting);
    }
    if (!ap.noNormals) dfs.computeNormals();
    if (dfs.write(ap.ofname))
    {
      if (ap.verbosity>0) std::cout<<"wrote surface file "<<ap.ofname<<std::endl;
    }
    else
    {
      CommonErrors::cantWrite(ap.ofname);
      return false;
    }
  }
  return true;
}

int main(int argc, char *argv[])
{
  Timer timer;timer.start();
  DFSParser parser;
  if (!parser.parseAndValidate(argc,argv)) return parser.usage();
  Vol3DBase *volume = VolumeLoader::load(parser.ifname);
  if (!volume) { std::cerr<<"Error: unable to load "<<parser.ifname<<std::endl; return 1; }
  SILT::Surface dfs;
  bool flag = false;
  switch (volume->typeID())
  {
    case SILT::Sint8 : flag = PF(dfs,*(Vol3D<sint8>  *)volume,parser); break;
    case SILT::Uint8 : flag = PF(dfs,*(Vol3D<uint8>  *)volume,parser); break;
    case SILT::Sint16: flag = PF(dfs,*(Vol3D<sint16> *)volume,parser); break;
    case SILT::Uint16: flag = PF(dfs,*(Vol3D<uint16> *)volume,parser); break;
    case SILT::Sint32: flag = PF(dfs,*(Vol3D<sint32> *)volume,parser); break;
    case SILT::Uint32: flag = PF(dfs,*(Vol3D<uint32> *)volume,parser); break;
    case SILT::Float32: flag = PF(dfs,*(Vol3D<float> *)volume,parser); break;
    case SILT::Float64: flag = PF(dfs,*(Vol3D<double> *)volume,parser); break;
    default:
      std::cerr<<"Error: "<<parser.ifname<<" is of an unsupported datatype ("<<volume->datatypeName()<<")."<<std::endl;
      flag = false;
  }
  delete volume;
  if (parser.timer)
  {
    timer.stop();
    std::cout<<"dfs took "<<timer.elapsed()<<std::endl;
  }
  return (flag) ? 0 : 1;
}
