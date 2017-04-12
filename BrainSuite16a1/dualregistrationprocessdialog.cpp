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

#include "dualregistrationprocessdialog.h"
#include "ui_dualregistrationprocessdialog.h"
#include <iostream>
#include <qmessagebox.h>
#include <qevent.h>
#include <qtimer.h>
#include <dualregistrationprocess.h>

DualRegistrationProcessDialog::DualRegistrationProcessDialog(QWidget *parent) :
  QDialog(parent), returnState(NotRun),
  ui(new Ui::DualRegistrationProcessDialog),
  leftProcess(0), rightProcess(0), processName("Registration Process"), processTimer(0)
{
  ui->setupUi(this);
  setCursor(Qt::WaitCursor);
}

DualRegistrationProcessDialog::~DualRegistrationProcessDialog()
{
  if (leftProcess)
  {
    leftProcess->terminate();
    delete leftProcess;
  }
  if (rightProcess)
  {
    rightProcess->terminate();
    delete rightProcess;
  }
  if (processTimer)
  {
    processTimer->stop();
    delete processTimer;
  }
  delete ui;
}

void DualRegistrationProcessDialog::onTimer()
{
  runtime.stop();
  ui->elapsedTime->setText((std::string("Runtime: ") + runtime.elapsedMMSS()).c_str());
}

int DualRegistrationProcessDialog::run(QString program, QString directory, QStringList leftArguments, QStringList rightArguments)
{
  setWindowTitle(processName);
  runtime.start();
  ui->processDescription->setText(processName);
  ui->leftStatusMessage->setText(leftTag);
  ui->rightStatusMessage->setText(rightTag);
  if (!leftProcess)
  {
    leftProcess = new DualRegistrationProcess(this);
    if (!leftProcess)
    {
      errorStream<<"could not create "<<leftTag.toStdString()<<std::endl;
      return QDialog::Rejected;
    }
    std::cout<<"Executing: "<<program.toStdString();
    foreach (QString line, leftArguments) std::cout<<" "<<line.toStdString(); std::cout<<std::endl;
    leftProcess->launch(program,leftArguments,directory);
  }
  if (!rightProcess)
  {
    rightProcess = new DualRegistrationProcess(this);
    if (!rightProcess)
    {
      errorStream<<"could not create "<<rightTag.toStdString()<<std::endl;
      return QDialog::Rejected;
    }
    std::cout<<"Executing: "<<program.toStdString();
    foreach (QString line, rightArguments) std::cout<<" "<<line.toStdString(); std::cout<<std::endl;
    rightProcess->launch(program,rightArguments,directory);
  }
  if (!leftProcess->waitForStarted())
  {
    errorStream<<"could not launch "<<leftTag.toStdString()<<std::endl;
    errorStream<<"please check that the executable "<<program.toStdString()
              <<" exists and that "<<directory.toStdString()<<" is a valid directory."<<std::endl;
    return QDialog::Rejected;
  }
  if (!rightProcess->waitForStarted())
  {
    errorStream<<"could not launch "<<rightTag.toStdString()<<std::endl;
    errorStream<<"please check that the executable "<<program.toStdString()
              <<" exists and that "<<directory.toStdString()<<" is a valid directory."<<std::endl;
    return QDialog::Rejected;
  }
  processTimer = new QTimer(this);
  connect(processTimer,SIGNAL(timeout()), this, SLOT(onTimer()));
  processTimer->setInterval(1000);
  processTimer->start();
  return exec();
}

void DualRegistrationProcessDialog::notifyFinished(DualRegistrationProcess *sender)
{
  if (!sender) return;
  if (sender==leftProcess)
  {
    if (leftProcess->exitCode()==0)
    {
      ui->leftStatusMessage->setText("Finished");
    }
    else
    {
      ui->leftStatusMessage->setText("Error. Process halted.");
    }
    if (rightProcess->state()==QProcess::Starting) return;
  }
  else if (sender==rightProcess)
  {
    if (rightProcess->exitCode()==0)
    {
      ui->rightStatusMessage->setText("Finished");
    }
    else
    {
      ui->rightStatusMessage->setText("Error. Process halted.");
    }
    if (leftProcess->state()==QProcess::Starting) return;
  }
  if (leftProcess->state()==QProcess::NotRunning && rightProcess->state()==QProcess::NotRunning)
  {
    if (rightProcess->exitCode()==0 && leftProcess->exitCode()==0)
    {
      returnState = Success;
      setResult(QDialog::Accepted);
    }
    else
    {
      if (returnState==CanceledByUser) std::cout<<"Process canceled by user."<<std::endl;
      returnState = Error;
      QMessageBox msgBox;
      msgBox.setText("Registration Process Error");
      msgBox.setIcon(QMessageBox::Critical);
      msgBox.setStandardButtons(QMessageBox::Ok);
      msgBox.exec();
      setResult(QDialog::Rejected);
      if (rightProcess->exitCode()!=0)
      {
        errorStream<<"Error occurred during "<<leftTag.toStdString()<<std::endl;
        if (leftProcess->exitCode()==78)
          errorStream<<"A script configuration error occured.\n";
        leftProcessStream<<"left log:\n-----\n"<<ui->leftStatusBoxTextEdit->toPlainText().toStdString()<<'\n';
      }
      if (rightProcess->exitCode()!=0)
      {
        errorStream<<"Error occurred during "<<rightTag.toStdString()<<std::endl;
        if (rightProcess->exitCode()==78)
          errorStream<<"A script configuration error occured.\n";
        rightProcessStream<<"right log:\n-----\n"<<ui->rightStatusBoxTextEdit->toPlainText().toStdString()<<'\n';
      }
    }
    close();
  }
}

void DualRegistrationProcessDialog::parseOutput(DualRegistrationProcess *sender, QString msg)
{
  if (msg.isEmpty()) return;
  if (!sender) return;
  std::string s=msg.toStdString();
  std::string::size_type p=s.find("SVREG::");
  if (p!=s.npos)
  {
    std::string subs=s.substr(p+sizeof("SVREG::")+1);
    std::cout<<s<<std::endl;
    p=subs.find(':');
    if (p!=subs.npos)
    {
      subs=subs.substr(p+1);
    }
    msg = subs.c_str();
  }
  if (sender==leftProcess)
  {
    std::ostringstream ostr;
    ui->leftStatusMessage->setText(msg);
    leftStatusStream<<msg.toStdString()<<'\n';
    ui->leftStatusBoxTextEdit->clear();
    ui->leftStatusBoxTextEdit->insertPlainText(leftStatusStream.str().c_str());
    ui->leftStatusBoxTextEdit->moveCursor (QTextCursor::End) ;
    ui->leftStatusBoxTextEdit->ensureCursorVisible();
    ui->leftStatusBoxTextEdit->repaint();
  }
  else if (sender==rightProcess)
  {
    std::ostringstream ostr;
    ui->rightStatusMessage->setText(msg);
    rightStatusStream<<msg.toStdString()<<'\n';
    ui->rightStatusBoxTextEdit->clear();
    ui->rightStatusBoxTextEdit->insertPlainText(rightStatusStream.str().c_str());
    ui->rightStatusBoxTextEdit->moveCursor (QTextCursor::End) ;
    ui->rightStatusBoxTextEdit->ensureCursorVisible();
    ui->rightStatusBoxTextEdit->repaint();
  }
}

void	DualRegistrationProcessDialog::closeEvent ( QCloseEvent * event )
{
  if ((leftProcess && leftProcess->state()==QProcess::Running)
      &&(rightProcess && rightProcess->state()==QProcess::Running))
  {
    QMessageBox msgBox(this);
    msgBox.setText("Cancel Registration Process?");
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setInformativeText("Are you sure you want to cancel the running registration process?");
    msgBox.setStandardButtons(QMessageBox::Yes|QMessageBox::No);
    msgBox.exec();
    if (msgBox.standardButton(msgBox.clickedButton())==QMessageBox::Yes)
    {
      setResult(QDialog::Rejected);
    }
    else
    {
      event->ignore();
    }
  }
}

void DualRegistrationProcessDialog::on_cancel_clicked()
{
  returnState = CanceledByUser;
  close();
}
