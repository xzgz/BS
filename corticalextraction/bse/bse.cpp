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

#include <vol3dlib.h>
#include <vol3dsimple.h>
#include <DS/timer.h>
#include <volumeloader.h>
#include "bseparser.h"
#include <BSE/bsetool.h>

BSEParser::BSEParser(BSETool &bseTool) : ArgParserBase("bse")
{
  description = "brain surface extractor (BSE)\n"
                "This program performs automated skull and scalp removal on T1-weighted MRI volumes.\n"
                "BSE is part of the BrainSuite collection of tools.\n"
                "For more information, please see: http://brainsuite.org";
  copyright = "Copyright (C) 2015 The Regents of the University of California and the University of Southern California";
  usageDisplayWidth=30;
  bind("i",ifname,"<input filename>","input MRI volume",true);
  bind("o",ofname,"<output filename>","output brain-masked MRI volume",false);
  bind("d",bseTool.settings.diffusionConstant,"<float>","diffusion constant",false);
  bind("n",bseTool.settings.diffusionIterations,"<iterations>","diffusion iterations",false);
  bind("s",bseTool.settings.edgeConstant,"<edge sigma>","edge detection constant",false);
  bind("r",bseTool.settings.erosionSize,"<size>","radius of erosion/dilation filter",false);
  bindFlag("p",bseTool.settings.dilateFinalMask,"dilate final mask");
  bindFlag("-trim",bseTool.settings.removeBrainstem,"trim brainstem");
  bind("-mask",mfname,"<filename>","save smooth brain mask",false);
  bind("-adf",adfFilename,"<filename>","diffusion filter output",false);
  bind("-edge",edgeFilename,"<filename>","edge map output",false);
  bind("-hires",hiresMask,"<filename>","save detailed brain mask",false);
  bind("-cortex",cortexFilename,"<filename>","cortex file",false);
  bind("v",bseTool.settings.verbosity,"<number>","verbosity level (0=silent)",false);
  bind("-neckfile",noneckFilename,"<filename>","save image after neck removal",false,true);
  bindFlag("-norotate",Vol3DBase::noRotate,"retain original orientation (default behavior will auto-rotate input NII files to LPI orientation");
  example = progname + " -i input_mri.img -o skull_stripped_mri.img";
}

bool writeByte(std::string ofname, Vol3D<VBit> &vBit)
{
  Vol3D<uint8> vMask;
  vBit.decode(vMask);
  return vMask.write(ofname);
}

void status(std::string s) { std::cout<<s<<std::endl; }

int main(int argc, char *argv[])
{
  Timer t;
  t.start();
  bool timer=false;
  BSETool bseTool;
  BSEParser ap(bseTool);
  ap.bindFlag("-timer",timer,"show timing",false);
  if (!ap.parseAndValidate(argc,argv)) { return ap.usage(); }

  Vol3DBase *vIn = VolumeLoader::load(ap.ifname);
  Vol3DBase *referenceVolume=0;
  if (!vIn)	return CommonErrors::cantRead(ap.ifname);
  int retcode = 0;
  Vol3D<uint8> maskVolume;
  {
    Vol3D<uint8> edgeMap;
    {
      if (!bseTool.stepForward(maskVolume,referenceVolume,vIn)) return 1; // adf
      if (ap.adfFilename.empty()==false)
      {
        if (referenceVolume->write(ap.adfFilename))
        {
          if (bseTool.settings.verbosity>0) std::cout<<"Wrote anisotropic diffusion filtered volume "<<ap.adfFilename<<std::endl;
        }
        else
        {
          retcode |= ::CommonErrors::cantWrite(ap.adfFilename);
        }
      }
      if (!bseTool.stepForward(maskVolume,referenceVolume,vIn)) return 2; // edge
      if (ap.edgeFilename.empty()==false)
      {
        bseTool.edgemask.decode(edgeMap);
        if (writeByte(ap.edgeFilename,bseTool.edgemask))
        {
          if (bseTool.settings.verbosity>0) std::cout<<"Wrote edge mask "<<ap.edgeFilename<<std::endl;
        }
        else
        {
          retcode |= ::CommonErrors::cantWrite(ap.edgeFilename);
        }
      }
      bseTool.stepForward(maskVolume,referenceVolume,vIn); // erode
      bseTool.stepForward(maskVolume,referenceVolume,vIn);
      if (ap.hiresMask.empty()==false)
      {
        Vol3D<uint8> vHiRes;
        bseTool.edgemask.decode(vHiRes);
        vHiRes.maskWith(maskVolume);
        if (vHiRes.write(ap.hiresMask))
        {
          if (bseTool.settings.verbosity>0) std::cout<<"Wrote detailed brain mask "<<ap.hiresMask<<std::endl;
        }
        else
        {
          retcode |= ::CommonErrors::cantWrite(ap.edgeFilename);
        }
      }
      if (ap.cortexFilename.empty()==false)
      {
        if (writeByte(ap.cortexFilename,bseTool.vCortex))
        {
          if (bseTool.settings.verbosity>0) std::cout<<"Wrote cortex mask "<<ap.cortexFilename<<std::endl;
        }
        else
        {
          retcode |= ::CommonErrors::cantWrite(ap.cortexFilename);
        }
      }
    }
  }
  std::ostringstream description;
  description<<"bse -n "<<bseTool.settings.diffusionIterations
            <<" -d "<<bseTool.settings.diffusionConstant<<" -s "<<bseTool.settings.edgeConstant<<" -r "<<bseTool.settings.erosionSize<<" -i "<<ap.ifname;
  if (bseTool.settings.removeBrainstem)	description<<" --trim";
  if (bseTool.settings.dilateFinalMask)	description<<" -p";
  if (ap.mfname.empty()==false)
  {
    maskVolume.description = description.str();
    if (maskVolume.write(ap.mfname))
    {
      if (bseTool.settings.verbosity>0) std::cout<<"Wrote mask file "<<ap.mfname<<std::endl;
    }
    else
      retcode |= ::CommonErrors::cantWrite(ap.mfname);
  }
  if (ap.ofname.empty()==false)
  {
    vIn->description = description.str();
    vIn->maskWith(maskVolume);
    if (vIn->write(ap.ofname))
    {
      if (bseTool.settings.verbosity>0) std::cout<<"Wrote skull-stripped MRI volume "<<ap.ofname<<std::endl;
    }
    else
    {
      retcode |= ::CommonErrors::cantWrite(ap.mfname);
    }
  }
  t.stop();
  if ((bseTool.settings.verbosity>1)||(timer))
  {
    std::cout<<"BSE took "<<t.elapsed()<<std::endl;
  }
  delete vIn; vIn=0;
  delete referenceVolume; referenceVolume=0;
  return retcode;
}
