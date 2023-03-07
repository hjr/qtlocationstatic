/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtLocation module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QTILEPROVIDEROSM_H
#define QTILEPROVIDEROSM_H

#include <QtLocation/private/qgeomaptype_p.h>
#include <QtLocation/private/qgeocameracapabilities_p.h>
#include <QtCore/QUrl>
#include <QtCore/QList>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtCore/QPointer>
#include <QTimer>
#include <algorithm>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QDateTime>

QT_BEGIN_NAMESPACE

class TileProvider: public QObject
{
    Q_OBJECT
public:
    enum Status {Idle,
                 Resolving,
                 Valid,
                 Invalid };

    TileProvider();
    // "Online" constructor. Needs resolution to fetch the parameters
    TileProvider(const QUrl &urlRedirector, bool highDpi = false);
    // Offline constructor. Doesn't need URLRedirector and networkmanager
    TileProvider(const QString &urlTemplate,
             const QString &format,
             const QString &copyRightMap,
             const QString &copyRightData,
             bool highDpi = false,
             int minimumZoomLevel = 0,
             int maximumZoomLevel = 19);

    ~TileProvider();
    void setNetworkManager(QNetworkAccessManager *nm);

    void resolveProvider();
    void handleError(QNetworkReply::NetworkError error);
    void setupProvider();

    inline bool isValid() const;
    inline bool isInvalid() const;
    inline bool isResolved() const;
    inline Status status() const;

    inline QString mapCopyRight() const;
    inline QString dataCopyRight() const;
    inline QString styleCopyRight() const;
    inline QString format() const;
    inline int minimumZoomLevel() const;
    inline int maximumZoomLevel() const;
    inline const QDateTime &timestamp() const;
    inline bool isHighDpi() const;
    inline bool isHTTPS() const;
    QUrl tileAddress(int x, int y, int z) const;

    // Optional properties, not needed to construct a provider
    void setStyleCopyRight(const QString &copyright);
    void setTimestamp(const QDateTime &timestamp);

    Status m_status;
    QUrl m_urlRedirector; // The URL from where to fetch the URL template in case of a provider to resolve.
    QNetworkAccessManager *m_nm;
    QString m_urlTemplate;
    QString m_format;
    QString m_copyRightMap;
    QString m_copyRightData;
    QString m_copyRightStyle;
    QString m_urlPrefix;
    QString m_urlSuffix;
    int m_minimumZoomLevel;
    int m_maximumZoomLevel;
    QDateTime m_timestamp;
    bool m_highDpi;

    int paramsLUT[3];  //Lookup table to handle possibly shuffled x,y,z
    QString paramsSep[2]; // what goes in between %x, %y and %z

Q_SIGNALS:
    void resolutionFinished(TileProvider *provider);
    void resolutionError(TileProvider *provider);

public Q_SLOTS:
    void onNetworkReplyFinished();
    void onNetworkReplyError(QNetworkReply::NetworkError error);

friend class QGeoTileProviderOsm;
};

class QGeoTileProviderOsm: public QObject
{
    Q_OBJECT

    friend class QGeoTileFetcherOsm;
    friend class QGeoMapReplyOsm;
    friend class QGeoTiledMappingManagerEngineOsm;
public:
    enum Status {Idle,
                 Resolving,
                 Resolved };

    QGeoTileProviderOsm(QNetworkAccessManager *nm, const QGeoMapType &mapType,
                        const QList<TileProvider *> &providers,
                        const QGeoCameraCapabilities &cameraCapabilities);
    ~QGeoTileProviderOsm();

    QUrl tileAddress(int x, int y, int z) const;
    QString mapCopyRight() const;
    QString dataCopyRight() const;
    QString styleCopyRight() const;
    QString format() const;
    int minimumZoomLevel() const;
    int maximumZoomLevel() const;
    bool isHighDpi() const;
    const QGeoMapType &mapType() const;
    bool isValid() const;
    bool isResolved() const;
    QDateTime timestamp() const;
    QGeoCameraCapabilities cameraCapabilities() const;

Q_SIGNALS:
    void resolutionFinished(const QGeoTileProviderOsm *provider);
    void resolutionError(const QGeoTileProviderOsm *provider);
    void resolutionRequired();

public Q_SLOTS:
    void resolveProvider();
    void disableRedirection();

protected Q_SLOTS:
    void onResolutionFinished(TileProvider *provider);
    void onResolutionError(TileProvider *provider);
    void updateCameraCapabilities();

protected:
    void addProvider(TileProvider *provider);

/* Data members */

    QNetworkAccessManager *m_nm;
    QList<TileProvider *> m_providerList;
    TileProvider *m_provider;
    int m_providerId;
    QGeoMapType m_mapType;
    Status m_status;
    QGeoCameraCapabilities m_cameraCapabilities;
};

QT_END_NAMESPACE

#endif // QTILEPROVIDEROSM_H
