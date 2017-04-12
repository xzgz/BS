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

#include "dsfilelabel.h"
#include <qpainter.h>
#include <qevent.h>
#include <qurl.h>
#include <iostream>
#include <qfontmetrics.h>
#include <qdir.h>
#include <qmimedata.h>

DSFileLabel::DSFileLabel(QWidget *parent) : QLabel(parent)
{
  setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
}

QSize DSFileLabel::sizeHint() const
{
  return QSize(8,15);
}

void DSFileLabel::paintEvent(QPaintEvent *event)
{
  QString s = QDir::toNativeSeparators(text());
  setText(fontMetrics().elidedText(text(),Qt::ElideLeft,contentsRect().width()-5));
  setToolTip(s);
  QLabel::paintEvent(event);
  setText(s);
}

void DSFileLabel::dragEnterEvent(QDragEnterEvent *event)
{
  if (event->mimeData()->hasFormat("text/uri-list"))
  {
    event->acceptProposedAction();
  }
}

void DSFileLabel::dropEvent(QDropEvent *event)
{
  if (event->mimeData()->hasUrls())
  {
    QList<QUrl> list = event->mimeData()->urls();
    for (QList<QUrl>::iterator i=list.begin(); i!=list.end();i++)
    {
      QUrl url=*i;
      emit fileDropped(url.toLocalFile());
    }
  }
}

