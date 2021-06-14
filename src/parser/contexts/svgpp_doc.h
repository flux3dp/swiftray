#include <parser/svgpp_common.h>
#include <set>

#ifndef SVGPP_DOC_H
#define SVGPP_DOC_H

class SVGPPDoc {
public:
  class FollowRef;

  SVGPPDoc(xmlDoc *doc) : xml_(doc) {}

  ~SVGPPDoc() {
    // TODO(Free all xml refs)
    xmlFree(doc);
  }

  xmlNode *root() { return xmlDocGetRootElement(xml_); }

  xmlNode *getElementById(xmlNode *rootnode, const xmlChar *id) {
    xmlNode *node = rootnode;
    if (node == nullptr) {
      qInfo() << "ROOT IS NULL?";
      return nullptr;
    }

    while (node != nullptr) {
      xmlChar *node_id = xmlGetProp(node, (xmlChar *) "id");
      if (node_id && xmlStrcmp(node_id, id) == 0) {
        return node;
      } else if (node->children != nullptr) {
        xmlNode *children_result = getElementById(node->children, id);
        if (children_result != nullptr) {
          return children_result;
        }
      }
      node = node->next;
    }
    return NULL;
  }

  xmlNode *getElementById(std::string id) { return getElementById(root(), (xmlChar *) id.c_str()); }

  xmlDoc *xml_;
  typedef std::set<xmlNode *> followed_refs_t;
  followed_refs_t followed_refs_;
};

class SVGPPDoc::FollowRef {
public:
  FollowRef(SVGPPDoc &document, xmlNode *el) : document_(document) {
    std::pair<SVGPPDoc::followed_refs_t::iterator, bool> ins = document.followed_refs_.insert(el);
    if (!ins.second)
      throw std::runtime_error("Cyclic reference found");
    lock_ = ins.first;
  }

  ~FollowRef() { document_.followed_refs_.erase(lock_); }

private:
  SVGPPDoc &document_;
  SVGPPDoc::followed_refs_t::iterator lock_;
};

#endif