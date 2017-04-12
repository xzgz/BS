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

#include "welcomesplashpage.h"
#include "ui_welcomesplashpage.h"
#include <brainsuitewindow.h>
#include <qdesktopservices.h>
#include <brainsuiteqtsettings.h>
#include <qurl.h>
#include <qsettings.h>
#include <qfileinfo.h>
#include <dsfilepathbutton.h>
#include <protocolcurveset.h>
#include <qdir.h>
#include <filedownloader.h>
#include <buildversion.h>
#include <latestversionmanifest.h>

#include <svregguitool.h>
extern SVRegGUITool svregGUITool;

bool updatesAvailable(std::ostream &ostr, const LatestVersionManifest&lvm)
{
  bool updates=false;
  if (BuildVersion::buildNumber()<lvm.brainSuiteVersion.build)
  {
    updates=true;
    ostr<<"A newer version of BrainSuite (v."<<lvm.brainSuiteVersion.version
        <<" released "<<lvm.brainSuiteVersion.releaseDate.toString("d MMMM yyyy").toStdString()<<") is available.\n";
  }
#ifndef QT_X86_32 // there is no svreg for Win32 versions of BrainSuite
  float svregVersion = 0;
  std::istringstream istr(lvm.svregVersion.version);
  istr>>svregVersion;
  if (lvm.svregVersion.build > svregGUITool.svreg.svregManifest.buildNumber)
  {
    updates=true;
    ostr<<"A newer version of SVReg (v."<<lvm.svregVersion.version
        <<" released "<<lvm.svregVersion.releaseDate.toString("d MMMM yyyy").toStdString()<<") is available.\n";
  }
#endif
  if (!updates) ostr<<"BrainSuite is up to date.";
  return updates;
}

WelcomeSplashPage::WelcomeSplashPage(QWidget *parent) :
  QWidget(parent), ui(new Ui::WelcomeSplashPage), brainSuiteWindow(0), downloader(0)
{
  ui->setupUi(this);
  ui->brainsuiteTitleLabel->setText(BrainSuiteQtSettings::titleString.c_str());
  if (BrainSuiteQtSettings::enableCheckForUpdates)
  {
    QUrl versionUrl(BrainSuiteQtSettings::latestVersionServer.c_str());
    downloader = new FileDownloader(versionUrl, this);
    connect(downloader, SIGNAL(downloaded()), SLOT(readVersionFromWeb()));
    ui->newVersionLabel->setText("checking for updates...");
  }
  else
  {
    ui->newVersionLabel->setText("Checking for updates is disabled. To enable it, go to the Help menu.\nClick here to check for updates now.");
  }
  fileButtons.push_back(ui->recentFilePushButton);
  fileButtons.push_back(ui->recentFilePushButton2);
  fileButtons.push_back(ui->recentFilePushButton3);
  fileButtons.push_back(ui->recentFilePushButton4);
  fileButtons.push_back(ui->recentFilePushButton5);
  fileButtons.push_back(ui->recentFilePushButton6);
  fileButtons.push_back(ui->recentFilePushButton7);
  fileButtons.push_back(ui->recentFilePushButton8);
  defaultSize=rect();
  updateRecentlyUsedFiles();
}

void WelcomeSplashPage::attachAndInitialize(BrainSuiteWindow *bsw)
{
  brainSuiteWindow=bsw;
}

WelcomeSplashPage::~WelcomeSplashPage()
{
  delete ui;
  delete downloader;
}

void WelcomeSplashPage::updateRecentlyUsedFiles()
{
  QSettings settings("BrainSuite",BrainSuiteQtSettings::brainSuiteQtKey);
  QStringList files = settings.value("recentFileList").toStringList();
  int numRecentFiles = std::min(files.size(),(int)fileButtons.size());
  ui->clearListPushButton->setVisible(numRecentFiles>0);
  ui->recentFilesLabel->setVisible(numRecentFiles>0);
  filenames.resize(numRecentFiles);
  for (int i = 0; i < numRecentFiles; i++)
  {
    QString filename = QFileInfo(files[i]).fileName();
    QString text = filename;
    QString path=QFileInfo(files[i]).path();
    fileButtons[i]->setText(text);
    fileButtons[i]->filepath=QDir::toNativeSeparators(path);
    fileButtons[i]->setToolTip(files[i]);
    fileButtons[i]->setVisible(true);
    filenames[i] = files[i].toStdString();
  }
  for (size_t i = numRecentFiles; i < fileButtons.size(); ++i)
    fileButtons[i]->setVisible(false);
}

void WelcomeSplashPage::on_visitBrainSuiteWebsiteCommandLinkButton_clicked()
{
  QDesktopServices::openUrl(QUrl(BrainSuiteQtSettings::website.c_str()));
}

void WelcomeSplashPage::on_visitBrainSuiteUserForumCommandLinkButton_clicked()
{
  QDesktopServices::openUrl(QUrl(BrainSuiteQtSettings::websiteForum.c_str()));
}

void WelcomeSplashPage::on_openTractographyCommandLinkButton_clicked()
{
  if (brainSuiteWindow)
    brainSuiteWindow->loadFiberTrackSet();
}

void WelcomeSplashPage::on_openBSTCommandLinkButton_clicked()
{
  if (brainSuiteWindow)
    brainSuiteWindow->loadBSTFile();
}

void WelcomeSplashPage::on_openImageCommandLinkButton_clicked()
{
  if (brainSuiteWindow)
    brainSuiteWindow->loadVolume();
}

void WelcomeSplashPage::on_openSurfaceCommandLinkButton_clicked()
{
  if (brainSuiteWindow)
    brainSuiteWindow->loadSurface();
}

void WelcomeSplashPage::loadRecentFile(size_t index)
{
  if (filenames.size()>index)
  {
    if (!brainSuiteWindow->loadFile(filenames[index]))
    {
      std::cerr<<"unable to load "<<filenames[index]<<std::endl;
    }
  }
}

void WelcomeSplashPage::on_recentFilePushButton_clicked()
{
  loadRecentFile(0);
}

void WelcomeSplashPage::on_recentFilePushButton2_clicked()
{
  loadRecentFile(1);
}

void WelcomeSplashPage::on_recentFilePushButton3_clicked()
{
  loadRecentFile(2);
}

void WelcomeSplashPage::on_recentFilePushButton4_clicked()
{
  loadRecentFile(3);
}

void WelcomeSplashPage::on_recentFilePushButton5_clicked()
{
  loadRecentFile(4);
}

void WelcomeSplashPage::on_recentFilePushButton6_clicked()
{
  loadRecentFile(5);
}

void WelcomeSplashPage::on_recentFilePushButton7_clicked()
{
  loadRecentFile(6);
}

void WelcomeSplashPage::on_recentFilePushButton8_clicked()
{
  loadRecentFile(7);
}

void WelcomeSplashPage::on_openProtocolCurveLinkButton_clicked()
{
  if (brainSuiteWindow)
    brainSuiteWindow->loadProtocolCurveset();
}

void WelcomeSplashPage::on_clearListPushButton_clicked()
{
  QSettings settings("BrainSuite",BrainSuiteQtSettings::brainSuiteQtKey);
  settings.remove("recentFileList");
  updateRecentlyUsedFiles();
  if (brainSuiteWindow)
    brainSuiteWindow->updateRecentFileActions();
}

void WelcomeSplashPage::readVersionFromWeb()
{
  if (!downloader) return;
  if (downloader->error())
  {
    ui->newVersionLabel->setText("Unable to establish connection with BrainSuite version server. Click here to try again.");
    return;
  }
  LatestVersionManifest lvm;
  lvm.parse(downloader->downloadedData().data());
  std::ostringstream ostr;
  if (updatesAvailable(ostr,lvm))
  {
    ui->newVersionLabel->setText(ostr.str().c_str());
  }
  else
    ui->newVersionLabel->setText("BrainSuite is up to date.");
  downloader->deleteLater();
  downloader=0;
}

void WelcomeSplashPage::checkForUpdates()
{
  if (!downloader)
  {
    QUrl versionURL(BrainSuiteQtSettings::latestVersionServer.c_str());
    downloader = new FileDownloader(versionURL, this);
    connect(downloader, SIGNAL(downloaded()), SLOT(readVersionFromWeb()));
    ui->newVersionLabel->setText("checking for updates...");
  }
}

void WelcomeSplashPage::on_newVersionLabel_clicked()
{
  checkForUpdates();
}
