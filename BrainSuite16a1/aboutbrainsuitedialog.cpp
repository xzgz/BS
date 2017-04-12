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

#include "aboutbrainsuitedialog.h"
#include "ui_aboutbrainsuitedialog.h"
#include <brainsuiteqtsettings.h>
#include <qdesktopservices.h>
#include <qurl.h>
#include <qfile.h>
#include <QNetworkRequest>
#include <iostream>
#include <buildversion.h>
#include <sstream>
#include <latestversionmanifest.h>
#include <svregguitool.h>

AboutBrainSuiteDialog::AboutBrainSuiteDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::AboutBrainSuiteDialog),
  downloader(0)
{
  ui->setupUi(this);
  setWindowTitle((std::string("About ") + BrainSuiteQtSettings::titleString).c_str());
  ui->brainSuiteCredit->setText(BrainSuiteQtSettings::programCredits.c_str());
  ui->brainSuiteCredit->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
  ui->brainSuiteCredit->setOpenExternalLinks(true);
  ui->brainSuiteCredit->setText(BrainSuiteQtSettings::programCredits.c_str());
  ui->brainSuiteCreditLibs->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
  ui->brainSuiteCreditLibs->setOpenExternalLinks(true);
  ui->brainSuiteCreditLibs->setText(BrainSuiteQtSettings::programCreditsLibs.c_str());
  if (BrainSuiteQtSettings::enableCheckForUpdates)
  {
    QUrl versionUrl(BrainSuiteQtSettings::latestVersionServer.c_str());
    downloader = new FileDownloader(versionUrl, this);
    connect(downloader, SIGNAL(downloaded()), SLOT(readVersionFromWeb()));
    ui->newVersionAvailable->setText("checking for updates...");
  }
  else
    ui->newVersionAvailable->setText("Checking for updates is disabled. To enable it, go to the Help menu.\nClick here to check for updates now.");
}

AboutBrainSuiteDialog::~AboutBrainSuiteDialog()
{
  delete ui;
  delete downloader;
}

void AboutBrainSuiteDialog::on_visitBrainSuiteButton_clicked()
{
  QDesktopServices::openUrl(QUrl(BrainSuiteQtSettings::website.c_str()));
}
bool updatesAvailable(std::ostream &ostr, const LatestVersionManifest&lvm);

extern SVRegGUITool svregGUITool;
void AboutBrainSuiteDialog::readVersionFromWeb()
{
  if (downloader->error())
  {
    ui->newVersionAvailable->setText("Unable to establish connection with BrainSuite version server. Click here to try again.");
    return;
  }
  LatestVersionManifest lvm;
  lvm.parse(downloader->downloadedData().data());
  std::ostringstream ostr;
  if (updatesAvailable(ostr,lvm))
  {
    ostr<<"Please visit the BrainSuite website to download the updated software.";
    ui->newVersionAvailable->setText(ostr.str().c_str());
  }
  else
    ui->newVersionAvailable->setText("BrainSuite is up to date.");
  downloader->deleteLater();
  downloader=0;
}

void AboutBrainSuiteDialog::on_newVersionAvailable_clicked()
{
  if (!downloader)
  {
    QUrl versionURL(BrainSuiteQtSettings::latestVersionServer.c_str());
    downloader = new FileDownloader(versionURL, this);
    connect(downloader, SIGNAL(downloaded()), SLOT(readVersionFromWeb()));
    ui->newVersionAvailable->setText("checking for updates...");
  }
}
