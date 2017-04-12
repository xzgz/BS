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

#include <silttypes.h>
#include <qgl.h>
#include "brainsuitewindow.h"
#include "ui_brainsuitewindow.h"
#include <vol3d_t.h>
#include <fibertrackset.h>
#include <brainsuiteqtsettings.h>

Vol3DInstance(DSPoint)

float computeGlyphScaleFactor(Vol3DBase *vol);

bool BrainSuiteWindow::saveODFEig(std::string ofname)
{
  return brainSuiteDataManager.vODFEig.write(ofname);
}

bool BrainSuiteWindow::loadODFEig(std::string ifname)
{
  return brainSuiteDataManager.vODFEig.read(ifname);
}

void BrainSuiteWindow::on_actionODF_Eig_triggered()
{
  QString filename = dsGetSaveFileNameQS(tr("Save ODF Volume"),tr("3D Image Volume Files (*.eig.img *.eig.nii *.eig.img.gz *.eig.nii.gz);;3D Image Volume Files (*.img *.nii *.img.gz *.nii.gz)"),"");
  if (filename.isEmpty()) return;
  if (!saveODFEig(filename.toStdString()))
    std::cerr<<"unable to save "<<filename.toStdString()<<std::endl;
}

void BrainSuiteWindow::on_actionLoad_ODF_Maxima_triggered()
{
  QString filename = QFileDialog::getOpenFileName(this,tr("Save ODF Volume"),BrainSuiteQtSettings::getCurrentDirectory().c_str(),
                                                  tr("3D Image Volume Files (*.eig.img *.eig.nii *.eig.img.gz *.eig.nii.gz);;3D Image Volume Files (*.img *.nii *.img.gz *.nii.gz)"),0,
                                                  fileDialogOptions);
  if (filename.isEmpty()) return;// false;
  if (!loadODFEig(filename.toStdString()))
    std::cerr<<"unable to save "<<filename.toStdString()<<std::endl;
}

void BrainSuiteWindow::on_actionSave_Full_Set_triggered()
{
  if (!brainSuiteDataManager.fiberTrackSet)
  {
    return;
  }
  QString filename = dsGetSaveFileNameQS(tr("Save Fibertrack Set"),tr("BrainSuite Track File (*dft)"),"");
  if (filename.isEmpty()) return;
  if (!brainSuiteDataManager.fiberTrackSet->write(filename.toStdString()))
  {
    std::cerr<<"unable to save "<<filename.toStdString()<<std::endl;
  }
}

void BrainSuiteWindow::on_actionSave_Subset_triggered()
{
  if (!brainSuiteDataManager.fiberTrackSubset) return;
  QString filename = dsGetSaveFileNameQS(tr("Save Fibertrack Set"),tr("BrainSuite Track File (*dft)"),"");
  if (filename.isEmpty()) return;
  if (!brainSuiteDataManager.fiberTrackSubset->write(filename.toStdString()))
  {
    std::cerr<<"unable to save "<<filename.toStdString()<<std::endl;
  }
}

