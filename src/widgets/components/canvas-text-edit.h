#pragma once

#include <QPlainTextEdit>

class CanvasTextEdit : public QPlainTextEdit {
public:

  CanvasTextEdit(QWidget *parent);

  QString preeditString() const;

  void inputMethodEvent(QInputMethodEvent *event) override;

private:

  QString preedit_data_;
};