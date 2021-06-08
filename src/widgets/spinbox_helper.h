#include <QSpinBox>
#include <QLineEdit>

#ifndef SPINBOX_HELPER
#define SPINBOX_HELPER
class SpinBoxHelper : public QSpinBox {
    public:
        // By subclassing QSpinbox, we are allowed to access protected properties
        QLineEdit* lineEdit() { return QSpinBox::lineEdit(); }
};
#endif