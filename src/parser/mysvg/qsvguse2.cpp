// This file is based on QT 6.7.2 qsvg_hander in QtSVG module.

// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsvguse2.h"
#include <private/qsvgstructure_p.h>
#include <private/qsvgfont_p.h>

#include <qabstracttextdocumentlayout.h>
#include <qdebug.h>
#include <qloggingcategory.h>
#include <qpainter.h>
#include <qscopedvaluerollback.h>
#include <qtextcursor.h>
#include <qtextdocument.h>
#include <private/qfixed_p.h>

#include <QElapsedTimer>
#include <QLoggingCategory>

#include <math.h>
#include <limits.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcSvgDraw, "qt.svg.draw")

QSvgUse2::QSvgUse2(const QPointF &start, QSvgNode *parent, QSvgNode *node)
    : QSvgNode(parent), m_link(node), m_start(start), m_recursing(false)
{
    m_linkId = node->nodeId();
}

void QSvgUse2::drawCommand(QPainter *p, QSvgExtraStates &states)
{
    if (Q_UNLIKELY(!m_link || isDescendantOf(m_link) || m_recursing))
        return;

    Q_ASSERT(states.nestedUseCount == 0 || states.nestedUseLevel > 0);
    if (states.nestedUseLevel > 3 && states.nestedUseCount > (256 + states.nestedUseLevel * 2)) {
        qCDebug(lcSvgDraw, "Too many nested use nodes at #%s!", qPrintable(m_linkId));
        return;
    }

    QScopedValueRollback<bool> inUseGuard(states.inUse, true);

    if (!m_start.isNull()) {
        p->translate(m_start);
    }
    if (states.nestedUseLevel > 0)
        ++states.nestedUseCount;
    {
        QScopedValueRollback<int> useLevelGuard(states.nestedUseLevel, states.nestedUseLevel + 1);
        QScopedValueRollback<bool> recursingGuard(m_recursing, true);
        m_link->draw(p, states);
    }
    if (states.nestedUseLevel == 0)
        states.nestedUseCount = 0;

    if (!m_start.isNull()) {
        p->translate(-m_start);
    }
}

QSvgNode::Type QSvgUse2::type() const
{
    return Use;
}

QRectF QSvgUse2::bounds(QPainter *p, QSvgExtraStates &states) const
{
    QRectF bounds;
    if (Q_LIKELY(m_link && !isDescendantOf(m_link) && !m_recursing)) {
        QScopedValueRollback<bool> guard(m_recursing, true);
        p->translate(m_start);
        bounds = m_link->transformedBounds(p, states);
        p->translate(-m_start);
    }
    return bounds;
}

QT_END_NAMESPACE
