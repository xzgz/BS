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

#include "registrationprocessdialog.h"
#include "ui_registrationprocessdialog.h"
#include <iostream>
#include <qmessagebox.h>
#include <qevent.h>
#include <qtimer.h>
#include <surfacelabelprocess.h>

RegistrationProcessDialog::RegistrationProcessDialog(QWidget *parent) :
  QDialog(parent),
  process(0), returnState(NotRun),
  ui(new Ui::RegistrationProcessDialog)
{
  ui->setupUi(this);
  setCursor(Qt::WaitCursor);
}

RegistrationProcessDialog::~RegistrationProcessDialog()
{
  delete ui;
}

void RegistrationProcessDialog::onTimer()
{
  runtime.stop();
  ui->elapsedTime->setText((std::string("Runtime: ") + runtime.elapsedMMSS()).c_str());
}

int RegistrationProcessDialog::run(QString program, QString directory, QStringList arguments)
{
  setWindowTitle(processName);
  runtime.start();
  ui->processDescription->setText(processName);
  ui->statusMessage->setText(tag);
  if (!process)
  {
    process = new SurfaceLabelProcess(this);
    if (!process)
    {
      errorStream<<"could not create "<<tag.toStdString()<<std::endl;
      return QDialog::Rejected;
    }
    std::cout<<"Executing: "<<program.toStdString();
    foreach (QString line, arguments)
      std::cout<<" "<<line.toStdString();
    std::cout<<std::endl;
    process->launch(program,arguments,directory);
  }
  if (!process->waitForStarted())
  {
    errorStream<<"could not launch "<<tag.toStdString()<<std::endl;
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

void RegistrationProcessDialog::notifyFinished(SurfaceLabelProcess *sender)
{
  if (!sender) return;
  if (sender==process)
  {
    if (process->exitCode()==0)
    {
      returnState = Success;
      ui->statusMessage->setText("Finished");
    }
    else
    {
      if (returnState==CanceledByUser) std::cout<<"Canceled by user."<<std::endl;
      returnState = Error;
      ui->statusMessage->setText("Error. Process halted.");
      errorStream<<"Error occurred during "<<tag.toStdString()<<std::endl;
      processStream<<"process log:\n-----\n"<<ui->statusBoxTextEdit->toPlainText().toStdString()<<'\n';
    }
  }
  if (process->state()==QProcess::NotRunning)
  {
    if (process->exitCode()==0)
    {
      returnState = Success;
      setResult(QDialog::Accepted);
      process->terminate();
      //					killThread();
    }
    else
    {
      setResult(QDialog::Rejected);
    }
    close();
  }
}

void RegistrationProcessDialog::parseOutput(QString msg)
{
  if (msg.isEmpty()) return;
  //	if (!sender) return;
  std::string s=msg.toStdString();
  std::string::size_type p=s.find("SVREG::");
  if (p!=s.npos)
  {
    std::string subs=s.substr(p+sizeof("SVREG::")+1);
    std::cout<<msg.toStdString()<<std::endl;
    p=subs.find(':');
    if (p!=subs.npos)
    {
      subs=subs.substr(p+1);
    }
    msg = subs.c_str();
  }
  std::ostringstream ostr;
  ui->statusMessage->setText(msg);
  statusStream<<msg.toStdString()<<'\n';
  ui->statusBoxTextEdit->clear();
  ui->statusBoxTextEdit->insertPlainText(statusStream.str().c_str());
  ui->statusBoxTextEdit->moveCursor (QTextCursor::End) ;
  ui->statusBoxTextEdit->ensureCursorVisible();
  ui->statusBoxTextEdit->repaint();
}

void RegistrationProcessDialog::on_cancel_clicked()
{
  returnState = CanceledByUser;
  close();
}


void	RegistrationProcessDialog::closeEvent ( QCloseEvent * event )
{
  if (process && process->state()==QProcess::Running)
  {
    QMessageBox msgBox(this);
    msgBox.setText("Cancel Registration Process?");
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setInformativeText("Are you sure you want to cancel the running registration process?");
    msgBox.setStandardButtons(QMessageBox::Yes|QMessageBox::No);
    /* int code = */ msgBox.exec();
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
