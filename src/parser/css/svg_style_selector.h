#ifndef VECTY_SVG_STYLE_SELECTOR_H
#define VECTY_SVG_STYLE_SELECTOR_H

#include <QtGui/private/qcssparser_p.h>

class SVGStyleSelector : public QCss::StyleSelector {
public:
  SVGStyleSelector() {}

  virtual ~SVGStyleSelector() {}

  inline xmlNode *getXMLNode(NodePtr node) const {
    return static_cast<xmlNode *>(node.ptr);
  }

  QString getNodeId(xmlNode *node) const {
    xmlChar *node_id = xmlGetProp(node, (xmlChar *) "id");
    return QString::fromLatin1((char *) node_id, xmlStrlen(node_id));
  }

  QString getNodeClass(xmlNode *node) const {
    xmlChar *node_id = xmlGetProp(node, (xmlChar *) "class");
    return QString::fromLatin1((char *) node_id, xmlStrlen(node_id));
  }

  virtual bool nodeNameEquals(NodePtr node, const QString &nodeName) const {
    xmlNode *n = getXMLNode(node);
    if (!n) return false;
    QString name = QString::fromLatin1((char *) n->name);
    return QString::compare(name, nodeName, Qt::CaseInsensitive) == 0;
  }

  virtual QString attribute(NodePtr node, const QString &name) const {
    xmlNode *n = getXMLNode(node);
    if ((!getNodeId(n).isEmpty() && (name == QLatin1String("id") ||
                                     name == QLatin1String("xml:id"))))
      return getNodeId(n);
    if (!getNodeClass(n).isEmpty() && name == QLatin1String("class"))
      return getNodeClass(n);
    return QString();
  }

  virtual bool hasAttributes(NodePtr node) const {
    xmlNode *n = getXMLNode(node);
    return (n &&
            (!getNodeId(n).isEmpty() || !getNodeClass(n).isEmpty()));
  }

  virtual QStringList nodeIds(NodePtr node) const {
    xmlNode *n = getXMLNode(node);
    QString nid;
    if (n)
      nid = getNodeId(n);
    QStringList lst;
    lst.append(nid);
    return lst;
  }

  virtual QStringList nodeNames(NodePtr node) const {
    xmlNode *n = getXMLNode(node);
    if (n)
      return QStringList(QString::fromLatin1((char *) n->name, xmlStrlen(n->name)));
    return QStringList();
  }

  virtual bool isNullNode(NodePtr node) const {
    return !node.ptr;
  }

  virtual NodePtr parentNode(NodePtr node) const {
    xmlNode *n = getXMLNode(node);
    NodePtr newNode;
    newNode.ptr = 0;
    newNode.id = 0;
    if (n) {
      xmlNode *svgParent = n->parent;
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

    xmlNode *n = getXMLNode(node);
    if (!n)
      return newNode;

    newNode.ptr = getXMLNode(node)->prev;
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
