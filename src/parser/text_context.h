#include <QDebug>
#include <parser/base_context.h>
#include <iostream>

#pragma once

class TextContext : public BaseContext {
public:
  TextContext(BaseContext const &parent) : BaseContext(parent), x_(0), y_(0) {
    qInfo() << "Enter text";
  }

  template <class Range>
  void set_text(Range const &text) {
    text_content_.append(boost::begin(text), boost::end(text));
  }

  template <class Range>
  void set(tag::attribute::x, Range const &range) {
    for (auto it = boost::begin(range), end = boost::end(range); it != end;
         ++it)
      std::cout << *it;
  }

  template <class Range>
  void set(tag::attribute::y, Range const &range) {
    for (auto it = boost::begin(range), end = boost::end(range); it != end;
         ++it)
      std::cout << *it;
  }

  using ObjectContext::set;
  using StylableContext::set;

  void set(tag::attribute::x, double x) { x_ = x; }

  void set(tag::attribute::y, double y) { y_ = y; }

private:
  std::string text_content_;
  double x_, y_;
};
