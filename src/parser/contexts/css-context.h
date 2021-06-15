#ifndef CSSCONTEXT_H
#define CSSCONTEXT_H

#include <QtGui/private/qcssparser_p.h>
#include <QtSvg/private/qsvghandler_p.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <parser/contexts/base-context.h>
#include <parser/svgpp_defs.h>

class CSSContext : public BaseContext {
public:
  CSSContext(BaseContext const &parent) : BaseContext(parent) {
    qInfo() << "<style>";
  }

  template<class Range>
  void set_text(Range const &text) {
    std::string str = std::string(text.begin(), text.end());
    QString css = QString::fromStdString(str);
    QCss::StyleSheet sheet;
    bool success = QCss::Parser(css).parse(&sheet);
    if (success) {
      qInfo() << "Success load CSS" << sheet.styleRules.size();
      svgpp_style_selector->styleSheets.append(sheet);
    } else {
      qInfo() << "Failed to load CSS";
    }
  }

  void on_exit_element() {
    qInfo() << "</style>";
  }

  using BaseContext::set;
  using ObjectContext::set;
  using StylableContext::set;

private:
  std::string fragment_id_;
};

#endif