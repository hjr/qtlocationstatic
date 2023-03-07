/****************************************************************************
**
** Copyright (C) 2017 Mapbox, Inc.
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

#include "qmaplibreglstylechange_p.h"

#include <QtCore/QDebug>
#include <QtCore/QMetaProperty>
#include <QtCore/QRegularExpression>
#include <QtCore/QStringList>
#include <QtPositioning/QGeoPath>
#include <QtPositioning/QGeoPolygon>
#include <QtQml/QJSValue>
#include <QtLocation/private/qdeclarativecirclemapitem_p_p.h>

namespace {

QString getId(QDeclarativeGeoMapItemBase *mapItem)
{
    return QStringLiteral("QtLocation-") +
            ((mapItem->objectName().isEmpty()) ? QString::number(quint64(mapItem)) : mapItem->objectName());
}

// Mapbox GL supports geometry segments that spans above 180 degrees in
// longitude. To keep visual expectations in parity with Qt, we need to adapt
// the coordinates to always use the shortest path when in ambiguity.
static bool geoRectangleCrossesDateLine(const QGeoRectangle &rect) {
    return rect.topLeft().longitude() > rect.bottomRight().longitude();
}

QMapLibreGL::Feature featureFromMapRectangle(QDeclarativeRectangleMapItem *mapItem)
{
    const QGeoRectangle *rect = static_cast<const QGeoRectangle *>(&mapItem->geoShape());
    QMapLibreGL::Coordinate bottomLeft { rect->bottomLeft().latitude(), rect->bottomLeft().longitude() };
    QMapLibreGL::Coordinate topLeft { rect->topLeft().latitude(), rect->topLeft().longitude() };
    QMapLibreGL::Coordinate bottomRight { rect->bottomRight().latitude(), rect->bottomRight().longitude() };
    QMapLibreGL::Coordinate topRight { rect->topRight().latitude(), rect->topRight().longitude() };
    if (geoRectangleCrossesDateLine(*rect)) {
        bottomRight.second += 360.0;
        topRight.second += 360.0;
    }
    QMapLibreGL::CoordinatesCollections geometry { { { bottomLeft, bottomRight, topRight, topLeft, bottomLeft } } };

    return QMapLibreGL::Feature(QMapLibreGL::Feature::PolygonType, geometry, {}, getId(mapItem));
}

QMapLibreGL::Feature featureFromMapCircle(QDeclarativeCircleMapItem *mapItem)
{
    static const int circleSamples = 128;
    const QGeoProjectionWebMercator &p = static_cast<const QGeoProjectionWebMercator&>(mapItem->map()->geoProjection());
    QList<QGeoCoordinate> path;
    QGeoCoordinate leftBound;
    QDeclarativeCircleMapItemPrivate::calculatePeripheralPoints(path, mapItem->center(), mapItem->radius(), circleSamples, leftBound);
    QList<QDoubleVector2D> pathProjected;
    for (const QGeoCoordinate &c : qAsConst(path))
        pathProjected << p.geoToMapProjection(c);
    if (QDeclarativeCircleMapItemPrivateCPU::crossEarthPole(mapItem->center(), mapItem->radius()))
        QDeclarativeCircleMapItemPrivateCPU::preserveCircleGeometry(pathProjected, mapItem->center(), mapItem->radius(), p);
    path.clear();
    for (const QDoubleVector2D &c : qAsConst(pathProjected))
        path << p.mapProjectionToGeo(c);


    QMapLibreGL::Coordinates coordinates;
    for (const QGeoCoordinate &coordinate : path) {
        coordinates << QMapLibreGL::Coordinate { coordinate.latitude(), coordinate.longitude() };
    }
    coordinates.append(coordinates.first());  // closing the path
    QMapLibreGL::CoordinatesCollections geometry { { coordinates } };
    return QMapLibreGL::Feature(QMapLibreGL::Feature::PolygonType, geometry, {}, getId(mapItem));
}

static QMapLibreGL::Coordinates qgeocoordinate2mapboxcoordinate(const QList<QGeoCoordinate> &crds, const bool crossesDateline, bool closed = false)
{
    QMapLibreGL::Coordinates coordinates;
    for (const QGeoCoordinate &coordinate : crds) {
        if (!coordinates.empty() && crossesDateline && qAbs(coordinate.longitude() - coordinates.last().second) > 180.0) {
            coordinates << QMapLibreGL::Coordinate { coordinate.latitude(), coordinate.longitude() + (coordinate.longitude() >= 0 ? -360.0 : 360.0) };
        } else {
            coordinates << QMapLibreGL::Coordinate { coordinate.latitude(), coordinate.longitude() };
        }
    }
    if (closed && !coordinates.empty() && coordinates.last() != coordinates.first())
        coordinates.append(coordinates.first());  // closing the path
    return coordinates;
}

QMapLibreGL::Feature featureFromMapPolygon(QDeclarativePolygonMapItem *mapItem)
{
    const QGeoPolygon *polygon = static_cast<const QGeoPolygon *>(&mapItem->geoShape());
    const bool crossesDateline = geoRectangleCrossesDateLine(polygon->boundingGeoRectangle());
    QMapLibreGL::CoordinatesCollections geometry;
    QMapLibreGL::CoordinatesCollection poly;
    QMapLibreGL::Coordinates coordinates = qgeocoordinate2mapboxcoordinate(polygon->perimeter(), crossesDateline, true);
    poly.push_back(coordinates);
    for (int i = 0; i < polygon->holesCount(); ++i) {
        coordinates = qgeocoordinate2mapboxcoordinate(polygon->holePath(i), crossesDateline, true);
        poly.push_back(coordinates);
    }

    geometry.push_back(poly);
    return QMapLibreGL::Feature(QMapLibreGL::Feature::PolygonType, geometry, {}, getId(mapItem));
}

QMapLibreGL::Feature featureFromMapPolyline(QDeclarativePolylineMapItem *mapItem)
{
    const QGeoPath *path = static_cast<const QGeoPath *>(&mapItem->geoShape());
    QMapLibreGL::Coordinates coordinates;
    const bool crossesDateline = geoRectangleCrossesDateLine(path->boundingGeoRectangle());
    for (const QGeoCoordinate &coordinate : path->path()) {
        if (!coordinates.empty() && crossesDateline && qAbs(coordinate.longitude() - coordinates.last().second) > 180.0) {
            coordinates << QMapLibreGL::Coordinate { coordinate.latitude(), coordinate.longitude() + (coordinate.longitude() >= 0 ? -360.0 : 360.0) };
        } else {
            coordinates << QMapLibreGL::Coordinate { coordinate.latitude(), coordinate.longitude() };
        }
    }
    QMapLibreGL::CoordinatesCollections geometry { { coordinates } };

    return QMapLibreGL::Feature(QMapLibreGL::Feature::LineStringType, geometry, {}, getId(mapItem));
}

QMapLibreGL::Feature featureFromMapItem(QDeclarativeGeoMapItemBase *item)
{
    switch (item->itemType()) {
    case QGeoMap::MapRectangle:
        return featureFromMapRectangle(static_cast<QDeclarativeRectangleMapItem *>(item));
    case QGeoMap::MapCircle:
        return featureFromMapCircle(static_cast<QDeclarativeCircleMapItem *>(item));
    case QGeoMap::MapPolygon:
        return featureFromMapPolygon(static_cast<QDeclarativePolygonMapItem *>(item));
    case QGeoMap::MapPolyline:
        return featureFromMapPolyline(static_cast<QDeclarativePolylineMapItem *>(item));
    default:
        qWarning() << "Unsupported QGeoMap item type: " << item->itemType();
        return QMapLibreGL::Feature();
    }
}

} // namespace


QList<QSharedPointer<QMaplibreGLStyleChange>> QMaplibreGLStyleChange::addMapItem(QDeclarativeGeoMapItemBase *item, const QString &before)
{
    QList<QSharedPointer<QMaplibreGLStyleChange>> changes;

    switch (item->itemType()) {
    case QGeoMap::MapRectangle:
    case QGeoMap::MapCircle:
    case QGeoMap::MapPolygon:
    case QGeoMap::MapPolyline:
        break;
    default:
        qWarning() << "Unsupported QGeoMap item type: " << item->itemType();
        return changes;
    }

    QMapLibreGL::Feature feature = featureFromMapItem(item);

    changes << QMaplibreGLStyleAddLayer::fromFeature(feature, before);
    changes << QMaplibreGLStyleAddSource::fromFeature(feature);
    changes << QMaplibreGLStyleSetPaintProperty::fromMapItem(item);
    changes << QMaplibreGLStyleSetLayoutProperty::fromMapItem(item);

    return changes;
}

QList<QSharedPointer<QMaplibreGLStyleChange>> QMaplibreGLStyleChange::removeMapItem(QDeclarativeGeoMapItemBase *item)
{
    QList<QSharedPointer<QMaplibreGLStyleChange>> changes;

    const QString id = getId(item);

    changes << QSharedPointer<QMaplibreGLStyleChange>(new QMaplibreGLStyleRemoveLayer(id));
    changes << QSharedPointer<QMaplibreGLStyleChange>(new QMaplibreGLStyleRemoveSource(id));

    return changes;
}

// QMaplibreGLStyleSetLayoutProperty

void QMaplibreGLStyleSetLayoutProperty::apply(QMapLibreGL::Map *map)
{
    map->setLayoutProperty(m_layer, m_property, m_value);
}

QList<QSharedPointer<QMaplibreGLStyleChange>> QMaplibreGLStyleSetLayoutProperty::fromMapItem(QDeclarativeGeoMapItemBase *item)
{
    QList<QSharedPointer<QMaplibreGLStyleChange>> changes;

    switch (item->itemType()) {
    case QGeoMap::MapPolyline:
        changes = fromMapItem(static_cast<QDeclarativePolylineMapItem *>(item));
    default:
        break;
    }

    changes << QSharedPointer<QMaplibreGLStyleChange>(
        new QMaplibreGLStyleSetLayoutProperty(getId(item), QStringLiteral("visibility"),
            item->isVisible() ? QStringLiteral("visible") : QStringLiteral("none")));

    return changes;
}

QList<QSharedPointer<QMaplibreGLStyleChange>> QMaplibreGLStyleSetLayoutProperty::fromMapItem(QDeclarativePolylineMapItem *item)
{
    QList<QSharedPointer<QMaplibreGLStyleChange>> changes;
    changes.reserve(2);

    const QString id = getId(item);

    changes << QSharedPointer<QMaplibreGLStyleChange>(
        new QMaplibreGLStyleSetLayoutProperty(id, QStringLiteral("line-cap"), QStringLiteral("square")));
    changes << QSharedPointer<QMaplibreGLStyleChange>(
        new QMaplibreGLStyleSetLayoutProperty(id, QStringLiteral("line-join"), QStringLiteral("bevel")));

    return changes;
}

QMaplibreGLStyleSetLayoutProperty::QMaplibreGLStyleSetLayoutProperty(const QString& layer, const QString& property, const QVariant &value)
    : m_layer(layer), m_property(property), m_value(value)
{
}

// QMaplibreGLStyleSetPaintProperty

QMaplibreGLStyleSetPaintProperty::QMaplibreGLStyleSetPaintProperty(const QString& layer, const QString& property, const QVariant &value)
    : m_layer(layer), m_property(property), m_value(value)
{
}

void QMaplibreGLStyleSetPaintProperty::apply(QMapLibreGL::Map *map)
{
    map->setPaintProperty(m_layer, m_property, m_value);
}

QList<QSharedPointer<QMaplibreGLStyleChange>> QMaplibreGLStyleSetPaintProperty::fromMapItem(QDeclarativeGeoMapItemBase *item)
{
    switch (item->itemType()) {
    case QGeoMap::MapRectangle:
        return fromMapItem(static_cast<QDeclarativeRectangleMapItem *>(item));
    case QGeoMap::MapCircle:
        return fromMapItem(static_cast<QDeclarativeCircleMapItem *>(item));
    case QGeoMap::MapPolygon:
        return fromMapItem(static_cast<QDeclarativePolygonMapItem *>(item));
    case QGeoMap::MapPolyline:
        return fromMapItem(static_cast<QDeclarativePolylineMapItem *>(item));
    default:
        qWarning() << "Unsupported QGeoMap item type: " << item->itemType();
        return QList<QSharedPointer<QMaplibreGLStyleChange>>();
    }
}

QList<QSharedPointer<QMaplibreGLStyleChange>> QMaplibreGLStyleSetPaintProperty::fromMapItem(QDeclarativeRectangleMapItem *item)
{
    QList<QSharedPointer<QMaplibreGLStyleChange>> changes;
    changes.reserve(3);

    const QString id = getId(item);

    changes << QSharedPointer<QMaplibreGLStyleChange>(
        new QMaplibreGLStyleSetPaintProperty(id, QStringLiteral("fill-opacity"), item->color().alphaF() * item->mapItemOpacity()));
    changes << QSharedPointer<QMaplibreGLStyleChange>(
        new QMaplibreGLStyleSetPaintProperty(id, QStringLiteral("fill-color"), item->color()));
    changes << QSharedPointer<QMaplibreGLStyleChange>(
        new QMaplibreGLStyleSetPaintProperty(id, QStringLiteral("fill-outline-color"), item->border()->color()));

    return changes;
}

QList<QSharedPointer<QMaplibreGLStyleChange>> QMaplibreGLStyleSetPaintProperty::fromMapItem(QDeclarativeCircleMapItem *item)
{
    QList<QSharedPointer<QMaplibreGLStyleChange>> changes;
    changes.reserve(3);

    const QString id = getId(item);

    changes << QSharedPointer<QMaplibreGLStyleChange>(
        new QMaplibreGLStyleSetPaintProperty(id, QStringLiteral("fill-opacity"), item->color().alphaF() * item->mapItemOpacity()));
    changes << QSharedPointer<QMaplibreGLStyleChange>(
        new QMaplibreGLStyleSetPaintProperty(id, QStringLiteral("fill-color"), item->color()));
    changes << QSharedPointer<QMaplibreGLStyleChange>(
        new QMaplibreGLStyleSetPaintProperty(id, QStringLiteral("fill-outline-color"), item->border()->color()));

    return changes;
}

QList<QSharedPointer<QMaplibreGLStyleChange>> QMaplibreGLStyleSetPaintProperty::fromMapItem(QDeclarativePolygonMapItem *item)
{
    QList<QSharedPointer<QMaplibreGLStyleChange>> changes;
    changes.reserve(3);

    const QString id = getId(item);

    changes << QSharedPointer<QMaplibreGLStyleChange>(
        new QMaplibreGLStyleSetPaintProperty(id, QStringLiteral("fill-opacity"), item->color().alphaF() * item->mapItemOpacity()));
    changes << QSharedPointer<QMaplibreGLStyleChange>(
        new QMaplibreGLStyleSetPaintProperty(id, QStringLiteral("fill-color"), item->color()));
    changes << QSharedPointer<QMaplibreGLStyleChange>(
        new QMaplibreGLStyleSetPaintProperty(id, QStringLiteral("fill-outline-color"), item->border()->color()));

    return changes;
}

QList<QSharedPointer<QMaplibreGLStyleChange>> QMaplibreGLStyleSetPaintProperty::fromMapItem(QDeclarativePolylineMapItem *item)
{
    QList<QSharedPointer<QMaplibreGLStyleChange>> changes;
    changes.reserve(3);

    const QString id = getId(item);

    changes << QSharedPointer<QMaplibreGLStyleChange>(
        new QMaplibreGLStyleSetPaintProperty(id, QStringLiteral("line-opacity"), item->line()->color().alphaF() * item->mapItemOpacity()));
    changes << QSharedPointer<QMaplibreGLStyleChange>(
        new QMaplibreGLStyleSetPaintProperty(id, QStringLiteral("line-color"), item->line()->color()));
    changes << QSharedPointer<QMaplibreGLStyleChange>(
        new QMaplibreGLStyleSetPaintProperty(id, QStringLiteral("line-width"), item->line()->width()));

    return changes;
}

// QMaplibreGLStyleAddLayer

void QMaplibreGLStyleAddLayer::apply(QMapLibreGL::Map *map)
{
    map->addLayer(m_params, m_before);
}

QSharedPointer<QMaplibreGLStyleChange> QMaplibreGLStyleAddLayer::fromFeature(const QMapLibreGL::Feature &feature, const QString &before)
{
    auto layer = new QMaplibreGLStyleAddLayer();
    layer->m_params[QStringLiteral("id")] = feature.id;
    layer->m_params[QStringLiteral("source")] = feature.id;

    switch (feature.type) {
    case QMapLibreGL::Feature::PointType:
        layer->m_params[QStringLiteral("type")] = QStringLiteral("circle");
        break;
    case QMapLibreGL::Feature::LineStringType:
        layer->m_params[QStringLiteral("type")] = QStringLiteral("line");
        break;
    case QMapLibreGL::Feature::PolygonType:
        layer->m_params[QStringLiteral("type")] = QStringLiteral("fill");
        break;
    }

    layer->m_before = before;

    return QSharedPointer<QMaplibreGLStyleChange>(layer);
}


// QMaplibreGLStyleRemoveLayer

void QMaplibreGLStyleRemoveLayer::apply(QMapLibreGL::Map *map)
{
    map->removeLayer(m_id);
}

QMaplibreGLStyleRemoveLayer::QMaplibreGLStyleRemoveLayer(const QString &id) : m_id(id)
{
}


// QMaplibreGLStyleAddSource

void QMaplibreGLStyleAddSource::apply(QMapLibreGL::Map *map)
{
    map->updateSource(m_id, m_params);
}

QSharedPointer<QMaplibreGLStyleChange> QMaplibreGLStyleAddSource::fromFeature(const QMapLibreGL::Feature &feature)
{
    auto source = new QMaplibreGLStyleAddSource();

    source->m_id = feature.id.toString();
    source->m_params[QStringLiteral("type")] = QStringLiteral("geojson");
    source->m_params[QStringLiteral("data")] = QVariant::fromValue<QMapLibreGL::Feature>(feature);

    return QSharedPointer<QMaplibreGLStyleChange>(source);
}

QSharedPointer<QMaplibreGLStyleChange> QMaplibreGLStyleAddSource::fromMapItem(QDeclarativeGeoMapItemBase *item)
{
    return fromFeature(featureFromMapItem(item));
}


// QMaplibreGLStyleRemoveSource

void QMaplibreGLStyleRemoveSource::apply(QMapLibreGL::Map *map)
{
    map->removeSource(m_id);
}

QMaplibreGLStyleRemoveSource::QMaplibreGLStyleRemoveSource(const QString &id) : m_id(id)
{
}


// QMaplibreGLStyleSetFilter

void QMaplibreGLStyleSetFilter::apply(QMapLibreGL::Map *map)
{
    map->setFilter(m_layer, m_filter);
}

// QMaplibreGLStyleAddImage

void QMaplibreGLStyleAddImage::apply(QMapLibreGL::Map *map)
{
    map->addImage(m_name, m_sprite);
}
