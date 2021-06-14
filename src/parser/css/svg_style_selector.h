#ifndef VECTY_SVG_STYLE_SELECTOR_H
#define VECTY_SVG_STYLE_SELECTOR_H

#include <shape/shape.h>
#include <QtGui/private/qcssparser_p.h>

class SVGNode {
public:
  QString id_;
  QString class_name_;
  QString name_;
  SVGNode *parent; // todo
  SVGNode *prev; // todo

  SVGNode(QString name, QString id, QString class_name) {
    name_ = name;
    id_ = id;
    class_name_ = class_name;
    parent = nullptr;
    prev = nullptr;
  }
};

class SVGStyleSelector : public QCss::StyleSelector {
public:
  SVGStyleSelector() {}

  virtual ~SVGStyleSelector() {}

  inline SVGNode *getSVGNode(NodePtr node) const {
    return static_cast<SVGNode *>(node.ptr);
  }

  QString getNodeId(SVGNode *node) const {
    return node->id_;
  }

  QString getNodeClass(SVGNode *node) const {
    return node->class_name_;
  }

  virtual bool nodeNameEquals(NodePtr node, const QString &nodeName) const {
    SVGNode *n = getSVGNode(node);
    if (!n) return false;
    QString name = n->name_;
    return QString::compare(name, nodeName, Qt::CaseInsensitive) == 0;
  }

  virtual QString attribute(NodePtr node, const QString &name) const {
    SVGNode *n = getSVGNode(node);
    if ((!getNodeId(n).isEmpty() && (name == QLatin1String("id") ||
                                     name == QLatin1String("xml:id"))))
      return getNodeId(n);
    if (!getNodeClass(n).isEmpty() && name == QLatin1String("class"))
      return getNodeClass(n);
    return QString();
  }

  virtual bool hasAttributes(NodePtr node) const {
    SVGNode *n = getSVGNode(node);
    return (n &&
            (!getNodeId(n).isEmpty() || !getNodeClass(n).isEmpty()));
  }

  virtual QStringList nodeIds(NodePtr node) const {
    SVGNode *n = getSVGNode(node);
    QString nid;
    if (n)
      nid = getNodeId(n);
    QStringList lst;
    lst.append(nid);
    return lst;
  }

  virtual QStringList nodeNames(NodePtr node) const {
    SVGNode *n = getSVGNode(node);
    if (n)
      return QStringList(n->name_);
    return QStringList();
  }

  virtual bool isNullNode(NodePtr node) const {
    return !node.ptr;
  }

  virtual NodePtr parentNode(NodePtr node) const {
    SVGNode *n = getSVGNode(node);
    NodePtr newNode;
    newNode.ptr = 0;
    newNode.id = 0;
    if (n) {
      SVGNode *svgParent = n->parent;
      if (svgParent) {
        newNode.ptr = svgParent;
      }
    }
    return newNode;
  }

  virtual NodePtr previousSiblingNode(NodePtr node) const {
    NodePtr newNode;
    newNode.ptr = 0;
    newNode.id = 0;

    SVGNode *n = getSVGNode(node);
    if (!n)
      return newNode;

    newNode.ptr = getSVGNode(node)->prev;
    return newNode;
  }

  virtual NodePtr duplicateNode(NodePtr node) const {
    NodePtr n;
    n.ptr = node.ptr;
    n.id = node.id;
    return n;
  }

  virtual void freeNode(NodePtr node) const {
    Q_UNUSED(node);
  }
};

#endif //VECTY_SVG_STYLE_SELECTOR_H
