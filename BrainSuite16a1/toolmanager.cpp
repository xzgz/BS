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

#include <toolmanager.h>
#include <connectivityviewdialog.h>
#include <skullstrippingdialog.h>
#include <nonuniformitycorrectiondialog.h>
#include <tissueclassificationdialog.h>
#include <cerebrumextractiondialog.h>
#include <cortexidentificationdialog.h>
#include <scrubmaskdialog.h>
#include <topologicalcorrectiondialog.h>
#include <dewispdialog.h>
#include <innercorticalsurfacedialog.h>
#include <pialsurfacedialog.h>
#include <splithemispheresdialog.h>
#include <corticalextractionsequencedialog.h>
#include <svregistrationdialog.h>
#include <dsglviewdialog.h>
#include <diffusiontoolboxform.h>
#include <imagedisplaypropertiesform.h>
#include <skullandscalpdialog.h>
#include <corticalextractionsettings.h>

ToolManager::ToolManager() :
  surfaceDisplayForm(0),
  delineationToolboxForm(0),
  imageDisplayPropertiesForm(0),
  diffusionToolboxForm(0),
  curveToolboxForm(0)
{
}

ToolManager::~ToolManager()
{
// ToolManager is owned by the BrainSuiteWindow object
// Forms are not deleted because they are owned by the dockwidgets in the BrainSuiteWindow
// Other objects are autodeleted by std::unique_ptr
}

bool ToolManager::autosaveValid()
{
  return (corticalExtractionSequenceDialog) ? corticalExtractionSequenceDialog->autosaveValid() : false;
}

template <class Dialog>
void updateDialogFromCES(std::unique_ptr<Dialog> &dlg)
{
  if (dlg)
    dlg->updateDialogFromCES();
}

void ToolManager::updateFromCES()
{
  updateDialogFromCES(skullStrippingDialog);
  updateDialogFromCES(skullfinderDialog);
  updateDialogFromCES(nonuniformityCorrectionDialog);
  updateDialogFromCES(tissueClassificationDialog);
  updateDialogFromCES(cerebrumExtractionDialog);
  updateDialogFromCES(cortexIdentificationDialog);
  updateDialogFromCES(scrubMaskDialog);
  updateDialogFromCES(topologicalCorrectionDialog);
  updateDialogFromCES(dewispDialog);
  updateDialogFromCES(innerCorticalSurfaceDialog);
  updateDialogFromCES(pialSurfaceDialog);
  updateDialogFromCES(svRegistrationDialog);
}
