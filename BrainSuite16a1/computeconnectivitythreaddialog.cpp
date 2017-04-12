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

#include "computeconnectivitythreaddialog.h"
#include "ui_computeconnectivitythreaddialog.h"
#include <qtimer.h>
#include <computeconnectivitythread.h>
#include <brainsuitewindow.h>
#include <computeconnectivitythread.h>
#include <fibertrackset.h>
#include <qmessagebox.h>
#include <qevent.h>


ComputeConnectivityThreadDialog::ComputeConnectivityThreadDialog(BrainSuiteWindow *brainSuiteWindow, QWidget *caller) :
  QDialog(caller),
  mergeCortialROIs(false),
  ui(new Ui::ComputeConnectivityThreadDialog),
  brainSuiteWindow(brainSuiteWindow),
  thread(0),
  processTimer(0),
  avgLoopTimeMS(0),
  statusCode(ThreadDialogStatus::NotStarted),
  nTracks(0)
{
  ui->setupUi(this);
}

ComputeConnectivityThreadDialog::~ComputeConnectivityThreadDialog()
{
  killThread();
  if (processTimer)
  {
    processTimer->blockSignals(true);
    processTimer->stop();
    processTimer->deleteLater();
    processTimer = 0;
  }
  delete ui;
}

void ComputeConnectivityThreadDialog::on_cancel_clicked()
{
  close();
}

bool ComputeConnectivityThreadDialog::killThread()
{
  if (thread)
  {
    if (thread->isRunning())
    {
      std::cerr<<"connectivity computation canceled by user... waiting for thread to end"<<std::endl;
      setStatus("waiting for thread to end");
      QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
      thread->wait();
    }
    thread->deleteLater();
    thread = 0;
  }
  return true;
}

void ComputeConnectivityThreadDialog::closeEvent(QCloseEvent * event)
{
  if (thread && thread->isRunning())
  {
    QMessageBox msgBox(this);
    msgBox.setText("Cancel connectivity computation?");
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setInformativeText("Are you sure you want to cancel the connectivity computation?");
    msgBox.setStandardButtons(QMessageBox::Yes|QMessageBox::No);
    //int code =
    msgBox.exec();
    if (msgBox.standardButton(msgBox.clickedButton())==QMessageBox::Yes)
    {
      killThread();
      statusCode = ThreadDialogStatus::CanceledByUser;
      setResult(QDialog::Rejected);
    }
    else
    {
      event->ignore();
    }
  }
}

bool ComputeConnectivityThreadDialog::setStatus(std::string status)
{
  ui->statusMessage->setText(status.c_str());
  return true;

}

int ComputeConnectivityThreadDialog::launch()
{
  if (thread)
  {
    std::cerr<<"the ComputeConnectivityThread is already running."<<std::endl;
    return -1;
  }
  statusCode = ThreadDialogStatus::NotStarted;
  if (!brainSuiteWindow)
  {
    std::cerr<<"ComputeConnectivityThreadDialog is not linked to the main window."<<std::endl;
    return -1;
  }
  FiberTrackSet *tracks = brainSuiteWindow->brainSuiteDataManager.fiberTrackSet;
  if (!tracks)
  {
    std::cerr<<"Cannot compute connectivity: no tracks have been loaded or computed."<<std::endl;
    return -1;
  }
  Vol3DBase *vLabel = brainSuiteWindow->brainSuiteDataManager.vLabel;
  if (!vLabel)
  {
    std::cerr<<"Cannot compute connectivity: no brain label data loaded."<<std::endl;
    return -1;
  }
  switch (vLabel->typeID())
  {
    case SILT::Uint16: break;
    case SILT::Sint16: break;
    default:
      std::cout<<"Cannot compute connectivity: label volume datatype ("<<vLabel->datatypeName()<<") is not supported for ROI filtering."<<'\n';
      return -1;
      break;
  }
  nTracks=tracks->nCurves();
  std::ostringstream desc;
  desc<<"analyzing "<<nTracks<<" tracks";
  ui->processDescription->setText(desc.str().c_str());
  ui->progressBar->setRange(0,nTracks);
  ui->progressBar->setValue(0);//(int)tool->iterationNumber());
  processTimer = new QTimer(this);
  runtime.start();
  connect(processTimer,SIGNAL(timeout()), this, SLOT(onTimer()));
  processTimer->setInterval(1000);
  processTimer->start();
  setStatus("Starting thread");

  thread = new ComputeConnectivityThread(this,
                                         brainSuiteWindow->brainSuiteDataManager.connectivityMap,
                                         brainSuiteWindow->brainSuiteDataManager.connectivityProperties,
                                         *tracks,*(Vol3D<uint16> *)vLabel,brainSuiteWindow->brainSuiteDataManager.labelDescriptionSet,
                                         mergeCortialROIs,true);
  thread->start();
  statusCode = ThreadDialogStatus::Running;
  return exec();
}

void ComputeConnectivityThreadDialog::updateRuntime()
{
  std::ostringstream ostr;
  runtime.stop();
  ostr<<"Runtime: "<<runtime.elapsedMMSS();
  int nIterations=0;
  if (nIterations>1)
  {
    int tTotalMS = (int)(avgLoopTimeMS*nIterations);
    int tRemainingMS = tTotalMS - runtime.ms();
    if (tRemainingMS>0)
    {
      int tRemaining = tRemainingMS/1000;
      ostr<<"\testimated time remaining: "<<int(tRemaining/60)<<':'<<std::setfill('0')<<std::setw(2)<<tRemaining%60<<'\n';
    }
  }
  ui->elapsedTime->setText(ostr.str().c_str());
}

void ComputeConnectivityThreadDialog::onTimer()
{
  updateRuntime();
}

void ComputeConnectivityThreadDialog::notifyFinished(ComputeConnectivityThread * /*sender*/)
{
  statusCode = ThreadDialogStatus::Succeeded;
  setStatus("finished.");
  if (thread)
  {
    thread->deleteLater();
    thread = 0;
  }
  close();
}

void ComputeConnectivityThreadDialog::notifyThreadStartingIteration(int iterationNumber)
{
  int nLabels=brainSuiteWindow->brainSuiteDataManager.connectivityMap.labelIDs.size();
  if (iterationNumber>0)
  {
    avgLoopTimeMS = float(runtime.ms()) / iterationNumber;
  }
  ui->progressBar->setRange(0,(int)nLabels);
  ui->progressBar->setValue(iterationNumber);
  std::ostringstream status;
  int id=iterationNumber;
  if (id>=0 && id < nLabels)
  {
    id = brainSuiteWindow->brainSuiteDataManager.connectivityMap.labelIDs[id];
    status<<"computing connectivity for ROI# "<<id;
    if (id>=0 && id<(int)brainSuiteWindow->brainSuiteDataManager.labelDescriptionSet.labelDetails.size())
    {
      status<<"<br/>("<<brainSuiteWindow->brainSuiteDataManager.labelDescriptionSet.labelDetails[id].descriptor<<")";
    }
  }
  else
  {
    status<<"computing connectivity for ROI# "<<id;
  }
  setStatus(status.str());
  if (iterationNumber>0)
    updateRuntime();
}
