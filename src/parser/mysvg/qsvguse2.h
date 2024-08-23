// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include <private/qsvgnode_p.h>

#ifndef QSVGGRAPHICS2_P_H
#define QSVGGRAPHICS2_P_H

class QSvgUse2 : public QSvgNode
{
public:
    QSvgUse2(const QPointF &start, QSvgNode *parent, QSvgNode *link);
    QSvgUse2(const QPointF &start, QSvgNode *parent, const QString &linkId)
        : QSvgUse2(start, parent, nullptr)
    { m_linkId = linkId; }
    void drawCommand(QPainter *p, QSvgExtraStates &states) override;
    Type type() const override;
    QRectF bounds(QPainter *p, QSvgExtraStates &states) const override;
    bool isResolved() const { return m_link != nullptr; }
    QString linkId() const { return m_linkId; }
    void setLink(QSvgNode *link) { m_link = link; }
    QSvgNode *link() const { return m_link; }
    QPointF startPos() const { return m_start; }

private:
    QSvgNode *m_link;
    QPointF   m_start;
    QString   m_linkId;
    mutable bool m_recursing;
};

#endif // QSVGGRAPHICS2_P_H
