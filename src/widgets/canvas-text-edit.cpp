#include <QDebug>
#include <widgets/canvas-text-edit.h>

CanvasTextEdit::CanvasTextEdit(QWidget *parent) : QPlainTextEdit(parent) {

}

QString CanvasTextEdit::preeditString() const {
  return preedit_data_;
}

void CanvasTextEdit::inputMethodEvent(QInputMethodEvent *event) {
  qInfo() << "IME Event" << event;
  preedit_data_ = event->preeditString();
  QPlainTextEdit::inputMethodEvent(event);
}