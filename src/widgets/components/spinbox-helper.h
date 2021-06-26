#include <QSpinBox>
#include <QLineEdit>

#ifndef SPINBOX_HELPER
#define SPINBOX_HELPER

template<class SpinBoxType>
class SpinBoxHelper : public SpinBoxType {
    public:
        // By subclassing QSpinbox, we are allowed to access protected properties
        QLineEdit* lineEdit() { return SpinBoxType::lineEdit(); }
};
#endif