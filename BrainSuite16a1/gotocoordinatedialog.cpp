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

#include "gotocoordinatedialog.h"
#include "ui_gotocoordinatedialog.h"
#include <brainsuitewindow.h>

GotoCoordinateDialog::GotoCoordinateDialog(BrainSuiteWindow *parent) :
  QDialog(parent),
  ui(new Ui::GotoCoordinateDialog), brainSuiteWindow(parent)
{
  ui->setupUi(this);
  if (brainSuiteWindow)
    initialPosition=brainSuiteWindow->currentVolumePosition;
  std::ostringstream ostr;
  ostr<<initialPosition;
  ui->coordinateLineEdit->setText(ostr.str().c_str());
  ui->coordinateLineEdit->setFocus();
  ui->coordinateLineEdit->selectAll();
}

GotoCoordinateDialog::~GotoCoordinateDialog()
{
  delete ui;
}

IPoint3D readPosition(QLineEdit *lineEdit)
{
  std::istringstream istr(lineEdit->text().toStdString());
  std::vector<float> container;
  for (;;)
  {
    char ch;
    while (!istr.eof()&&!::isdigit(istr.peek()))
    {
      istr.get(ch);
    }
    if (istr.eof()) break;
    float label=0;
    if (istr>>label)
    {
      container.push_back(label);
    }
  }
  IPoint3D ip;
  if (container.size()>0) ip.x=container[0];
  if (container.size()>1) ip.y=container[1];
  if (container.size()>2) ip.z=container[2];
  return ip;
}

void GotoCoordinateDialog::on_coordinateLineEdit_textEdited(const QString &/*arg1*/)
{
  if (brainSuiteWindow)
  {
    IPoint3D ip=::readPosition(ui->coordinateLineEdit);
    brainSuiteWindow->mouseMoveTo(0,ip,0); //IPoint3D(x,y,z));
  }
}

void GotoCoordinateDialog::on_coordinateLineEdit_editingFinished()
{
}
