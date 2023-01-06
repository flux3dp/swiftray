/**
 * @file qxpotrace.h
 * @author Cristian Pallarés (cristian@pallares.io)
 * @brief origin repo: https://github.com/skyrpex/QxPotrace
 *        this file is modified by FLUX Inc. to meet our needs
 * @version 0.1
 * @date 2022-11-09
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#ifndef QXPOTRACE_H
#define QXPOTRACE_H

#include <QImage>
#include <QList>
#include <QPolygonF>
#include <QPainterPath>
#include <memory>

class QxPotraceImpl;

class QxPotrace
{
public:

  QxPotrace();

  bool trace(const QImage &image, int low_thres, int high_thres,
             int turd_size, qreal smooth, qreal curve_tolerance);
  QPainterPath getContours();

private:
  std::shared_ptr<QxPotraceImpl> impl_;
};

#endif // QXPOTRACE_H
