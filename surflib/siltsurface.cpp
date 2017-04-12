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

#include <surface.h>
#include <zstream.h>
#include <vectorutil.h>
#include <dfsheader.h>

bool SILT::Surface::readDFS(std::string s)
{
  SILT::izstream ifile;
  ifile.open(s.c_str());
  if (!ifile) return false;
  DFSHeader hdr;
  ifile.read((char *)&hdr,sizeof(hdr));
  bool swapped =
      (hdr.mdoffset>0) ? (hdr.mdoffset>32767) :
                         ((hdr.pdoffset>0) ? (hdr.pdoffset>32767) : (hdr.hdrsize>32767));
  if (swapped)
  {
    hdr.swapHeader();
  }
  triangles.resize(hdr.nTriangles);
  vertices.resize(hdr.nVertices);
  ifile.read((char *)&triangles[0],triangles.size()*sizeof(Triangle));
  ifile.read((char *)&vertices[0],vertices.size()*sizeof(DSPoint));
  const size_t nv = hdr.nVertices;
  vertexNormals.resize((hdr.normals>0) ? hdr.nVertices : 0);
  vertexColor.resize((hdr.vcoffset>0) ? hdr.nVertices : 0);
  vertexUV.resize((hdr.uvoffset>0) ? hdr.nVertices : 0);
  vertexLabels.resize((hdr.labelOffset>0) ? hdr.nVertices : 0);
  vertexAttributes.resize((hdr.vertexAttributes>0) ? hdr.nVertices : 0);
  if (vertexNormals.size()==nv) ifile.read((char *)&vertexNormals[0],vertexNormals.size()*sizeof(DSPoint));
  if (vertexColor.size()==nv) ifile.read((char *)&vertexColor[0],vertexColor.size()*sizeof(DSPoint));
  if (vertexUV.size()==nv) ifile.read((char *)&vertexUV[0],vertexUV.size()*sizeof(UVPoint));
  if (vertexLabels.size()==nv) ifile.read((char *)&vertexLabels[0],vertexLabels.size()*sizeof(vertexLabels[0]));
  if (vertexAttributes.size()==nv) ifile.read((char *)&vertexAttributes[0],vertexAttributes.size()*sizeof(vertexAttributes[0]));
  if (swapped)
  {
    byteswap(triangles);
    byteswap(vertices);
    byteswap(vertexColor);
    byteswap(vertexNormals);
    byteswap(vertexUV);
  }
  computeNormals();
  computeCenter();
  return true;
}

bool SILT::Surface::read(std::string s)
{
  filename = s;
  if (StrUtil::hasExtension(s,".obj")) return readOBJ(s);
  if (StrUtil::hasExtension(s,".dfs")||StrUtil::hasExtension(s,".dfsz")||StrUtil::hasExtension(StrUtil::gzStrip(s),".dfs")) return readDFS(s);
  return false;
}

void SILT::Surface::computeCenter()
{
  const size_t nv = vertices.size();
  if (nv<=0)
    center=DSPoint(0,0,0);
  else
  {
    DSPoint p;
    const DSPoint *v = &vertices[0];
    for (size_t i=0;i<nv;i++)
      p += v[i];
    center = p/(float)nv;
  }
}

void SILT::Surface::computeNormals()
{
  const size_t nt = triangles.size();
  const size_t nv = vertices.size();
  vertexNormals.resize(vertices.size());
  std::fill(vertexNormals.begin(),vertexNormals.end(),DSPoint(0,0,0));
  const DSPoint *vv = &vertices[0];
  const Triangle *tv = &triangles[0];
  for (size_t n=0;n<nt;n++)
  {
    Triangle t  = tv[n];
    DSPoint v1    = vv[t.a];
    DSPoint v3    = vv[t.b];
    DSPoint v2    = vv[t.c];
    DSPoint tn = cross(v3-v1,v3-v2);
    vertexNormals[t.a] += tn;
    vertexNormals[t.b] += tn;
    vertexNormals[t.c] += tn;
  }
  for (size_t n=0;n<nv;n++)
  {
    const float m = vertexNormals[n].mag();
    if (m>0)
    {
      vertexNormals[n].x/=m;
      vertexNormals[n].y/=m;
      vertexNormals[n].z/=m;
    }
  }
}

bool SILT::Surface::write(std::string s)
{
  std::string ext = StrUtil::getExt(filename);
  if (StrUtil::hasExtension(s,".stl")) return writeSTL(s);
  if (StrUtil::hasExtension(s,".off")) return writeOFF(s);
  if (StrUtil::hasExtension(s,".wfo")) return writeWFO(s); // wrong extension, but a useful hack for supporting
  if (StrUtil::hasExtension(s,".obj")) return writeOBJ(s); // both MNI obj and Wavefront obj.
  return writeDFS(s);
}

template <class V>
inline bool okaytowrite(const V &v, const size_t nv)
{
  return ((v.size()==nv) && (nv!=0));
}

template <class T> bool SILT::Surface::outputDFS(T &outputStream)
{
  if (!outputStream) return false;
  DFSHeader hdr;
  hdr.nTriangles = (int)nt();
  hdr.nVertices = (int)nv();
  size_t pos = sizeof(hdr) + sizeof(Triangle)*nt() + sizeof(DSPoint)*nv();
  if (okaytowrite(vertexNormals,nv())) { hdr.normals = (int)pos; pos += sizeof(DSPoint)*nv(); }
  if (okaytowrite(vertexColor,nv()))   { hdr.vcoffset = (int)pos; pos += sizeof(DSPoint)*nv(); }
  if (okaytowrite(vertexUV,nv()))      { hdr.uvoffset = (int)pos; pos += sizeof(UVPoint)*nv(); }
  if (okaytowrite(vertexLabels,nv()))  { hdr.labelOffset = (int)pos; pos += sizeof(vertexLabels[0])*nv(); }
  if (okaytowrite(vertexAttributes,nv()))  { hdr.vertexAttributes = (int)pos; pos += sizeof(vertexAttributes[0])*nv(); }
  outputStream.write((char *)&hdr,sizeof(hdr));
  SILT::writeToStream(outputStream,triangles);
  SILT::writeToStream(outputStream,vertices);
  if (okaytowrite(vertexNormals,nv())) SILT::writeToStream(outputStream,vertexNormals);
  if (okaytowrite(vertexColor,nv())) SILT::writeToStream(outputStream,vertexColor);
  if (okaytowrite(vertexUV,nv())) SILT::writeToStream(outputStream,vertexUV);
  if (okaytowrite(vertexLabels,nv())) SILT::writeToStream(outputStream,vertexLabels);
  if (okaytowrite(vertexAttributes,nv())) SILT::writeToStream(outputStream,vertexAttributes);
  return true;
}

bool SILT::Surface::writeDFS(std::string s)
{
  if (StrUtil::hasExtension(s,".gz") || StrUtil::hasExtension(s,".dfsz"))
  {
    SILT::ozstream ofile(s);
    if (!ofile) return false;
    return outputDFS(ofile);
  }
  else
  {
    std::ofstream ofile(s.c_str(),std::ios::binary);
    if (!ofile) return false;
    return outputDFS(ofile);
  }
}

bool SILT::Surface::writeSTL(std::string s)
{
  std::ofstream outputStream(s.c_str());
  if (!outputStream) return false;
  const size_t nTriangles = nt();
  DSPoint *v = &vertices[0];
  if (vertexNormals.size()!=nv()) computeNormals();
  Triangle *t = &triangles[0];
  outputStream<<"solid "<<s<<"\n";
  for (size_t i=0;i<nTriangles;i++)
  {
    outputStream<<"facet normal ";
    outputStream<<"0 0 0\n";
    outputStream<<"  outer loop\n"
               <<"    vertex "<<v[t[i].a]<<"\n"
              <<"    vertex "<<v[t[i].b]<<"\n"
             <<"    vertex "<<v[t[i].c]<<"\n"
            <<"  endloop\n"
           <<"endfacet\n";
  }
  outputStream<<"endsolid "<<s<<'\n';
  return true;
}

void SILT::Surface::computeConnectivity()
{
  const size_t nt = triangles.size();
  const size_t nv = vertices.size();
  connectivity.resize(nv);
  for (size_t i=0;i<nv;i++)
  {
    connectivity[i].clear();
  }
  {
    connectivity.resize(nv);
    Triangle *tri = &triangles[0];
    for (size_t i=0;i<nt;i++)
    {
      const int a = tri[i].a;
      const int b = tri[i].b;
      const int c = tri[i].c;
      connectivity[a].insert(b);
      connectivity[a].insert(c);
      connectivity[b].insert(a);
      connectivity[b].insert(c);
      connectivity[c].insert(a);
      connectivity[c].insert(b);
    }
  }
}

bool SILT::Surface::writeOFF(std::string s)
{
  std::ofstream ofile(s.c_str(),std::ios::binary);
  if (!ofile) return false;
  const size_t nv = vertices.size();
  const size_t nt = triangles.size();
  const DSPoint *v = &vertices[0];
  const Triangle *t = &triangles[0];
  if (vertexColor.size()==nv)
  {
    const DSPoint *c = &vertexColor[0];
    ofile<<"COFF\n";
    ofile<<nv<<' '<<nt<<' '<<0<<'\n';
    for (size_t i=0;i<nv;i++)
      ofile<<v[i]<<' '<<c[i]<<" 1.0\n";
    for (size_t i=0;i<nt;i++)
      ofile<<"3 "<<t[i].a<<' '<<t[i].c<<' '<<t[i].b<<'\n';
  }
  else
  {
    ofile<<"OFF\n";
    ofile<<nv<<' '<<nt<<' '<<0<<'\n';
    for (size_t i=0;i<nv;i++)
      ofile<<v[i]<<'\n';
    for (size_t i=0;i<nt;i++)
      ofile<<"3 "<<t[i].a<<' '<<t[i].c<<' '<<t[i].b<<'\n';
  }
  return true;
}

bool SILT::Surface::writeWFO(std::string s) // wavefront object
{
  std::ofstream ofile(s.c_str() );
  if (!ofile) return false;
  const size_t nv = vertices.size();
  const size_t nt = triangles.size();
  const DSPoint *v = &vertices[0];
  const Triangle *t = &triangles[0];
  for (size_t i=0;i<nv;i++)
    ofile<<"v "<<v[i]<<'\n';
  if (nv==vertexNormals.size())
  {
    const DSPoint *vn = &vertexNormals[0];
    for (size_t i=0;i<nv;i++)
      ofile<<"vn "<<vn[i]<<'\n';
  }
  for (size_t i=0;i<nt;i++)
    ofile<<"f "<<t[i].a+1<<' '<<t[i].c+1<<' '<<t[i].b+1<<'\n';
  return true;
}

bool SILT::Surface::writeOBJ(std::string s)
{
  std::ofstream ofile(s.c_str(),std::ios::binary);
  if (!ofile) return false;
  ofile<<"P 0.3 0.7 0.5 100 1 "<<vertices.size()<<"\n";
  for (size_t i=0;i<vertices.size();i++)
  ofile<<' '<<vertices[i]<<"\n";
  char buf[256];
  ofile<<"\n";
  for (size_t i=0;i<vertexNormals.size();i++)
  {
    sprintf(buf," %f %f %f",vertexNormals[i].x,vertexNormals[i].y,vertexNormals[i].z);
    ofile<<buf<<"\n";
  }
  ofile<<"\n";
  ofile<<' '<<triangles.size()<<std::endl;
  if (vertexColor.size()==vertices.size())
  {
    ofile<<"2 ";
    for (size_t i=0;i<vertices.size();i++)
      ofile<<vertexColor[i]<<" 1\n";
  }
  else
  {
    ofile<<" 0 1 1 1 1\n"; // default to white vertices
  }
  ofile<<"\n";
  for (size_t i=1;i<=triangles.size();i++)
  {
    ofile<<i*3<<' ';
    if ((i%8)==0) ofile<<'\n';
    if ((i%8)==7) ofile<<' ';
  }
  ofile<<'\n';
  for (size_t i=0;i<triangles.size();i++)
  {
    ofile<<triangles[i].a<<' '<<triangles[i].b<<' '<<triangles[i].c<<"\n";
  }
  return true;
}

namespace SILT {
inline void endian_swap(DSPoint &p)
{
  SILT::endian_swap(p.x);
  SILT::endian_swap(p.y);
  SILT::endian_swap(p.z);
}
inline void endian_swap(Triangle &t)
{
  SILT::endian_swap(t.a);
  SILT::endian_swap(t.b);
  SILT::endian_swap(t.c);
}
inline void endian_swap(uint8 &/*t*/) {  }
} // end of namespace SILT

class StreamReader {
public:
  StreamReader(std::istream &istream) : istr(istream), endianflag(false) {} // i officially hate this
  template <class T> void scan(T &t)
  {
    istr.read((char *)&t,sizeof(T));
    if (endianflag) SILT::endian_swap(t);
  }
  std::istream &istr;
  bool endianflag;
};

template <class T> inline StreamReader &operator>>(StreamReader &sr, T &f)
{
  sr.scan(f);
  return sr;
}

inline void endian_swapf(float& x) { return SILT::endian_swap(*reinterpret_cast<unsigned int *>(&x)); }

bool SILT::Surface::readOBJbin(std::string s)
{
  std::ifstream ifile(s.c_str(),std::ios::binary);
  if (!ifile) return false;
  if (ifile.peek()!='p') { std::cerr<<"failed to read "<<s<<" as a surface object."<<std::endl; return false; }
  std::cout<<"reading "<<s<<std::endl;
  float a=0,b=0,c=0,d=0,e=0;
  int nv;
  int nt;
  char ch;

  ifile.read(&ch,sizeof(ch));
  if (ch!='p') return false;
  StreamReader streamReader(ifile);
  streamReader>>a>>b>>c>>d>>e>>nv;
  if ((nv>0x00FFFFFF)||(nv<0))
  {
    streamReader.endianflag = true;
    ifile.seekg(0);
    endian_swapf(a);
    endian_swapf(b);
    endian_swapf(c);
    endian_swapf(d);
    endian_swapf(e);
    endian_swap(nv);
    streamReader>>a>>b>>c>>d>>e>>nv;
  }
  vertices.resize(nv);
  vertexNormals.resize(nv);
  for (int i=0;i<nv;i++) streamReader>>vertices[i];
  for (int i=0;i<nv;i++) streamReader>>vertexNormals[i];
  streamReader>>nt;
  triangles.resize(nt);
  int colorcode = 0;
  streamReader>>colorcode;
  if (colorcode==2)
  {
    vertexColor.resize(nv);
    uint8 alpha;
    for (int i=0;i<nv;i++)
    {
      uint8 r,g,b;
      streamReader>>alpha>>b>>g>>r;
      vertexColor[i]=DSPoint(r/255.0f,g/255.0f,b/255.0f);
    }
  }
  else if (colorcode==0)
  {
    uint8 a,b,c,d;
    streamReader>>a>>b>>c>>d;
  }
  else
  {
    std::cerr<<"encountered unknown color code ("<<colorcode<<") while trying to read MNI OBJ file "<<std::endl;
    return false;
  }
  ifile.seekg(nt*sizeof(int),std::ios_base::cur);// skip NT ints
  for (int i=0;i<nt;i++) streamReader>>triangles[i].a>>triangles[i].c>>triangles[i].b;
  computeCenter();
  return true;
}

bool SILT::Surface::readOBJ(std::string s)
{
  std::ifstream ifile(s.c_str());
  if (!ifile)
  {
    std::cerr<<"error opening "<<s<<std::endl;
    return false;
  }
  {
    int c = ifile.peek();
    if (c=='p') return readOBJbin(s);
    else if (c!='P') { std::cerr<<"failed to read "<<s<<" as a surface object."<<std::endl; return false; }
  }
  const int maxlinesize = 2048;
  char code[maxlinesize];
  ifile.getline(code,maxlinesize);
  std::istringstream istr(std::string(code,maxlinesize));
  float a,b,c,d,e;
  int nv;
  int nt;
  char ch;
  istr>>ch>>a>>b>>c>>d>>e>>nv;
  if (ch!='P') return false;
  vertices.resize(nv);
  vertexNormals.resize(nv);
  for (int i=0;i<nv;i++)
  {
    ifile.getline(code,maxlinesize);
    std::istringstream istr(std::string(code,maxlinesize)); istr>>vertices[i];
  }
  ifile>>std::ws;
  for (int i=0;i<nv;i++)
  {
    ifile.getline(code,maxlinesize);
    std::istringstream istr(std::string(code,maxlinesize)); istr>>vertexNormals[i];
  }
  ifile>>std::ws;
  ifile.getline(code,maxlinesize);
  std::istringstream istr2(std::string(code,maxlinesize)); istr2>>nt;
  triangles.resize(nt);
  int colorcode = 0;
  ifile>>std::ws>>colorcode;
  if (colorcode==2)
  {
    vertexColor.resize(nv);
    for (int i=0;i<nv;i++)
    {
      ifile.getline(code,maxlinesize);
      std::istringstream istr(std::string(code,maxlinesize)); istr>>vertexColor[i];
    }
  }
  else if (colorcode==0)
  {
    ifile.getline(code,maxlinesize);
    float a,b,c,d;
    std::istringstream istr(std::string(code,maxlinesize)); istr>>a>>b>>c>>d;
  }
  else
  {
  }
  int junk=0;
  for (int i=0;i<nt;i++)
  {
    ifile>>junk>>std::ws;
    if (ifile.peek()=='#') ifile.getline(code,maxlinesize);
  }
  for (int i=0;i<nt;i++)
  {
    ifile>>triangles[i].a>>std::ws;
    if (ifile.peek()=='#') ifile.getline(code,maxlinesize);
    ifile>>triangles[i].c>>std::ws;
    if (ifile.peek()=='#') ifile.getline(code,maxlinesize);
    ifile>>triangles[i].b>>std::ws;
    if (ifile.peek()=='#') ifile.getline(code,maxlinesize);
  }
  computeCenter();
  return true;
}
