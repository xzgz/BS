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

#include "bfcthreaddialog.h"
#include "ui_bfcthreaddialog.h"
#include <qtimer.h>
#include <DS/timer.h>
#include <qmessagebox.h>
#include <qevent.h>
#include <bfcguitool.h>
#include <brainsuitewindow.h>
#include <nonuniformitycorrectiondialog.h>
#include <corticalextractionsequencedialog.h>
#include <BFC/clampcopy.h>

BFCThreadDialog::BFCThreadDialog(BrainSuiteWindow *brainSuiteWindow, QWidget *caller) :
  QDialog(caller),
  ui(new Ui::BFCThreadDialog),
  brainSuiteWindow(brainSuiteWindow),
  tool(0), thread(0), processTimer(0),
  avgLoopTimeMS(0),
  statusCode(ThreadDialogStatus::NotStarted)
{
  ui->setupUi(this);
}

BFCThreadDialog::~BFCThreadDialog()
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

void BFCThreadDialog::on_cancel_clicked()
{
  close();
}

bool BFCThreadDialog::killThread()
{
  if (thread && tool)
  {
    if (thread->isRunning())
    {
      tool->bfcTool.bfc.halt=true;
      std::cerr<<"Nonuniformity correction canceled by user... waiting for thread to end"<<std::endl;
      setStatus("waiting for thread to end");
      QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
      thread->wait();
    }
    thread->deleteLater();
    thread = 0;
  }
  return true;
}

void BFCThreadDialog::closeEvent(QCloseEvent * event)
{
  if (thread && thread->isRunning())
  {
    QMessageBox msgBox(this);
    msgBox.setText("Cancel nonuniformity correction?");
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setInformativeText("Are you sure you want to cancel the nonuniformity correction process?");
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

bool BFCThreadDialog::setStatus(std::string status)
{
  ui->statusMessage->setText(status.c_str());
  if (!brainSuiteWindow) return false;
  if (brainSuiteWindow->toolManager.nonuniformityCorrectionDialog)
    brainSuiteWindow->toolManager.nonuniformityCorrectionDialog->updateStepText(status);
  if (brainSuiteWindow->toolManager.corticalExtractionSequenceDialog)
    brainSuiteWindow->toolManager.corticalExtractionSequenceDialog->updateStepText(status);
  return true;
}

int BFCThreadDialog::launch(BFCGUITool *bfcGUITool_)
{
  if (!bfcGUITool_) return false;
  tool = bfcGUITool_;
  ui->progressBar->setRange(0,(int)tool->nIterations());
  ui->progressBar->setValue((int)tool->iterationNumber());
  if (brainSuiteWindow && brainSuiteWindow->toolManager.corticalExtractionSequenceDialog)
  {
    brainSuiteWindow->toolManager.corticalExtractionSequenceDialog->setProgress(0,(int)tool->nIterations());
  }
  if (brainSuiteWindow && brainSuiteWindow->toolManager.nonuniformityCorrectionDialog)
  {
    brainSuiteWindow->toolManager.nonuniformityCorrectionDialog->setProgress(0,(int)tool->nIterations());
  }
  processTimer = new QTimer(this);
  runtime.start();
  connect(processTimer,SIGNAL(timeout()), this, SLOT(onTimer()));
  processTimer->setInterval(1000);
  processTimer->start();
  setStatus("Starting thread");
  if (!thread)
  {
    ClampCopy::vcopy(vWorking,brainSuiteWindow->brainSuiteDataManager.volume);
    thread = new BFCThread(this,tool,vWorking,vSpline,brainSuiteWindow->brainSuiteDataManager.vMask);
    thread->start();
    statusCode = ThreadDialogStatus::Running;
  }
  return exec();
}

void BFCThreadDialog::notifyFinished(BFCThread * /*sender*/)
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

bool BFCThreadDialog::copyWorkingVolume(Vol3DBase *vOut)
{
  if (!vOut) { std::cerr<<"error: empty volume sent to BFC Thread"<<std::endl; return false; }
  return ClampCopy::vcopy(vOut,vWorking);
}

void BFCThreadDialog::updateRuntime()
{
  std::ostringstream ostr;
  runtime.stop();
  ostr<<"Runtime: "<<runtime.elapsedMMSS();
  if (tool->bfcTool.bfc.iterationNo>1)
  {
    int tTotalMS = (int)(avgLoopTimeMS*tool->bfcTool.bfc.nIterations());
    int tRemainingMS = tTotalMS - runtime.ms();
    if (tRemainingMS>0)
    {
      int tRemaining = tRemainingMS/1000;
      ostr<<"\testimated time remaining: "<<int(tRemaining/60)<<':'<<std::setfill('0')<<std::setw(2)<<tRemaining%60<<'\n';
    }
  }
  ui->elapsedTime->setText(ostr.str().c_str());
}

void BFCThreadDialog::notifyThreadStartingIteration(int iterationNumber)
{
  if (tool->bfcTool.bfc.iterationNo>0)
  {
    avgLoopTimeMS = float(runtime.ms()) / (tool->bfcTool.bfc.iterationNo);
  }
  ui->progressBar->setRange(0,(int)tool->bfcTool.bfc.nIterations());
  ui->progressBar->setValue(iterationNumber);
  if (brainSuiteWindow->toolManager.corticalExtractionSequenceDialog)
    brainSuiteWindow->toolManager.corticalExtractionSequenceDialog->setProgress(iterationNumber,(int)tool->bfcTool.bfc.nIterations());
  if (brainSuiteWindow->toolManager.nonuniformityCorrectionDialog)
    brainSuiteWindow->toolManager.nonuniformityCorrectionDialog->setProgress(iterationNumber,(int)tool->bfcTool.bfc.nIterations());
  std::ostringstream status;
  status<<"performing bfc iteration "<<iterationNumber+1<<"/"<<tool->bfcTool.bfc.nIterations();
  setStatus(status.str());
  if (iterationNumber>0)
    updateRuntime();
}

void BFCThreadDialog::onTimer()
{
  updateRuntime();
}
