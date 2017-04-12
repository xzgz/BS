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

#include <QApplication>
#include "brainsuitewindow.h"
#include <qsettings.h>
#include <qmessagebox.h>
#include <brainsuiteqtsettings.h>
#include <brainsuitesettings.h>
#include <labeldescriptionset.h>
#include <brainsuiteqtsettings.h>
#include <vol3d.h>
#include <qdesktopservices.h>
//#include <scantextfile.h>
#include <colormap.h>

extern ColorMap lutList;

int uninstallBrainSuite()
{
  QSettings settings("BrainSuite",BrainSuiteQtSettings::brainSuiteQtKey);
  QStringList keys =  settings.allKeys();
  std::cout<<"uninstall request detected"<<std::endl;
  QMessageBox msgBox;
  msgBox.setText("Uninstall BrainSuite15?");
  msgBox.setIcon(QMessageBox::Question);
  std::ostringstream ostr;
  std::string install_directory = settings.value(BrainSuiteQtSettings::qtinstallkey.c_str()).toString().toStdString(); // install_directory
  ostr<<"This will remove all stored settings from this machine.\n\n"
     <<"Are you sure you want to do this?\n\n";
  if (install_directory.empty()==false)
  {
    ostr<<"Note: the BrainSuite install directory, currently set to "<<install_directory<<", must be removed manually\n\n";
  }
  msgBox.setInformativeText(ostr.str().c_str());
  msgBox.setStandardButtons(QMessageBox::Yes|QMessageBox::No);
  if (msgBox.exec())
  {
    for (QStringList::iterator i = keys.begin(); i != keys.end(); i++)
    {
      settings.remove(*i);
    }
  }
  return 0;
}

int main(int argc, char *argv[])
{
// load lookup tables if they are in the user's .brainsuite directory
  QString bsprefdir=QDir::homePath()+"/.brainsuite";
  QStringList files = QDir(bsprefdir).entryList(QDir::Files);
  for(int i = 0; i< files.count(); i++)
  {
    QString s=files[i];
    std::string s2=s.toStdString();
    if (StrUtil::hasExtension(s2,".lut"))
    {
      std::cout<<"loading "<<s2<<std::endl;
      lutList.loadCustomLUT(bsprefdir.toStdString()+'/'+s2);
    }
  }
  qputenv("QT_LOGGING_RULES", "qt.network.ssl.warning=false"); // Qt does not include OpenSSL by default
  QApplication application(argc, argv);
  if (argc>1 && StrUtil::eqnocase(argv[1],"--uninstall"))
  {
    return uninstallBrainSuite();
  }
  while (argc>1)
  {
    bool mod=false;
    if (StrUtil::eqnocase(argv[1],"-nc"))
    {
      BrainSuiteSettings::noConsole = true;
      mod=true;
    }
    if (!mod) break;
    argv[1]=argv[0];
    argc--; argv++;
  }
  BrainSuiteQtSettings::initialize();
  BrainSuiteWindow brainSuiteWindow(argc,argv);
  brainSuiteWindow.show();
  for (int i=1;i<argc;i++)
  {
    brainSuiteWindow.loadFile(argv[i]);
  }
  return application.exec();
}
