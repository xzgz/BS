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

#include "pialsurfacethreaddialog.h"
#include "ui_pialsurfacethreaddialog.h"
#include <qtimer.h>
#include <DS/timer.h>
#include <iostream>
#include <qmessagebox.h>
#include <qevent.h>
#include <pialsurfacetool.h>
#include <iomanip>
#include <brainsuitewindow.h>
#include <corticalextractionsequencedialog.h>
#include <pialsurfaceguitool.h>
extern PialSurfaceGUITool pialSurfaceGUITool;

PialSurfaceThreadDialog::PialSurfaceThreadDialog(BrainSuiteWindow *brainSuiteWindow, QWidget *caller) :
  QDialog(caller),
  statusCode(ThreadDialogStatus::NotStarted),
  ui(new Ui::PialSurfaceThreadDialog),
  brainSuiteWindow(brainSuiteWindow),
  pialSurfaceTool(0), pialSurfaceThread(0), processTimer(0)
{
  ui->setupUi(this);
}

PialSurfaceThreadDialog::~PialSurfaceThreadDialog()
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

void PialSurfaceThreadDialog::on_cancel_clicked()
{
  close();
}

bool PialSurfaceThreadDialog::killThread()
{
  if (pialSurfaceThread && pialSurfaceTool)
  {
    if (pialSurfaceThread->isRunning())
    {
      pialSurfaceTool->halt=true;
      ui->statusMessage->setText("waiting for thread to end");
      ui->statusMessage->repaint();
      QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
      pialSurfaceThread->wait();
    }
    pialSurfaceThread->deleteLater();
    pialSurfaceThread = 0;
  }
  return true;
}

void PialSurfaceThreadDialog::closeEvent(QCloseEvent * event)
{
  if (pialSurfaceThread && pialSurfaceThread->isRunning())
  {
    QMessageBox msgBox(this);
    msgBox.setText("Cancel Pial Surface Process?");
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setInformativeText("Are you sure you want to cancel the running pial surface process?");
    msgBox.setStandardButtons(QMessageBox::Yes|QMessageBox::No);
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

int PialSurfaceThreadDialog::launch(PialSurfaceTool *pialSurfaceTool_)
{
  if (!pialSurfaceTool_) return false;
  pialSurfaceTool = pialSurfaceTool_;
  ui->progressBar->setRange(0,(int)pialSurfaceTool->nIterations());
  ui->progressBar->setValue((int)pialSurfaceTool->iterationNumber());
  if (brainSuiteWindow && brainSuiteWindow->toolManager.corticalExtractionSequenceDialog)
    brainSuiteWindow->toolManager.corticalExtractionSequenceDialog->setProgress(0,(int)pialSurfaceTool->nIterations());
  processTimer = new QTimer(this);
  runtime.start();
  connect(processTimer,SIGNAL(timeout()), this, SLOT(onTimer()));
  processTimer->setInterval(1000);
  processTimer->start();
  ui->statusMessage->setText("Starting thread");
  if (!pialSurfaceThread)
  {
    pialSurfaceThread = new PialSurfaceThread(this,pialSurfaceTool);
    pialSurfaceThread->start();
    statusCode = ThreadDialogStatus::Running;
  }
  return exec();
}

void PialSurfaceThreadDialog::notifyFinished(PialSurfaceThread * /*sender*/)
{
  statusCode = ThreadDialogStatus::Succeeded;
  if (pialSurfaceThread)
  {
    pialSurfaceThread->deleteLater();
    pialSurfaceThread = 0;
  }
  close();
}

void PialSurfaceThreadDialog::updateRuntime()
{
  std::ostringstream ostr;
  runtime.stop();
  ostr<<"Runtime: "<<runtime.elapsedMMSS();
  if (pialSurfaceTool->iterationNo>0)
  {
    float avgMS = float(runtime.ms()) / (pialSurfaceTool->iterationNo);
    int tTotalMS = (int)(avgMS*pialSurfaceTool->nIterations());
    int tRemainingMS = tTotalMS - runtime.ms();
    if (tRemainingMS>0)
    {
      int tRemaining = tRemainingMS/1000;
      ostr<<"\testimated time remaining: "<<int(tRemaining/60)<<':'<<std::setfill('0')<<std::setw(2)<<tRemaining%60<<'\n';
    }
  }
  ui->elapsedTime->setText(ostr.str().c_str());
}

void PialSurfaceThreadDialog::notifyThreadStartingIteration(int iterationNumber)
{
  ui->progressBar->setValue(iterationNumber);
  pialSurfaceGUITool.startingIteration(brainSuiteWindow, pialSurfaceTool, iterationNumber);
  std::ostringstream status;
  status<<"performing pial expansion iteration "<<iterationNumber+1<<"/"<<pialSurfaceTool->numIterations;
  ui->statusMessage->setText(status.str().c_str());
  if (iterationNumber>0)
    updateRuntime();
}

void PialSurfaceThreadDialog::onTimer()
{
  updateRuntime();
}
