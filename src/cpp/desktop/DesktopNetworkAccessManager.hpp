/*
 * DesktopNetworkAccessManager.hpp
 *
 * Copyright (C) 2009-17 by RStudio, Inc.
 *
 * Unless you have received this program directly from RStudio pursuant
 * to the terms of a commercial license agreement with RStudio, then
 * this program is licensed to you under the terms of version 3 of the
 * GNU Affero General Public License. This program is distributed WITHOUT
 * ANY EXPRESS OR IMPLIED WARRANTY, INCLUDING THOSE OF NON-INFRINGEMENT,
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. Please refer to the
 * AGPL (http://www.gnu.org/licenses/agpl-3.0.txt) for more details.
 *
 */

#ifndef DESKTOP_NETWORK_ACCESS_MANAGER_HPP
#define DESKTOP_NETWORK_ACCESS_MANAGER_HPP

#include <QObject>
#include <QtNetwork>

class NetworkAccessManager : public QNetworkAccessManager
{
    Q_OBJECT
public:
    explicit NetworkAccessManager(QString secret,
                                  QObject *parent = nullptr);

private slots:
   void pollForIO();

protected:
    QNetworkReply* createRequest(Operation op,
                                 const QNetworkRequest& req,
                                 QIODevice* outgoingData = nullptr) override;

private:
    QString secret_;
};

#endif // DESKTOP_NETWORK_ACCESS_MANAGER_HPP
