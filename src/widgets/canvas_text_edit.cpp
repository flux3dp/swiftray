#include <QDebug>
#include <widgets/canvas_text_edit.h>

CanvasTextEdit::CanvasTextEdit(QWidget *parent): QPlainTextEdit(parent) {

}

QString CanvasTextEdit::preeditString() const {
    return preedit_data_;
}

void CanvasTextEdit::inputMethodEvent(QInputMethodEvent *event) {
    if (event->preeditString() != "" ) {
        preedit_data_ = event->preeditString();
    }
    if (event->commitString() != "") {
        preedit_data_ = "";
    }
    QPlainTextEdit::inputMethodEvent(event);
}