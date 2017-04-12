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

#include <filedownloader.h>
#include <iostream>

FileDownloader::FileDownloader(QUrl imageUrl, QObject *parent) :
  QObject(parent)
{
  connect(&m_WebCtrl, SIGNAL(finished(QNetworkReply*)),
          SLOT(fileDownloaded(QNetworkReply*)));
  QNetworkRequest request(imageUrl);
  m_WebCtrl.get(request);
}

FileDownloader::~FileDownloader()
{
}

void FileDownloader::fileDownloaded(QNetworkReply* pReply)
{
  m_DownloadedData = pReply->readAll();
//emit a signal
  networkError = pReply->error();
  if (networkError!=QNetworkReply::NoError)
  {
    errorString=pReply->errorString();
  }
  pReply->deleteLater();
  emit downloaded();
}

QByteArray FileDownloader::downloadedData() const
{
  return m_DownloadedData;
}

