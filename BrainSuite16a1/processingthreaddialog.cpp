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

#include "processingthreaddialog.h"
#include "ui_processingthreaddialog.h"
#include <qtimer.h>
#include <qthread.h>
#include <qmessagebox.h>
#include <qevent.h>
#include <qdebug.h>
#include <iostream>

ProcessingThread::ProcessingThread(ProcessingThreadDialog *parent, std::string processTitleStr) : QThread(parent), parent(parent), processTitleStr(processTitleStr)
{
  connectSlots();
}

ProcessingThread::~ProcessingThread()
{
}

std::string ProcessingThread:: status()
{
  std::ostringstream ostr;
  ostr<<"processing slice "<<progressPosition()<<'/'<<progressMaximum();
  return ostr.str();
}

void ProcessingThread::connectSlots()
{
  connect (this, SIGNAL(finished()),
           this, SLOT(whenFinished()));
  connect (this, SIGNAL(notifyParentFinished(ProcessingThread *)),
           parent, SLOT(notifyFinished(ProcessingThread *)));
}

void ProcessingThread::whenFinished()
{
  if (parent)
  {
    emit notifyParentFinished(this);
  }
}

ProcessingThreadDialog::ProcessingThreadDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::ProcessingThreadDialog),
  processingThread(0), processTimer(0)
{
  ui->setupUi(this);
}

ProcessingThreadDialog::~ProcessingThreadDialog()
{
  killThread();
  if (processTimer)
  {
    processTimer->blockSignals(true);
    processTimer->stop();
    processTimer->deleteLater();
    processTimer=0;
  }
  delete ui;
}

void ProcessingThreadDialog::setStatus(std::string status)
{
  ui->threadStatusLabel->setText(status.c_str());
}

void ProcessingThreadDialog::closeEvent(QCloseEvent * event)
{
  if (processingThread && processingThread->isRunning())
  {
    QMessageBox msgBox(this);
    msgBox.setText(("Cancel "+processingThread->processTitle()+"?").c_str());
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setInformativeText(("Are you sure you want to cancel the "+processingThread->processTitle()+" process?").c_str());
    msgBox.setStandardButtons(QMessageBox::Yes|QMessageBox::No);
    msgBox.exec();
    if (msgBox.standardButton(msgBox.clickedButton())==QMessageBox::Yes)
    {
      if (processingThread&&processingThread->isRunning())
      {
        killThread();
        statusCode = ThreadDialogStatus::CanceledByUser;
        setResult(QDialog::Rejected);
      }
      else if (processingThread)
      {
        deleteThread();
      }
    }
    else
    {
      if (processingThread && processingThread->isRunning()) event->ignore();
    }
  }
}

bool ProcessingThreadDialog::deleteThread()
{
  cleanupMutex.lock();
  if (processingThread) processingThread->deleteLater();
  if (processingThread) processingThread = 0;
  cleanupMutex.unlock();
  return true;
}

bool ProcessingThreadDialog::killThread()
{
  if (processingThread)
  {
    if (processingThread->isRunning())
    {
      if (processingThread->stop())
      {
        std::cerr<<"Processing canceled by user... waiting for thread to end"<<std::endl;
        setStatus("waiting for thread to end");
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
      }
      else
        std::cerr<<"unable to cancel process... waiting for it to finish"<<std::endl;
      if (processingThread) processingThread->wait();
    }
    deleteThread();
  }
  return true;
}

void ProcessingThreadDialog::updateRuntime()
{
  runtime.stop();
  ui->runningTimeLabel->setText((std::string("Runtime: ") + runtime.elapsedMMSS()).c_str());
}

void ProcessingThreadDialog::notifyFinished(ProcessingThread */*sender*/)
{
  setResult(QDialog::Accepted);
  statusCode = ThreadDialogStatus::Succeeded;
  setStatus("Finished.");
  if (processingThread)
  {
    deleteThread();
  }
  close();
}

void ProcessingThreadDialog::onTimer()
{
  if (processingThread)
  {
    ui->threadStatusLabel->setText(processingThread->status().c_str());
    ui->progressBar->setRange(0,processingThread->progressMaximum());
    ui->progressBar->setValue(processingThread->progressPosition());
  }
  updateRuntime();
}

void ProcessingThreadDialog::on_cancelPushButton_clicked()
{
  close();
}

int ProcessingThreadDialog::launch(ProcessingThread *processingThread_)
{
  if (processingThread!=0) { std::cerr<<"ProcessingThreadDialog passed a null pointer"<<std::endl; return QDialog::Rejected; }
  processingThread=processingThread_;
  setWindowTitle(processingThread->processTitle().c_str());
  runtime.start();
  ui->processDescriptionLabel->setText(processingThread->processTitle().c_str());
  ui->threadStatusLabel->setText("starting up...");
  ui->progressBar->setValue(0);
  processingThread->start();
  statusCode = ThreadDialogStatus::Running;
  processTimer = new QTimer(this);
  connect(processTimer,SIGNAL(timeout()), this, SLOT(onTimer()));
  processTimer->setInterval(100);
  processTimer->start();
  return exec();
}
