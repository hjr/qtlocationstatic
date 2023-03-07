/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#include "jsonparserhelpers.h"
#include "../qplacemanagerengine_nokiav2.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QVariantMap>
#include <QtPositioning/QGeoCoordinate>
#include <QtLocation/QPlaceContentRequest>
#include <QtLocation/QPlaceIcon>
#include <QtLocation/QPlaceSupplier>
#include <QtLocation/QPlaceContent>
#include <QtLocation/QPlaceUser>
#include <QtLocation/QPlaceContactDetail>
#include <QtLocation/QPlaceCategory>

QT_BEGIN_NAMESPACE

QGeoCoordinate parseCoordinate(const QJsonArray &coordinateArray)
{
    return QGeoCoordinate(coordinateArray.at(0).toDouble(), coordinateArray.at(1).toDouble());
}

QPlaceSupplier parseSupplier(const QJsonObject &supplierObject,
                             const QPlaceManagerEngineNokiaV2 *engine)
{
    Q_ASSERT(engine);

    QPlaceSupplier supplier;
    supplier.setName(supplierObject.value(QStringLiteral("title")).toString());
    supplier.setUrl(supplierObject.value(QStringLiteral("href")).toString());

    supplier.setIcon(engine->icon(supplierObject.value(QStringLiteral("icon")).toString()));

    return supplier;
}

QPlaceCategory parseCategory(const QJsonObject &categoryObject,
                             const QPlaceManagerEngineNokiaV2 *engine)
{
    Q_ASSERT(engine);

    QPlaceCategory category;

    category.setName(categoryObject.value(QStringLiteral("title")).toString());

    const QUrl href(categoryObject.value(QStringLiteral("href")).toString());
    const QString hrefPath(href.path());
    category.setCategoryId(hrefPath.mid(hrefPath.lastIndexOf(QLatin1Char('/')) + 1));


    category.setIcon(engine->icon(categoryObject.value(QStringLiteral("icon")).toString()));
    return category;
}

QList<QPlaceCategory> parseCategories(const QJsonArray &categoryArray,
                                     const QPlaceManagerEngineNokiaV2 *engine)
{
    Q_ASSERT(engine);

    QList<QPlaceCategory> categoryList;
    for (int i = 0; i < categoryArray.count(); ++i)
        categoryList.append(parseCategory(categoryArray.at(i).toObject(),
                                          engine));

    return categoryList;
}

QList<QPlaceContactDetail> parseContactDetails(const QJsonArray &contacts)
{
    QList<QPlaceContactDetail> contactDetails;

    for (int i = 0; i < contacts.count(); ++i) {
        QJsonObject contact = contacts.at(i).toObject();

        QPlaceContactDetail detail;
        detail.setLabel(contact.value(QStringLiteral("label")).toString());
        detail.setValue(contact.value(QStringLiteral("value")).toString());

        contactDetails.append(detail);
    }

    return contactDetails;
}

QPlaceContent parseImage(const QJsonObject &imageObject,
                         const QPlaceManagerEngineNokiaV2 *engine)
{
    Q_ASSERT(engine);

    QPlaceContent image(QPlaceContent::ImageType);

    image.setValue(QPlaceContent::ContentAttribution, imageObject.value(
                   QStringLiteral("attribution")).toString());
    image.setValue(QPlaceContent::ImageUrl, imageObject.value(
                   QStringLiteral("src")).toString());
    image.setValue(QPlaceContent::ContentSupplier, QVariant::fromValue(parseSupplier(
                   imageObject.value(QStringLiteral("supplier")).toObject(), engine)));

    return image;
}

QPlaceContent parseReview(const QJsonObject &reviewObject,
                          const QPlaceManagerEngineNokiaV2 *engine)
{
    Q_ASSERT(engine);

    QPlaceContent review(QPlaceContent::ReviewType);

    review.setValue(QPlaceContent::ReviewDateTime, QDateTime::fromString(
                    reviewObject.value(QStringLiteral("date")).toString()));

    if (reviewObject.contains(QStringLiteral("title"))) {
        review.setValue(QPlaceContent::ReviewTitle, reviewObject.value(
                        QStringLiteral("title")).toString());
    }

    if (reviewObject.contains(QStringLiteral("rating"))) {
        review.setValue(QPlaceContent::ReviewRating, reviewObject.value(
                        QStringLiteral("rating")).toDouble());
    }

    review.setValue(QPlaceContent::ReviewText, reviewObject.value(
                    QStringLiteral("description")).toString());

    QJsonObject userObject = reviewObject.value(QStringLiteral("user")).toObject();

    QPlaceUser user;
    user.setUserId(userObject.value(QStringLiteral("id")).toString());
    user.setName(userObject.value(QStringLiteral("title")).toString());
    review.setValue(QPlaceContent::ContentUser, QVariant::fromValue(user));

    review.setValue(QPlaceContent::ContentAttribution, reviewObject.value(
                    QStringLiteral("attribution")).toString());

    review.setValue(QPlaceContent::ReviewLanguage, reviewObject.value(
                    QStringLiteral("language")).toString());

    review.setValue(QPlaceContent::ContentSupplier, QVariant::fromValue(parseSupplier(
                    reviewObject.value(QStringLiteral("supplier")).toObject(), engine)));

    //if (reviewObject.contains(QStringLiteral("via"))) {
    //    QJsonObject viaObject = reviewObject.value(QStringLiteral("via")).toObject();
    //}

    return review;
}

QPlaceContent parseEditorial(const QJsonObject &editorialObject,
                             const QPlaceManagerEngineNokiaV2 *engine)
{
    Q_ASSERT(engine);

    QPlaceContent editorial(QPlaceContent::EditorialType);

    editorial.setValue(QPlaceContent::ContentAttribution, editorialObject.value(QStringLiteral("attribution")).toString());

    //if (editorialObject.contains(QStringLiteral("via"))) {
    //    QJsonObject viaObject = editorialObject.value(QStringLiteral("via")).toObject();
    //}

    editorial.setValue(QPlaceContent::ContentSupplier, QVariant::fromValue(parseSupplier(
                       editorialObject.value(QStringLiteral("supplier")).toObject(), engine)));
    editorial.setValue(QPlaceContent::EditorialLanguage, editorialObject.value(
                       QStringLiteral("language")).toString());
    editorial.setValue(QPlaceContent::EditorialText, editorialObject.value(
                       QStringLiteral("description")).toString());

    return editorial;
}

void parseCollection(QPlaceContent::Type type, const QJsonObject &object,
                     QPlaceContent::Collection *collection, int *totalCount,
                     QPlaceContentRequest *previous, QPlaceContentRequest *next,
                     const QPlaceManagerEngineNokiaV2 *engine)
{
    Q_ASSERT(engine);

    if (totalCount)
        *totalCount = object.value(QStringLiteral("available")).toDouble();

    int offset = 0;
    if (object.contains(QStringLiteral("offset")))
        offset = object.value(QStringLiteral("offset")).toDouble();

    if (previous && object.contains(QStringLiteral("previous"))) {
        previous->setContentType(type);
        previous->setContentContext(QUrl(object.value(QStringLiteral("previous")).toString()));
    }

    if (next && object.contains(QStringLiteral("next"))) {
        next->setContentType(type);
        next->setContentContext(QUrl(object.value(QStringLiteral("next")).toString()));
    }

    if (collection) {
        QJsonArray items = object.value(QStringLiteral("items")).toArray();
        for (int i = 0; i < items.count(); ++i) {
            QJsonObject itemObject = items.at(i).toObject();

            switch (type) {
            case QPlaceContent::ImageType:
                collection->insert(offset + i, parseImage(itemObject, engine));
                break;
            case QPlaceContent::ReviewType:
                collection->insert(offset + i, parseReview(itemObject, engine));
                break;
            case QPlaceContent::EditorialType:
                collection->insert(offset + i, parseEditorial(itemObject, engine));
                break;
            case QPlaceContent::NoType:
            default:
                break;
            }
        }
    }
}

QT_END_NAMESPACE
