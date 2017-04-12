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

#include "brainsuiteqtsettings.h"
#include <buildversion.h>
#include <qsettings.h>
#include <iostream>
#include <brainsuiteinstalldialog.h>
#include <qdir.h>
#include <qstandardpaths.h>
#include <qcoreapplication.h>

#if _WIN32
#ifdef QT_X86_64
std::string BrainSuiteQtSettings::platform="win64";
#else
std::string BrainSuiteQtSettings::platform="win32";
#endif
#else
#ifdef Q_OS_MAC
std::string BrainSuiteQtSettings::platform="maci64";
#else
std::string BrainSuiteQtSettings::platform="linux";
#endif
#endif

QString BrainSuiteQtSettings::brainSuiteQtKey=PROGNAME;
#if _WIN32
bool BrainSuiteQtSettings::useNativeFileDialogs=true;
#else
bool BrainSuiteQtSettings::useNativeFileDialogs=false;
#endif
std::string BrainSuiteQtSettings::titleString(PROGNAME);
bool BrainSuiteQtSettings::startingUp=false;
std::string BrainSuiteQtSettings::programCredits;
std::string BrainSuiteQtSettings::programCreditsLibs;
std::string BrainSuiteQtSettings::website("http://brainsuite.org/");
std::string BrainSuiteQtSettings::websiteForum("http://brainsuite.org/forum/");
std::string BrainSuiteQtSettings::websiteDownload("http://brainsuite.org/download/");
std::string BrainSuiteQtSettings::qtinstalldir;
std::string BrainSuiteQtSettings::qtinstallkey="install_directory";
std::string BrainSuiteQtSettings::qtsvregkey="svreg_directory";
std::string BrainSuiteQtSettings::qtsvregdir;
std::string BrainSuiteQtSettings::currentDirectory;
std::string BrainSuiteQtSettings::qtcheckforupdateskey="enable_check_for_updates";
std::string BrainSuiteQtSettings::qtuseNativeFileDialogsKey="use_native_file_dialogs";
std::string BrainSuiteQtSettings::latestVersionServer("http://brainsuite.org/latestversions.xml");
std::string BrainSuiteQtSettings::svregVersion="";
std::string BrainSuiteQtSettings::qtMostRecentDirectoryKey="most_recent_directory";
bool BrainSuiteQtSettings::enableCheckForUpdates=true;

std::string fixPath(std::string filepath);

void BrainSuiteQtSettings::setKeyValue(std::string key, bool value)
{
  QSettings settings("BrainSuite",brainSuiteQtKey);
  settings.setValue(key.c_str(),QVariant(value));
}

void BrainSuiteQtSettings::setKeyValue(std::string key, std::string value)
{
  QSettings settings("BrainSuite",brainSuiteQtKey);
  settings.setValue(key.c_str(),value.c_str());
}

bool BrainSuiteQtSettings::setUseNativeFileDialogs(bool state)
{
  useNativeFileDialogs=state;
  setKeyValue(BrainSuiteQtSettings::qtuseNativeFileDialogsKey,useNativeFileDialogs);
  return useNativeFileDialogs;
}

void BrainSuiteQtSettings::setInstallDir(std::string installDir)
{
  qtinstalldir=installDir;
  setKeyValue(qtinstallkey,installDir);
}

bool BrainSuiteQtSettings::setEnableCheckForUpdates(const bool state)
{
  BrainSuiteQtSettings::enableCheckForUpdates=state;
  QSettings settings("BrainSuite",brainSuiteQtKey);
  settings.setValue(BrainSuiteQtSettings::qtcheckforupdateskey.c_str(),QVariant(BrainSuiteQtSettings::enableCheckForUpdates));
  return BrainSuiteQtSettings::enableCheckForUpdates;
}

void BrainSuiteQtSettings::setCurrentDirectory(std::string directoryName)
{
  if (!startingUp)
  {
    currentDirectory = directoryName;
    setKeyValue(qtMostRecentDirectoryKey,currentDirectory);
  }
}

void BrainSuiteQtSettings::initialize()
{
  QSettings settings("BrainSuite",brainSuiteQtKey);
  enableCheckForUpdates=settings.value(qtcheckforupdateskey.c_str(),false).toBool();
  useNativeFileDialogs=settings.value(qtuseNativeFileDialogsKey.c_str(),false).toBool();
  currentDirectory = settings.value(BrainSuiteQtSettings::qtMostRecentDirectoryKey.c_str()).toString().toStdString(); // install_directory
  if (currentDirectory.empty())
  {
    QString homeLocation = QStandardPaths::locate(QStandardPaths::HomeLocation, QString(), QStandardPaths::LocateDirectory);
    currentDirectory=homeLocation.toStdString();
  }
  QString install_directory;
#ifdef _WIN32
  QSettings machineSettings("HKEY_LOCAL_MACHINE\\SOFTWARE\\BrainSuite\\" PROGNAME "\\Settings", QSettings::NativeFormat);
  install_directory = machineSettings.value("InstallPath").toString();
#else
#ifdef Q_OS_MAC
  QDir dir(QCoreApplication::instance()->applicationFilePath());
  dir.cdUp();
  dir.cdUp();
  dir.cdUp();
  dir.cdUp();
  install_directory=dir.absolutePath();
#else
  QDir dir(QCoreApplication::instance()->applicationFilePath());
  dir.cdUp();
  dir.cdUp();
  install_directory=dir.absolutePath();
#endif
#endif
  if (install_directory.isEmpty())
  {
    install_directory = settings.value(BrainSuiteQtSettings::qtinstallkey.c_str()).toString(); // install_directory
  }
  if (install_directory.isEmpty())
  {
    std::cout<<"install_directory is empty"<<std::endl;
    BrainSuiteInstallDialog installDialog(0,QDir::toNativeSeparators(QDir::homePath()+QString("/" PROGNAME "/")));
    installDialog.exec();
    QString install_directory = settings.value(BrainSuiteQtSettings::qtinstallkey.c_str()).toString(); // install_directory
    BrainSuiteQtSettings::qtinstalldir = install_directory.toStdString();
  }
  else
  {
    BrainSuiteQtSettings::qtinstalldir = install_directory.toStdString();
  }
  if (BrainSuiteQtSettings::qtinstalldir.empty()==false)
  {
    QString svreg_directory;
// these first two lines are for windows only -- the Windows installer sets this registry key
#ifdef _WIN32
		QSettings machineSettings("HKEY_LOCAL_MACHINE\\SOFTWARE\\BrainSuite\\" PROGNAME "\\Settings", QSettings::NativeFormat);
		svreg_directory = machineSettings.value("SVRegPath").toString();
#endif
    if (svreg_directory.isEmpty())
      svreg_directory=settings.value(BrainSuiteQtSettings::qtsvregkey.c_str()).toString(); // install_directory
    if (svreg_directory.isEmpty())
    {
      BrainSuiteQtSettings::qtsvregdir = "";
    }
    else
    {
      BrainSuiteQtSettings::qtsvregdir = svreg_directory.toStdString();
    }
  }
  if (BrainSuiteQtSettings::qtsvregdir.empty()==false)
  {
  }
  programCredits=
      std::string("<b>")
      +BrainSuiteQtSettings::titleString
      +" (build #"
      +BuildVersion::build()
      +")</b><br/>"
      +"Copyright (C) 2016 The Regents of the University of California<br/>"
      +BrainSuiteQtSettings::titleString
      +" was written by David W. Shattuck, Ph.D., and is produced and distributed as a collaborative "
       "project between Dr. Shattuck at the <a href=http://shattuck.bmap.ucla.edu>Ahmanson-Lovelace Brain Mapping Center at the University of California, Los Angeles</a> "
       "and Richard M. Leahy, Ph.D., at the <a href=http://neuroimage.usc.edu>Biomedical Imaging Group at the University of Southern California</a>.<br/>"
       +BrainSuiteQtSettings::titleString+" is free software and is released under a GNU General Public License; Version 2. "
       "For more details, please see the enclosed license files.<br/>"
       "For information on the individual copyrights of the programs included with "+BrainSuiteQtSettings::titleString+", please see the enclosed license files.<br/>"
#if _WIN32
      "<br/>"
#endif
       "SVReg was developed by Anand A. Joshi, Ph.D., at the <a href=http://neuroimage.usc.edu>Biomedical Imaging Group at USC</a>.<br/>"
#if _WIN32
      "<br/>"
#endif
       "BDP was developed by Chitresh Bhushan, Ph.D., Divya Varadarajan, and Justin P. Haldar, Ph.D., at the <a href=http://neuroimage.usc.edu>Biomedical Imaging Group at USC</a>.<br/>"
  #if _WIN32
        "<br/>"
  #endif
       "This work has been supported in part by NIH Grants NIH-NINDS R01-NS074980 and NIH-NIBIB R01-EB002010. "
       "For more information, please visit the <a href=\""+website+"\">BrainSuite website.</a>";
  programCreditsLibs =
      "BrainSuite downloads are available at <a href=\""+websiteDownload
      +"\">brainsuite.org/downloads.</a> "
       "Support is provided at the <a href=\""+websiteForum
      +"\">BrainSuite forum.</a><br/>"
#if _WIN32
      "<br/>"
#endif
       "BrainSuite uses the Qt GUI Toolkit, v 5.6.1. "
       "<a href=http://doc.qt.io/qt-5/lgpl.html>The Qt GUI Toolkit is Copyright (C) 2016 Digia Plc and/or its subsidiary(-ies) and other contributors</a>, and it is licensed under the LGPL v2.1 license (http://www.gnu.org/licenses/lgpl-2.1.txt)<br/>"
       +BrainSuiteQtSettings::titleString+", BDP, and SVReg use additional third party libraries. For license details, please see the individual license files included with these programs."
      ;
}

