#include <QPlainTextEdit>

#ifndef CANVASTEXTEDIT_H
#define CANVASTEXTEDIT_H
class CanvasTextEdit : public QPlainTextEdit {
    public:
        CanvasTextEdit(QWidget *parent);
        QString preeditString() const;
        void inputMethodEvent(QInputMethodEvent *event) override;
    private:
        QString preedit_data_;
};
#endif