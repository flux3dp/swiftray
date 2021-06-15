#include <parser/svgpp-common.h>
#include <libxml/xpath.h>
#include <set>

#ifndef SVGPP_DOC_H
#define SVGPP_DOC_H

class SVGPPDoc {
public:
  class FollowRef;

  SVGPPDoc(xmlDocPtr xml_doc) : xml_(xml_doc) {
    xml_context_ = xmlXPathNewContext(xml_doc);
  }

  ~SVGPPDoc() {
    qDebug() << "[Memory] ~SVGPPDoc(" << this << ")";
    xmlFreeDoc(xml_);
    xmlXPathFreeContext(xml_context_);
  }

  xmlNodePtr root() { return xmlDocGetRootElement(xml_); }

  xmlNodePtr getElementById(std::string id) {
    std::string query = QString("//*[@id='" + QString::fromStdString(id) + "']").toStdString();
    xmlXPathObjectPtr result = xmlXPathEval(
         (const xmlChar *) query.c_str(),
         xml_context_);
    if (result == nullptr || result->nodesetval == nullptr) return nullptr;

    for (int i = 0; i < result->nodesetval->nodeNr; i++) {
      xmlNodePtr node = result->nodesetval->nodeTab[i];
      if (node->type == XML_ELEMENT_NODE) {
        xmlFree(result);
        return node;
      }
    }
    return nullptr;
  }

  xmlDocPtr xml_;
  xmlXPathContextPtr xml_context_;
  typedef std::set<xmlNodePtr> followed_refs_t;
  followed_refs_t followed_refs_;
};

class SVGPPDoc::FollowRef {
public:
  FollowRef(SVGPPDoc &svgpp_doc, xmlNodePtr el) : svgpp_doc_(svgpp_doc) {
    std::pair<SVGPPDoc::followed_refs_t::iterator, bool> ins = svgpp_doc.followed_refs_.insert(el);
    if (!ins.second)
      throw std::runtime_error("Cyclic reference found");
    lock_ = ins.first;
  }

  ~FollowRef() { svgpp_doc_.followed_refs_.erase(lock_); }

private:
  SVGPPDoc &svgpp_doc_;
  SVGPPDoc::followed_refs_t::iterator lock_;
};

#endif