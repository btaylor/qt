/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qmlgraphicspainteditem_p.h"
#include "qmlgraphicspainteditem_p_p.h"

#include <QDebug>
#include <QPen>
#include <QFile>
#include <QEvent>
#include <QApplication>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPaintEngine>
#include <qmath.h>

QT_BEGIN_NAMESPACE

/*!
    \class QmlGraphicsPaintedItem
    \brief The QmlGraphicsPaintedItem class is an abstract base class for QmlView items that want cached painting.
    \internal

    This is a convenience class for implementing items that paint their contents
    using a QPainter.  The contents of the item are cached behind the scenes.
    The dirtyCache() function should be called if the contents change to
    ensure the cache is refreshed the next time painting occurs.

    To subclass QmlGraphicsPaintedItem, you must reimplement drawContents() to draw
    the contents of the item.
*/

/*!
    \fn void QmlGraphicsPaintedItem::drawContents(QPainter *painter, const QRect &rect)

    This function is called when the cache needs to be refreshed. When
    sub-classing QmlGraphicsPaintedItem this function should be implemented so as to
    paint the contents of the item using the given \a painter for the
    area of the contents specified by \a rect.
*/

/*!
    \property QmlGraphicsPaintedItem::contentsSize
    \brief The size of the contents

    The contents size is the size of the item in regards to how it is painted
    using the drawContents() function.  This is distinct from the size of the
    item in regards to height() and width().
*/

// XXX bug in WebKit - can call repaintRequested and other cache-changing functions from within render!
static int inpaint=0;
static int inpaint_clearcache=0;

/*!
    Marks areas of the cache that intersect with the given \a rect as dirty and
    in need of being refreshed.

    \sa clearCache()
*/
void QmlGraphicsPaintedItem::dirtyCache(const QRect& rect)
{
    Q_D(QmlGraphicsPaintedItem);
    QRect srect(qCeil(rect.x()*d->contentsScale),
            qCeil(rect.y()*d->contentsScale),
            qCeil(rect.width()*d->contentsScale),
            qCeil(rect.height()*d->contentsScale));
    for (int i=0; i < d->imagecache.count(); ) {
        QmlGraphicsPaintedItemPrivate::ImageCacheItem *c = d->imagecache[i];
        QRect isect = (c->area & srect) | c->dirty;
        if (isect == c->area && !inpaint) {
            delete d->imagecache.takeAt(i);
        } else {
            c->dirty = isect;
            ++i;
        }
    }
}

/*!
    Marks the entirety of the contents cache as dirty.

    \sa dirtyCache()
*/
void QmlGraphicsPaintedItem::clearCache()
{
    if (inpaint) {
        inpaint_clearcache=1;
        return;
    }
    Q_D(QmlGraphicsPaintedItem);
    qDeleteAll(d->imagecache);
    d->imagecache.clear();
}

/*!
    Returns the size of the contents.

    \sa setContentsSize()
*/
QSize QmlGraphicsPaintedItem::contentsSize() const
{
    Q_D(const QmlGraphicsPaintedItem);
    return d->contentsSize;
}

/*!
    Sets the size of the contents to the given \a size.

    \sa contentsSize()
*/
void QmlGraphicsPaintedItem::setContentsSize(const QSize &size)
{
    Q_D(QmlGraphicsPaintedItem);
    if (d->contentsSize == size) return;
    d->contentsSize = size;
    setImplicitWidth(size.width()*d->contentsScale);
    setImplicitHeight(size.height()*d->contentsScale);
    clearCache();
    update();
    emit contentsSizeChanged();
}

qreal QmlGraphicsPaintedItem::contentsScale() const
{
    Q_D(const QmlGraphicsPaintedItem);
    return d->contentsScale;
}

void QmlGraphicsPaintedItem::setContentsScale(qreal scale)
{
    Q_D(QmlGraphicsPaintedItem);
    if (d->contentsScale == scale) return;
    d->contentsScale = scale;
    setImplicitWidth(d->contentsSize.width()*scale);
    setImplicitHeight(d->contentsSize.height()*scale);
    clearCache();
    update();
    emit contentsScaleChanged();
}


/*!
    Constructs a new QmlGraphicsPaintedItem with the given \a parent.
*/
QmlGraphicsPaintedItem::QmlGraphicsPaintedItem(QmlGraphicsItem *parent)
  : QmlGraphicsItem(*(new QmlGraphicsPaintedItemPrivate), parent)
{
    init();
}

/*!
    \internal
    Constructs a new QmlGraphicsPaintedItem with the given \a parent and
    initialized private data member \a dd.
*/
QmlGraphicsPaintedItem::QmlGraphicsPaintedItem(QmlGraphicsPaintedItemPrivate &dd, QmlGraphicsItem *parent)
  : QmlGraphicsItem(dd, parent)
{
    init();
}

/*!
    Destroys the image item.
*/
QmlGraphicsPaintedItem::~QmlGraphicsPaintedItem()
{
    clearCache();
}

/*!
    \internal
*/
void QmlGraphicsPaintedItem::init()
{
    connect(this,SIGNAL(widthChanged()),this,SLOT(clearCache()));
    connect(this,SIGNAL(heightChanged()),this,SLOT(clearCache()));
    connect(this,SIGNAL(visibleChanged()),this,SLOT(clearCache()));
}

void QmlGraphicsPaintedItem::setCacheFrozen(bool frozen)
{
    Q_D(QmlGraphicsPaintedItem);
    if (d->cachefrozen == frozen)
        return;
    d->cachefrozen = frozen;
    // XXX clear cache?
}

/*!
    \reimp
*/
void QmlGraphicsPaintedItem::paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *)
{
    Q_D(QmlGraphicsPaintedItem);
    const QRect content(0,0,qCeil(d->contentsSize.width()*d->contentsScale),
                            qCeil(d->contentsSize.height()*d->contentsScale));
    if (content.width() <= 0 || content.height() <= 0)
        return;

    ++inpaint;

    const QTransform &x = p->deviceTransform();
    QTransform xinv = x.inverted();
    QRegion effectiveClip;
    QRegion sysClip = p->paintEngine()->systemClip();
    if (xinv.type() <= QTransform::TxScale && sysClip.numRects() < 5) {
        // simple transform, region gets no more complicated...
        effectiveClip = xinv.map(sysClip);
    } else {
        // do not make complicated regions...
        effectiveClip = xinv.mapRect(sysClip.boundingRect());
    }

    QRegion topaint = p->clipRegion();
    if (topaint.isEmpty()) {
        if (effectiveClip.isEmpty())
            topaint = QRect(0,0,p->device()->width(),p->device()->height());
        else
            topaint = effectiveClip;
    } else if (!effectiveClip.isEmpty()) {
        topaint &= effectiveClip;
    }

    topaint &= content;
    QRegion uncached(content);
    p->setRenderHints(QPainter::SmoothPixmapTransform, d->smooth);

    int cachesize=0;
    for (int i=0; i<d->imagecache.count(); ++i) {
        QRect area = d->imagecache[i]->area;
        if (topaint.contains(area)) {
            QRectF target(area.x(), area.y(), area.width(), area.height());
            if (!d->cachefrozen) {
                if (!d->imagecache[i]->dirty.isNull() && topaint.contains(d->imagecache[i]->dirty)) {
                    QPainter qp(&d->imagecache[i]->image);
                    qp.setRenderHints(QPainter::HighQualityAntialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform, d->smoothCache);
                    qp.translate(-area.x(), -area.y());
                    qp.scale(d->contentsScale,d->contentsScale);
                    QRect clip = d->imagecache[i]->dirty;
                    QRect sclip(qFloor(clip.x()/d->contentsScale),
                            qFloor(clip.y()/d->contentsScale),
                            qCeil(clip.width()/d->contentsScale+clip.x()/d->contentsScale-qFloor(clip.x()/d->contentsScale)),
                            qCeil(clip.height()/d->contentsScale+clip.y()/d->contentsScale-qFloor(clip.y()/d->contentsScale)));
                    qp.setClipRect(sclip);
                    if (d->fillColor.isValid()){
                        if(d->fillColor.alpha() < 255){
                            // ### Might not work outside of raster paintengine
                            QPainter::CompositionMode prev = qp.compositionMode();
                            qp.setCompositionMode(QPainter::CompositionMode_Source);
                            qp.fillRect(sclip,d->fillColor);
                            qp.setCompositionMode(prev);
                        }else{
                            qp.fillRect(sclip,d->fillColor);
                        }
                    }
                    drawContents(&qp, sclip);
                    d->imagecache[i]->dirty = QRect();
                }
            }
            p->drawPixmap(target.toRect(), d->imagecache[i]->image);
            topaint -= area;
            d->imagecache[i]->age=0;
        } else {
            d->imagecache[i]->age++;
        }
        cachesize += area.width()*area.height();
        uncached -= area;
    }

    if (!topaint.isEmpty()) {
        if (!d->cachefrozen) {
            // Find a sensible larger area, otherwise will paint lots of tiny images.
            QRect biggerrect = topaint.boundingRect().adjusted(-64,-64,128,128);
            cachesize += biggerrect.width() * biggerrect.height();
            while (d->imagecache.count() && cachesize > d->max_imagecache_size) {
                int oldest=-1;
                int age=-1;
                for (int i=0; i<d->imagecache.count(); ++i) {
                    int a = d->imagecache[i]->age;
                    if (a > age) {
                        oldest = i;
                        age = a;
                    }
                }
                cachesize -= d->imagecache[oldest]->area.width()*d->imagecache[oldest]->area.height();
                uncached += d->imagecache[oldest]->area;
                delete d->imagecache.takeAt(oldest);
            }
            const QRegion bigger = QRegion(biggerrect) & uncached;
            const QVector<QRect> rects = bigger.rects();
            for (int i = 0; i < rects.count(); ++i) {
                const QRect &r = rects.at(i);
                QPixmap img(r.size());
                if (d->fillColor.isValid())
                    img.fill(d->fillColor);
                {
                    QPainter qp(&img);
                    qp.setRenderHints(QPainter::HighQualityAntialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform, d->smoothCache);

                    qp.translate(-r.x(),-r.y());
                    qp.scale(d->contentsScale,d->contentsScale);
                    QRect sclip(qFloor(r.x()/d->contentsScale),
                            qFloor(r.y()/d->contentsScale),
                            qCeil(r.width()/d->contentsScale+r.x()/d->contentsScale-qFloor(r.x()/d->contentsScale)),
                            qCeil(r.height()/d->contentsScale+r.y()/d->contentsScale-qFloor(r.y()/d->contentsScale)));
                    drawContents(&qp, sclip);
                }
                QmlGraphicsPaintedItemPrivate::ImageCacheItem *newitem = new QmlGraphicsPaintedItemPrivate::ImageCacheItem;
                newitem->area = r;
                newitem->image = img;
                d->imagecache.append(newitem);
                p->drawPixmap(r, newitem->image);
            }
        } else {
            const QVector<QRect> rects = uncached.rects();
            for (int i = 0; i < rects.count(); ++i)
                p->fillRect(rects.at(i), Qt::lightGray);
        }
    }

    if (inpaint_clearcache) {
        clearCache();
        inpaint_clearcache = 0;
    }

    --inpaint;
}

/*!
  \qmlproperty int PaintedItem::pixelCacheSize

  This property holds the maximum number of pixels of image cache to
  allow. The default is 0.1 megapixels. The cache will not be larger
  than the (unscaled) size of the WebView.
*/
/*!
  \property QmlGraphicsPaintedItem::pixelCacheSize

  The maximum number of pixels of image cache to allow. The default
  is 0.1 megapixels. The cache will not be larger than the (unscaled)
  size of the QmlGraphicsPaintedItem.
*/
int QmlGraphicsPaintedItem::pixelCacheSize() const
{
    Q_D(const QmlGraphicsPaintedItem);
    return d->max_imagecache_size;
}

void QmlGraphicsPaintedItem::setPixelCacheSize(int pixels)
{
    Q_D(QmlGraphicsPaintedItem);
    if (pixels < d->max_imagecache_size) {
        int cachesize=0;
        for (int i=0; i<d->imagecache.count(); ++i) {
            QRect area = d->imagecache[i]->area;
            cachesize += area.width()*area.height();
        }
        while (d->imagecache.count() && cachesize > pixels) {
            int oldest=-1;
            int age=-1;
            for (int i=0; i<d->imagecache.count(); ++i) {
                int a = d->imagecache[i]->age;
                if (a > age) {
                    oldest = i;
                    age = a;
                }
            }
            cachesize -= d->imagecache[oldest]->area.width()*d->imagecache[oldest]->area.height();
            delete d->imagecache.takeAt(oldest);
        }
    }
    d->max_imagecache_size = pixels;
}



/*!
    \property QmlGraphicsPaintedItem::fillColor

    The color to be used to fill the item prior to calling drawContents().
    By default, this is Qt::transparent.

    Performance improvements can be achieved if subclasses call this with either an
    invalid color (QColor()), or an appropriate solid color.
*/
void QmlGraphicsPaintedItem::setFillColor(const QColor& c)
{
    Q_D(QmlGraphicsPaintedItem);
    if (d->fillColor == c)
        return;
    d->fillColor = c;
    emit fillColorChanged();
    update();
}

QColor QmlGraphicsPaintedItem::fillColor() const
{
    Q_D(const QmlGraphicsPaintedItem);
    return d->fillColor;
}

/*!
    \qmlproperty bool PaintedItem::smoothCache

    Controls whether the cached tiles of which the item is composed are
    rendered smoothly when they are generated.

    This is in addition toe Item::smooth, which controls the smooth painting of
    the already-painted cached tiles under transformation.
*/
bool QmlGraphicsPaintedItem::smoothCache() const
{
    Q_D(const QmlGraphicsPaintedItem);
    return d->smoothCache;
}

void QmlGraphicsPaintedItem::setSmoothCache(bool on)
{
    Q_D(QmlGraphicsPaintedItem);
    if (d->smoothCache != on) {
        d->smoothCache = on;
        clearCache();
    }
}


QT_END_NAMESPACE