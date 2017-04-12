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

#include "lutmenusystem.h"
#include <qaction.h>
#include <qmenu.h>
#include <brainsuitewindow.h>
#include <colormap.h>
#include <qapplication.h>
#include <toolmanager.h>

ColorMap lutList; // TODO: move to class

LUTMenuSystem::LUTMenuSystem(BrainSuiteWindow *parent) : brainSuiteWindow(parent), popupMenu(parent)
{
  makeLUTMenu(&popupMenu,&brainSuiteWindow->imageLUT,"&Image LUT");
  makeLUTMenu(&popupMenu,&brainSuiteWindow->imageOverlay1LUT,"&Overlay1 LUT");
  makeLUTMenu(&popupMenu,&brainSuiteWindow->imageOverlay2LUT,"&Overlay2 LUT");
}

LUTMenuSystem::LUTMenuSystem(BrainSuiteWindow *brainSuiteWindow, uint32 *&imageLUT) : brainSuiteWindow(brainSuiteWindow), popupMenu(brainSuiteWindow)
{
  uint32 **targetLUT = &brainSuiteWindow->imageLUT;
  popupMenu.setObjectName(QString::fromUtf8("lutMenu"));
  for (ColorMap::MapType::const_iterator lut=lutList.map.begin();
       lut!=lutList.map.end(); lut++)
  {
    QAction *action = popupMenu.addAction(lut->first.c_str());
    lutMenuItems.push_back(LUTMenuItem(action,lut->second,&imageLUT));
    action->setCheckable(true);
    action->setChecked((*targetLUT) == lut->second);
  }
}

QMenu *LUTMenuSystem::makeLUTMenu(QMenu *parentMenu, uint32 **targetLUT, std::string menuName)
{
  QMenu *lutMenu = new QMenu(parentMenu);
  lutMenu->setObjectName(QString::fromUtf8("lutMenu"));
  for (ColorMap::MapType::const_iterator lut=lutList.map.begin();
       lut!=lutList.map.end(); lut++)
  {
    QAction *action = lutMenu->addAction(lut->first.c_str());
    lutMenuItems.push_back(LUTMenuItem(action,lut->second,targetLUT));
    action->setCheckable(true);
    action->setChecked((*targetLUT) == lut->second);
  }
  parentMenu->addAction(lutMenu->menuAction());
  lutMenu->setTitle(menuName.c_str());
  return lutMenu;
}

void LUTMenuSystem::doPopup(const QPoint &pos)
{
  if (brainSuiteWindow==0) return;
  for (size_t i=0;i<lutMenuItems.size();i++)
  {
    if (lutMenuItems[i].action && lutMenuItems[i].parentLUT)
      lutMenuItems[i].action->setChecked((*lutMenuItems[i].parentLUT)==lutMenuItems[i].LUT);
  }
  QAction *action=popupMenu.exec(pos);
  for (size_t i=0;i<lutMenuItems.size();i++)
  {
    if (action == lutMenuItems[i].action)
    {
      if (lutMenuItems[i].parentLUT)
      {
        *lutMenuItems[i].parentLUT = lutMenuItems[i].LUT;
        brainSuiteWindow->updateImages();
        brainSuiteWindow->updateImageDisplaySliders();
      }
      return;
    }
  }
}
