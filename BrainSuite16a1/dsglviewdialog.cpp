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

#include "dsglviewdialog.h"
#include "ui_dsglviewdialog.h"
#include <brainsuitewindow.h>
#include <dsglwidget.h>
#include <qgridlayout.h>
#include <iostream>
#include <qevent.h>

DSGLViewDialog::DSGLViewDialog(BrainSuiteWindow *parent) :
  QDialog(parent),
  ui(new Ui::DSGLViewDialog), brainSuiteWindow(parent), gridLayout(0)
{
  ui->setupUi(this);
  gridLayout = new QGridLayout();
  setLayout(gridLayout);
  gridLayout->setContentsMargins(0,0,0,0);
  setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint);
}

DSGLViewDialog::~DSGLViewDialog()
{
  delete ui;
}

bool DSGLViewDialog::dettach(DSGLWidget *dsglWidget)
{
  gridLayout->removeWidget(dsglWidget);
  return true;
}

bool DSGLViewDialog::attach(DSGLWidget *dsglWidget)
{
  gridLayout->addWidget(dsglWidget,0,0);
  return true;
}

void DSGLViewDialog::closeEvent(QCloseEvent * /* event*/)
{
  if (brainSuiteWindow)
    brainSuiteWindow->detachDSGLView();
}

void DSGLViewDialog::keyPressEvent ( QKeyEvent * event )
{
  if (!event) return;
  bool shiftkey = event->modifiers()&Qt::ShiftModifier;
  int ascii=(event->key()<255) ? event->key() : 0;
  if (ascii>255) ascii=0;
  if (::isupper(ascii)&&!shiftkey) ascii=::tolower(ascii);
  switch (event->key())
  {
    case Qt::Key_F2 : resize(256,256); event->accept(); return;
    case Qt::Key_F5 : resize(512,512); event->accept(); return;
    case Qt::Key_F9  : resize(1920,1080); event->accept(); return;
    case Qt::Key_F10 : resize(1080,1080); event->accept(); return;
  }
  if (brainSuiteWindow)
    brainSuiteWindow->keyPressEvent(event);
}

void DSGLViewDialog::keyReleaseEvent( QKeyEvent * event )
{
  if (brainSuiteWindow)
    brainSuiteWindow->keyReleaseEvent(event);
}
