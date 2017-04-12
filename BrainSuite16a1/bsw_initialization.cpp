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

#include "brainsuitewindow.h"
#include "ui_brainsuitewindow.h"
#include <colormap.h>
#include <qmessagebox.h>
#include <qdebugstream.h>
#include <welcomesplashpage.h>
#include <brainsuiteqtsettings.h>
#include <qdir.h>
#include <dsglviewdialog.h>
#include <dsglwidget.h>
#include <imageframe.h>
#include <imagewidget.h>
#include <surfacedisplayform.h>
#include <toolmanager.h>
#include <surfacedisplayform.h>
#include <diffusiontoolboxform.h>
#include <brainsuiteinstalldialog.h>
#include <buildversion.h>
#include <imagedisplaypropertiesform.h>
#include <delineationtoolboxform.h>
#include <curvetoolboxform.h>
#include <qtoolbutton.h>
#include <svregguitool.h>
#include <qdiriterator.h>
#include <qsettings.h>
#include <brainsuitesettings.h>
#include <harditool.h>
#include <qdebug.h>
#include <connectivityviewdialog.h>

extern SVRegGUITool svregGUITool;
extern HARDITool hardiTool;
bool checkSVRegVersion(SVRegManifest &manifest);

template <class Form>
bool BrainSuiteWindow::attachFormToDockWidget(Form *&form, QDockWidget *dockWidget, std::string menu, std::string tooltip)
{
  form = new Form(this,dockWidget);
  dockWidget->hide();
  dockWidget->setWidget(form);
  QAction *action = dockWidget->toggleViewAction();
  action->setText(QString(menu.c_str()));
  action->setStatusTip(QString(tooltip.c_str()));
  action->setChecked(false);
  ui->menu_View->addAction(action);
  dockWidgets.push_back(dockWidget);
  return true;
}

BrainSuiteWindow::BrainSuiteWindow(int /*argc*/, char * /*argv */[], QWidget *parent) :
  QMainWindow(parent),
  viewMode(Ortho3D), lastViewMode(NoView),
  disablePathChecking(false), fileDialogOptions(QFileDialog::DontUseNativeDialog),
  popoutSurfaceView(false),
  activePort(-1),
  ui(new Ui::BrainSuiteWindow),
  qDebugStream(0), qErrorStream(0),
  dsglWidget(0), disableRecentFileList(false),
  imageLUT(ColorMap::greyLUT), imageOverlay1LUT(ColorMap::greyLUT), imageOverlay2LUT(ColorMap::greyLUT)
{
  ui->setupUi(this);
  ui->action_Tissue_Fraction_Volume->setVisible(false);
  ui->action_Hemisphere_Label_Volume->setVisible(false);
  ui->actionOpen_connectivity_matrix->setVisible(false);
  ui->actionLoad_ODF_Maxima->setVisible(false);
  ui->actionODF_Eig->setVisible(false);
  if (!BrainSuiteSettings::noConsole)
  {
    qDebugStream = new QDebugStream (std::cout, ui->consoleTextEdit);
    qDebugStream->front = "<span style=color:black;font-weight:normal>";
    qErrorStream  = new QDebugStream (std::cerr, ui->consoleTextEdit);
    qErrorStream->front= "<span style=color:red;font-weight:bold>";
    qErrorStream->back = "</span>";
  }
	QSettings settings("BrainSuite",BrainSuiteQtSettings::brainSuiteQtKey);//brainSuiteQtKey
  BrainSuiteQtSettings::enableCheckForUpdates=settings.value(BrainSuiteQtSettings::qtcheckforupdateskey.c_str(),false).toBool();
  ui->actionAllow_BrainSuite_to_Check_for_Updates->setChecked(BrainSuiteQtSettings::enableCheckForUpdates);
  ui->actionUse_Native_File_Dialogs->setChecked(BrainSuiteQtSettings::getUseNativeFileDialogs());
  fileDialogOptions = (BrainSuiteQtSettings::getUseNativeFileDialogs()) ? QFileDialog::Options() :  QFileDialog::DontUseNativeDialog;
  setStyleSheet("QMainWindow::separator { background: rgb(200, 200, 200); width: 2px; height: 2px; }");
  std::string compiler("unknown");
#ifdef _MSC_FULL_VER
  {
    switch (_MSC_VER)
    {
      case 1600 : compiler="MSVC2010"; break;
      case 1700 : compiler="MSVC2012"; break;
      case 1800 : compiler="MSVC2013"; break;
    }
  }
#else
  compiler=__VERSION__;
#endif
  std::cout<<"<b>Welcome to "<<BrainSuiteQtSettings::titleString<<" </b>(build #"<<BuildVersion::build()<<", ";
  std::cout<<"built with Qt "<<qVersion()<<", compiled with "<<compiler<<" on "<<__DATE__<<")"<<std::endl;
  std::cout<<"Copyright (C) 2016 The Regents of the University of California"<<std::endl;
  std::cout<<"This program is free software and is released under a GNU General Public License; Version 2. "
             "For more details, please see the enclosed license files.\n";
  std::cout<<"BrainSuite uses the Qt GUI Toolkit. "
          <<"The Qt GUI Toolkit is Copyright (C) 2015 Digia Plc and/or its subsidiary(-ies) ";
  std::cout<<"and is licensed under the LGPL v2.1."<<std::endl;
  if (BrainSuiteQtSettings::qtinstalldir.empty())
  {
  }
  else
  {
    std::cout<<"BrainSuite install directory: "<<BrainSuiteQtSettings::qtinstalldir<<std::endl;
  }
  if (!svregGUITool.initializeTool(ces))
  {
#ifndef QT_X86_32
    std::cerr<<"SVReg is not configured to run. If you wish to perform surface/volume registration, please install it."<<std::endl;//<<std::endl;
#endif
  }
  else
  {
    std::cout<<"Using SVReg installed in directory "<<BrainSuiteQtSettings::qtsvregdir<<std::endl;
  }
  hardiTool.odfFactory.flipX = false;
  hardiTool.odfFactory.flipY = true;
  hardiTool.odfFactory.flipZ = false;
  hardiTool.updateBasis();
  QAction *consoleAction = ui->consoleDockWidget->toggleViewAction();
  consoleAction->setText(QString("&BrainSuite Log Console"));
  consoleAction->setStatusTip(QString("Press to show/hide console."));
  consoleAction->setChecked(true);
  ui->menu_View->addAction(consoleAction);
  if (BrainSuiteSettings::noConsole)
    ui->consoleDockWidget->hide();
  attachFormToDockWidget(toolManager.imageDisplayPropertiesForm,ui->imageDisplayPropertiesDockWidget,"&Image Display Sidebar","Press to show/hide image display sidebar.");
  attachFormToDockWidget(toolManager.delineationToolboxForm,ui->delineationToolboxDockWidget,"&Delineation Tool Sidebar","Press to show/hide delineation tool sidebar.");
  attachFormToDockWidget(toolManager.surfaceDisplayForm,ui->surfaceDisplayPropertiesDockWidget,"&Surface Display Properties Sidebar","Press to show/hide surface display sidebar.");
  attachFormToDockWidget(toolManager.diffusionToolboxForm,ui->diffusionToolboxDockWidget,"&Diffusion Toolbox Sidebar","Press to show/hide diffusion toolbox sidebar.");
  attachFormToDockWidget(toolManager.curveToolboxForm,ui->curveToolDockWidget,"&Curve Tool Sidebar","Press to show/hide curve tool sidebar.");
  setTabPosition(Qt::LeftDockWidgetArea,QTabWidget::West);
  setTabPosition(Qt::RightDockWidgetArea,QTabWidget::East);
  setTabShape(QTabWidget::Triangular);
  if (dockWidgets.size()!=0)
  {
    for (auto i=1U;i<dockWidgets.size();i++)
      tabifyDockWidget(dockWidgets[0],dockWidgets[i]);
  }
  initializeColorTable();
  setWindowTitle(BrainSuiteQtSettings::titleString.c_str());
  statusBar()->showMessage("");
  ui->splashWidget->attachAndInitialize(this);
  createMenus();
  createActions();
  createToolbars();
  updateMenuChecks();
  show();
  initializeSettings();
  checkForSVRegUpdate();
  if (toolManager.delineationToolboxForm)
  {
    toolManager.delineationToolboxForm->updateLabelSelectionComboBox();
    toolManager.delineationToolboxForm->chooseLabel(toolManager.delineationToolboxForm->currentLabel());
  }
  ui->action_Popout_Surface_View->setChecked(popoutSurfaceView);
  ui->actionBig_Format_Data_Viewer->setVisible(false);
  ui->actionTime_series_list->setVisible(false);
  ui->actionTime_Series_Playback->setVisible(false);
}

void BrainSuiteWindow::initializeSettings()
{
  if (!BrainSuiteQtSettings::qtinstalldir.empty())
  {
    BrainSuiteQtSettings::disableSavingDirectory();
    {
      std::string defaultLabelDescriptionFiles[] = {
        "brainsuite_labeldescriptions_14May2014.xml"
      };
      const int nDefaultLabelDescriptionFiles = sizeof(defaultLabelDescriptionFiles)/sizeof(defaultLabelDescriptionFiles[0]);
      std::string defaultLabelDescriptionFile;
      std::string labeldescpath=BrainSuiteQtSettings::qtinstalldir + "/labeldesc/";
      bool loadedIt=false;
      for (int i=0;i<nDefaultLabelDescriptionFiles;i++)
      {
        defaultLabelDescriptionFile = QDir::toNativeSeparators(QDir::cleanPath((labeldescpath+defaultLabelDescriptionFiles[i]).c_str())).toStdString();
        if (loadLabelDescriptionFile(defaultLabelDescriptionFile))
        {
          loadedIt=true;
          break;
        }
      }
      if (!loadedIt)
      {
        if (nDefaultLabelDescriptionFiles>0)
          std::cerr<<"Unable to load label description file: "<<defaultLabelDescriptionFiles[0]<<std::endl;
        else
          std::cerr<<"Unable to load a label description file."<<std::endl;
        std::cerr<<"To install or re-install this file, please go to the menu item Help -> Reinstall BrainSuite Atlas Files"<<std::endl;
      }
    }
    std::string defaultProtocolFile =
        QDir::toNativeSeparators(QDir::cleanPath((BrainSuiteQtSettings::qtinstalldir + "/protocols/brainsuite_protocol_Damasio2009.xml").c_str())).toStdString();
    if (!loadCurveProtocol(defaultProtocolFile))
    {
      std::cerr<<"Unable to load curve protocol file: "<<defaultProtocolFile<<std::endl;
      std::cerr<<"To install or re-install this file, please go to the menu item Help -> Reinstall BrainSuite Atlas Files"<<std::endl;
    }
    BrainSuiteQtSettings::enableSavingDirectory();
  }
  else
  {
		std::cerr<<"BrainSuite files have not been installed -- unable to load label description and protocol files."<<std::endl;
    std::cerr<<"To install these files, please go to the menu item Help -> Reinstall BrainSuite Atlas Files"<<std::endl;
  }
}

void BrainSuiteWindow::checkForSVRegUpdate()
{      
  if (BrainSuiteQtSettings::qtinstalldir.empty()==false)
  {
    QDirIterator it(BrainSuiteQtSettings::qtinstalldir.c_str(), QDirIterator::Subdirectories); // should only see subdirectories, but grabs all files!!!
    std::list<SVRegManifest> newerVersions;
    while (it.hasNext())
    {
      QString nextDir=it.next();
      if (nextDir.isEmpty()) continue;
      if (it.fileName().toStdString()==".") continue;
      if (it.fileName().toStdString()=="..") continue;
      if (QDir(nextDir).exists()==false) continue;
      std::string nextManifestFile(nextDir.toStdString() + "/svregmanifest.xml");
      if (nextManifestFile==svregGUITool.svreg.svregManifest.filepath) continue;
      SVRegManifest otherSVRegManifest;
      if (otherSVRegManifest.read(nextManifestFile))
      {
        if (!checkSVRegVersion(otherSVRegManifest))
        {
          std::cout<<"incompatible svreg: "<<nextManifestFile<<std::endl;
          continue;
        }
        if (otherSVRegManifest.buildNumber > svregGUITool.svreg.svregManifest.buildNumber)
        {
          if (StrUtil::eqnocase(otherSVRegManifest.platform,BrainSuiteQtSettings::platform))
          {
            newerVersions.push_back(otherSVRegManifest);
          }
          else
          {
          }
        }
      }
    }
    if (newerVersions.size()>0)
    {
      QMessageBox msgBox(this);
      if (BrainSuiteQtSettings::qtsvregdir.empty())
        msgBox.setText("Install SVReg?");
      else
        msgBox.setText("Update SVReg?");
      msgBox.setIcon(QMessageBox::Question);
      std::ostringstream msg;
      std::list<SVRegManifest>::iterator newest=newerVersions.begin();
      for (std::list<SVRegManifest>::iterator i=newerVersions.begin(); i!=newerVersions.end(); i++)
      {
        if (i->buildNumber>newest->buildNumber) newest=i;
      }
      QFileInfo fileinfo(newest->filepath.c_str());
      if (BrainSuiteQtSettings::qtsvregdir.empty())
      {
				msg<<"An SVReg folder has been detected in your BrainSuite folder."
             " Would you like to configure BrainSuite to use it?\n";
      }
      else
				msg<<"A newer version of SVReg has been detected in your BrainSuite folder."
             " Would you like to configure BrainSuite to use this one?\n";
      msg<<"Version: "<<newest->versionstr<<'\n';
      if (newest->buildstr.empty()==false)
        msg<<"Build #: "<<newest->buildstr<<'\n';
      msg<<"Path: "<<QDir::toNativeSeparators(fileinfo.absolutePath()).toStdString(); //newest->filepath<<'\n';
      msgBox.setInformativeText(msg.str().c_str());
      msgBox.setStandardButtons(QMessageBox::Yes|QMessageBox::No);
      msgBox.setDefaultButton(QMessageBox::Yes);
      msgBox.exec();
      if (msgBox.standardButton(msgBox.clickedButton())==QMessageBox::Yes)
      {
        setSVRegDirectory(fileinfo.absolutePath().toStdString());
      }
    }
  }
}

BrainSuiteWindow::ViewSettings::ViewSettings() : singleView(false), syncGL(true), blockSync(false)
{
}

BrainSuiteWindow::~BrainSuiteWindow()
{
  if (toolManager.dsglViewDialog)
    toolManager.dsglViewDialog->dettach(dsglWidget);
  toolManager.dsglViewDialog.reset();
  toolManager.connectivityViewDialog.reset();
  delete qDebugStream;
  delete qErrorStream;
  delete ui;
}

void BrainSuiteWindow::createActions()
{
}

void BrainSuiteWindow::createToolbars()
{
}


void BrainSuiteWindow::initializeColorTable()
{
  const size_t nLabels = brainSuiteDataManager.labelDescriptionSet.labelDetails.size();
  for (size_t i=0;i<nLabels;i++)
  {
    int r=0,g=0,b=0;
    r = ((i&32)>0)*128;
    g = ((i&16)>0)*128;
    b = ((i&8)>0) *128;
    r += ((i&4)>0)*127;
    g += ((i&2)>0)*127;
    b += ((i&1)>0)*127;
    int col = (r<<16) + (g<<8) + b;
    brainSuiteDataManager.labelDescriptionSet.labelDetails[i] = LabelDetails("","",col);
  }
}

void BrainSuiteWindow::initializeSurfaceView()
{
  if (!dsglWidget)
  {
    dsglWidget = new DSGLWidget(this);
    if (toolManager.surfaceDisplayForm) toolManager.surfaceDisplayForm->updateSliceViewStatus();
  }
  if (dsglWidget)
  {
    if (popoutSurfaceView)
    {
      if (!toolManager.dsglViewDialog)
        toolManager.dsglViewDialog.reset(new DSGLViewDialog(this));
      if (toolManager.dsglViewDialog)
      {
        toolManager.dsglViewDialog->show();
        toolManager.dsglViewDialog->attach(dsglWidget);
        dsglWidget->show();
      }
    }
    else
    {
      if (toolManager.dsglViewDialog)
      {
        toolManager.dsglViewDialog->hide();
        toolManager.dsglViewDialog->dettach(dsglWidget);
      }
      ui->gridLayout->addWidget(dsglWidget,1,1);
    }
  }
  ui->gridLayout->setContentsMargins(0,0,0,0);
  ui->splashWidget->hide();
  ui->gridLayout->removeWidget(ui->splashWidget);
  ui->gridLayout->removeItem(ui->verticalSpacer);
  ui->gridLayout->removeItem(ui->verticalSpacer_2);
  ui->gridLayout->removeItem(ui->horizontalSpacer);
  ui->gridLayout->removeItem(ui->horizontalSpacer_2);
}

void BrainSuiteWindow::initializeWindows()
{
  ui->splashWidget->hide();
  ui->gridLayout->removeWidget(ui->splashWidget);
  ui->gridLayout->removeItem(ui->verticalSpacer);
  ui->gridLayout->removeItem(ui->verticalSpacer_2);
  ui->gridLayout->removeItem(ui->horizontalSpacer);
  ui->gridLayout->removeItem(ui->horizontalSpacer_2);
  if (ports.size())
  {
    for (size_t i=0;i<ports.size();i++)
    {
      delete ports[i];
      ports[i]=0;
    }
  }
  ports.resize(3);
  ports[0] = new ImageFrame(this);
  ports[1] = new ImageFrame(this);
  ports[2] = new ImageFrame(this);
  ports[0]->image()->orientation = Orientation::XZ;
  ports[1]->image()->orientation = Orientation::YZ;
  ports[2]->image()->orientation = Orientation::XY;
  ports[0]->setVisible(true);
  ports[1]->setVisible(true);
  ports[2]->setVisible(true);
  if (ports.size()>0) ui->gridLayout->addWidget(ports[0],0,0);
  if (ports.size()>1) ui->gridLayout->addWidget(ports[1],0,1);
  if (ports.size()>2) ui->gridLayout->addWidget(ports[2],1,0);
  if (!dsglWidget)
    dsglWidget = new DSGLWidget(this);
  if (dsglWidget)
    ui->gridLayout->addWidget(dsglWidget,1,1);
  if (toolManager.surfaceDisplayForm) toolManager.surfaceDisplayForm->updateSliceViewStatus();
  ui->gridLayout->setContentsMargins(0,0,0,0);
}

bool BrainSuiteWindow::checkInstall()
{
	BrainSuiteInstallDialog installDialog(this,QDir::toNativeSeparators(QDir::homePath()+QString("/"+BrainSuiteQtSettings::brainSuiteQtKey+"/")));
  installDialog.exec();
  return true;
}
