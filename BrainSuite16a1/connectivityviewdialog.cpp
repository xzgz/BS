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

#include "connectivityviewdialog.h"
#include "ui_connectivityviewdialog.h"
#include <qfiledialog.h>
#include <brainsuitewindow.h>
#include <connectivityglwidget.h>
#include <brainsuitewindow.h>
#include <waitcursor.h>
#include <connectivityrenderer.h>
#include <connectivitymap.h>
#include <QKeyEvent>
#include <brainsuitesettings.h>
#include <brainsuiteqtsettings.h>
#include <QCharRef>
#include <qchar.h>
#include <trackfilter.h>
#include <fibertrackset.h>
#include <fiberrenderer.h>
#include <computeconnectivitythreaddialog.h>

extern FiberRenderer fiberRenderer;
extern ConnectivityRenderer connectivityRenderer;

ConnectivityViewDialog::ConnectivityViewDialog(BrainSuiteWindow *parent) :
  QDialog(parent),
  ui(new Ui::ConnectivityViewDialog), brainSuiteWindow(parent), connectivityGLWidget(0), gridLayout(0)
{
  ui->setupUi(this);
  connectivityGLWidget = new ConnectivityGLWidget(parent);
  gridLayout = new QGridLayout();
  setLayout(gridLayout);
  gridLayout->addWidget(connectivityGLWidget,0,0);
  gridLayout->setContentsMargins(0,0,0,0);
  setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint);
}

ConnectivityViewDialog::~ConnectivityViewDialog()
{
  delete ui;
}

FiberTrackSet *ConnectivityViewDialog::getSelection()
{
  return connectivityGLWidget->getSubgroup();
}

FiberTrackSet *ConnectivityViewDialog::applyFilter(FiberTrackSet *tractSet)
{
  return connectivityGLWidget->applyFilter(tractSet);
}

bool ConnectivityViewDialog::computeConnectivity(bool mergeCortialROIs)
{
  connectivityRenderer.computing=true;
  ThreadDialogStatus::StatusCode returnState=ThreadDialogStatus::NotStarted;
  ComputeConnectivityThreadDialog threadDialog(brainSuiteWindow,this);
  threadDialog.mergeCortialROIs=mergeCortialROIs;
  /*int returnCode=*/threadDialog.launch();
  returnState=threadDialog.status();
  if (returnState!=ThreadDialogStatus::Succeeded)
  {
    brainSuiteWindow->brainSuiteDataManager.connectivityMap = ConnectivityMap();
    brainSuiteWindow->brainSuiteDataManager.connectivityProperties = ConnectivityProperties();
  }
  connectivityRenderer.computing=false;
  connectivityRenderer.connectivityShowAllBrain(brainSuiteWindow->brainSuiteDataManager.connectivityMap);
  brainSuiteWindow->updateSurfaceView();
  brainSuiteWindow->toolManager.connectivityViewDialog->update();
  return (returnState==ThreadDialogStatus::Succeeded);
}

inline bool inRange(const int value, const int lower, const int upper)
{
  return (value>=lower && value < upper);
}

void ConnectivityViewDialog::saveMatrix()
{
  brainSuiteWindow->saveConnectivityMatrix();
}

bool ConnectivityViewDialog::pickSubset(CorticalAreas area)
{
  bool resort=false;
  switch (area)
  {
    case AllLobes: connectivityRenderer.pickSubrange(brainSuiteWindow->brainSuiteDataManager.connectivityMap,100,600); connectivityRenderer.subsetName="all cortical areas";  resort = true; break;
    case FrontalLobe: connectivityRenderer.pickSubrange(brainSuiteWindow->brainSuiteDataManager.connectivityMap,100,200); connectivityRenderer.subsetName="frontal lobe"; resort = true; break;
    case ParietalLobe: connectivityRenderer.pickSubrange(brainSuiteWindow->brainSuiteDataManager.connectivityMap,200,300); connectivityRenderer.subsetName="parietal lobe"; resort = true; break;
    case TemporalLobe:	 connectivityRenderer.pickSubrange(brainSuiteWindow->brainSuiteDataManager.connectivityMap,300,400); connectivityRenderer.subsetName="temporal lobe"; resort = true; break;
    case OccipitalLobe: connectivityRenderer.pickSubrange(brainSuiteWindow->brainSuiteDataManager.connectivityMap,400,500); connectivityRenderer.subsetName="occipital lobe"; resort = true; break;
    case SubCortical: connectivityRenderer.pickSubrange(brainSuiteWindow->brainSuiteDataManager.connectivityMap,600,700); connectivityRenderer.subsetName="subcortical areas"; resort = true; break;
    case AllBrainAreas:
      connectivityRenderer.subsetName="all brain areas";
      connectivityRenderer.connectivitySubset.resize(0);
      for (size_t i=0;i<brainSuiteWindow->brainSuiteDataManager.connectivityMap.labelIDs.size();i++)
      {
        if (inRange(brainSuiteWindow->brainSuiteDataManager.connectivityMap.labelIDs[i],100,700)||inRange(brainSuiteWindow->brainSuiteDataManager.connectivityMap.labelIDs[i],800,1000))
        {
          connectivityRenderer.connectivitySubset.push_back((int)i);
        }
      }
      resort  = true;
      break;
    case AllFibers :
    case AllLabeledAreas:
      connectivityRenderer.connectivitySubset.resize(0);
      connectivityRenderer.subsetName="all labeled regions";
      for (size_t i=0;i<brainSuiteWindow->brainSuiteDataManager.connectivityMap.labelIDs.size();i++)
      {
        connectivityRenderer.connectivitySubset.push_back((int)i);
      }
      resort  = true; break;
  }
  return resort;
}

void ConnectivityViewDialog::keyReleaseEvent(QKeyEvent *event)
{
  if (!event) return;
  QString s = event->text();
  bool shiftKey = event->modifiers()==Qt::ShiftModifier;
  bool resort = false;
  if (event->text().length()!=0)
  {
    QString s = event->text();
    switch(::toupper(s[0].toLatin1()))
    {
      case 'A' : connectivityRenderer.drawArcs^=true; break;
      case 'B' :
      {
        connectivityRenderer.useAbbrevs^=true;
        for (int i=0;i<(int)brainSuiteWindow->brainSuiteDataManager.connectivityMap.labelIDs.size();i++)
        {
          int id=brainSuiteWindow->brainSuiteDataManager.connectivityMap.labelIDs[i];
          if ((id>=0)&&(id<(int)brainSuiteWindow->brainSuiteDataManager.labelDescriptionSet.labelDetails.size()))
          {
            brainSuiteWindow->brainSuiteDataManager.connectivityMap.nodeNames[i] = (connectivityRenderer.useAbbrevs)
               ? brainSuiteWindow->brainSuiteDataManager.labelDescriptionSet.labelDetails[id].tag
               : brainSuiteWindow->brainSuiteDataManager.labelDescriptionSet.labelDetails[id].descriptor;
          }
          else
          {
            std::ostringstream ostr;
            ostr<<"L"<<id;
            brainSuiteWindow->brainSuiteDataManager.connectivityMap.nodeNames[i] = ostr.str();
          }
        }
        break;
      }
      case 'E' : connectivityRenderer.statusMessage="";
      case 'G':  connectivityRenderer.groupMode^=true; connectivityGLWidget->regroup(); brainSuiteWindow->updateSurfaceView(); break;
      case 'Q' : break;
      case 'P' : connectivityRenderer.usePrintColors(); break;
      case 'S' : connectivityRenderer.useScreenColors(); break;
      case 'N' : BrainSuiteSettings::showConnectivityNodesUsingLabelColors ^= true; break;
      case '=' :
      case '+' : connectivityRenderer.circleRadius += 0.01f; break;
      case '-' : connectivityRenderer.circleRadius -= 0.01f; break;
      case 'T' : connectivityRenderer.textZoom += shiftKey ? 0.01f : -0.01f; break;
      case 'W' : ConnectivityRenderer::arcWidthBase += (shiftKey) ? -0.1f : 0.1f; break;
      case '1' : resort=pickSubset(AllLobes);break;
      case '2' : resort=pickSubset(FrontalLobe); break;
      case '3' : resort=pickSubset(ParietalLobe); break;
      case '4' : resort=pickSubset(TemporalLobe); break;
      case '5' : resort=pickSubset(OccipitalLobe); break;
      case '6' : resort=pickSubset(SubCortical); break;
      case '7' : resort=pickSubset(AllBrainAreas); break;
      case '8' : resort=pickSubset(AllLabeledAreas); break;
      default: break;
    }
    if (resort)
    {
      connectivityRenderer.sortOddEven(connectivityRenderer.connectivitySubset,brainSuiteWindow->brainSuiteDataManager.connectivityMap);
      connectivityRenderer.clearSelection();
      BrainSuiteSettings::showTrackSubset=false;
    }
  }
  connectivityGLWidget->update();
}
