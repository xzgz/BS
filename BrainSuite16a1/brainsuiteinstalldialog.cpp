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

#include "brainsuiteinstalldialog.h"
#include "ui_brainsuiteinstalldialog.h"
#include <qdir.h>
#include <qfiledialog.h>
#include <iostream>
#include <qsettings.h>
#include <qfileinfo.h>
#include <sstream>
#include <brainsuiteqtsettings.h>
#include <brainsuitewindow.h>

BrainSuiteInstallDialog::BrainSuiteInstallDialog(BrainSuiteWindow *parent, QString installDir) :
  QDialog(parent),
  ui(new Ui::BrainSuiteInstallDialog),
  brainSuiteWindow(0),
  installDir(installDir),
  defaultInstallDir(installDir),
  continueText("Start BrainSuite")
{
  ui->setupUi(this);
  ui->installLogTextEdit->setVisible(false);
  std::string text=
    "It appears that this is the first time "+BrainSuiteQtSettings::titleString+" has been run on this account. "
    "For certain features to work properly, "+BrainSuiteQtSettings::titleString+" needs to store some files on your local file system. "
    "The typical location for this would be in a subdirectory in your home directory. "
    "If you choose not to install at this time, you will be prompted again the next time you run "+BrainSuiteQtSettings::titleString+".";
  ui->explanationInfoLabel->setText(text.c_str());
  ui->allowInternetConnectionCheckBox->setChecked(true);
  setWindowTitle((BrainSuiteQtSettings::titleString+" Installer").c_str());
  setInstallDirectory(installDir);
}

void BrainSuiteInstallDialog::setCheckInstallFlag(bool flag)
{
  ui->allowInternetConnectionCheckBox->setChecked(flag);
}

void BrainSuiteInstallDialog::setContinueText(QString s)
{
  continueText=s;
}

void BrainSuiteInstallDialog::setMainText(std::string s)
{
  ui->explanationInfoLabel->setText(s.c_str());
}

BrainSuiteInstallDialog::~BrainSuiteInstallDialog()
{
  delete ui;
}

bool BrainSuiteInstallDialog::copyFolder(std::ostream &filelog, QString sourceFolder, QString destFolder, bool recursive)
{
  destFolder = QDir::toNativeSeparators(QDir::cleanPath(destFolder))	;
  QDir sourceDir(sourceFolder);
  if(!sourceDir.exists())
  {
    std::cerr<<"file "<<sourceFolder.toStdString()<<" does not exist"<<std::endl;
    return false;
  }
  QDir destDir(destFolder);
  if(!destDir.exists())
  {
    filelog<<"made directory: "<<destFolder.toStdString()<<"\n";
    destDir.mkdir(destFolder);
  }
  QStringList files = sourceDir.entryList(QDir::Files);
  for(int i = 0; i< files.count(); i++)
  {
    QString srcName = sourceFolder + "/" + files[i];
    QString destName = QDir::toNativeSeparators(QDir::cleanPath(destFolder + "/" + files[i]));
    QFile::copy(srcName, destName);
    filelog<<"installed file: "<<destName.toStdString()<<"\n";
  }
  if (recursive)
  {
    files.clear();
    files = sourceDir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
    for(int i = 0; i< files.count(); i++)
    {
      QString srcName = sourceFolder + "/" + files[i];
      QString destName = destFolder + "/" + files[i];
      copyFolder(filelog, srcName, destName, true);
    }
  }
  return true;
}

void BrainSuiteInstallDialog::setInstallDirectory(QString s)
{
  installDir = s;
  QFileInfo info(s);
  if (info.exists())
  {
    if (info.isDir())
    {
      s = QDir::toNativeSeparators(QDir::cleanPath(s)+'/');
      ui->installDirectoryLabel->setText(s);
      ui->installDirectoryInfoLabel->setText("BrainSuite files will be installed in this directory.");
      ui->installCommandLinkButton->setEnabled(true);
    }
    else
    {
      ui->installDirectoryLabel->setText("No installation location specified");
      ui->installDirectoryInfoLabel->setText("");
      ui->installCommandLinkButton->setDisabled(true);
    }
  }
  else
  {
    ui->installDirectoryLabel->setText(s);
    ui->installDirectoryInfoLabel->setText("This directory will be created and BrainSuite files will be installed in it.");
    ui->installCommandLinkButton->setEnabled(true);
  }
}

void BrainSuiteInstallDialog::on_installCommandLinkButton_clicked()
{
  bool flag=BrainSuiteQtSettings::setEnableCheckForUpdates(ui->allowInternetConnectionCheckBox->isChecked());
  BrainSuiteQtSettings::enableCheckForUpdates=flag;
  QFileInfo info(installDir);
  if (!info.exists())
  {
    QDir qdir;
    if (!qdir.mkdir(installDir))
    {
      std::cerr<<"could not create directory"<<std::endl;
      return;
    }
  }
  info.refresh();
  {
    if (info.isDir())
    {
      std::ostringstream filelog;
      filelog<<"install log:\n";
      copyFolder(filelog,":/atlas",installDir);
      copyFolder(filelog,":/protocols",installDir);
      copyFolder(filelog,":/labeldesc",installDir);
      ui->installCommandLinkButton->setVisible(false);
      ui->selectPushButton->setVisible(false);
      ui->installLogTextEdit->setVisible(true);
      ui->resetToDefaultLocationPushButton->setVisible(false);
      ui->installLogTextEdit->setText(filelog.str().c_str());
      BrainSuiteQtSettings::setInstallDir(installDir.toStdString());
      ui->explanationInfoLabel->setText("BrainSuite has finished installing files to your system.");
      ui->installDirectoryLabel->setText("");
      ui->installDirectoryLocationLabel->setText("");
      ui->installDirectoryInfoLabel->setText("");
      ui->selectPushButton->setVisible(false);
      ui->installCommandLinkButton->setVisible(false);
      ui->proceedCommandLinkButton->setText(continueText);
    }
  }
}

void BrainSuiteInstallDialog::on_proceedCommandLinkButton_clicked()
{
  bool flag=ui->allowInternetConnectionCheckBox->isChecked();
  BrainSuiteQtSettings::setEnableCheckForUpdates(flag);
  close();
}

void BrainSuiteInstallDialog::on_selectPushButton_clicked()
{
  QString directory = QFileDialog::getExistingDirectory (this, "Select an installation directory for BrainSuite", QDir::homePath(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks );
  if (!directory.isEmpty())
  {
    setInstallDirectory(directory);
    return;
  }
}

void BrainSuiteInstallDialog::on_resetToDefaultLocationPushButton_clicked()
{
  setInstallDirectory(defaultInstallDir);
}
