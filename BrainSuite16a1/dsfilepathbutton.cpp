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

#include "dsfilepathbutton.h"
#include <qpainter.h>
#include <qapplication.h>
#include <qstyleoption.h>

DSFilePathButton::DSFilePathButton(QWidget *parent) :
  QCommandLinkButton(parent)
{
}

void DSFilePathButton::setFlat(bool )
{
}

void DSFilePathButton::paintEvent(QPaintEvent *)
{
  QPainter painter(this);
  QStyleOptionButton option;
  option.initFrom(this);
  option.features = QStyleOptionButton::CommandLinkButton;
  style()->drawControl(QStyle::CE_PushButton, &option, &painter, this);
  painter.save();
  QRect contents = contentsRect();
  const int dx=6;
  const int dy=-2;

  int width = contents.width()-10;
  QFont font=painter.font();
  float f = font.pointSizeF();
  font.setBold(true);
  font.setPointSize(f+1.0f);
  painter.setFont(font);

  QFontMetrics fontMetrics(font);
  QString elidedFilenameText=fontMetrics.elidedText(text(),Qt::ElideRight,width);
  painter.drawText(QPoint(dx,dy+contents.height()/2+2),elidedFilenameText);
  font.setBold(false);
  font.setPointSize(f-1.0f);
  painter.setFont(font);
  QFontMetrics fontMetrics2(font);
  QString elidedText=fontMetrics2.elidedText(filepath,Qt::ElideLeft,width);
  painter.drawText(QPoint(dx,dy+contents.height()-2),elidedText);
  painter.restore();
}
