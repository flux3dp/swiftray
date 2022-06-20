#include <QFontComboBox>
#include "font-panel.h"
#include "ui_font-panel.h"
#include <constants.h>
#include <canvas/canvas.h>
#include <windows/mainwindow.h>
#include <windows/osxwindow.h>

FontPanel::FontPanel(QWidget *parent, MainWindow *main_window) :
     QFrame(parent),
     main_window_(main_window),
     ui(new Ui::FontPanel),
     BaseContainer() {
  assert(parent != nullptr && main_window != nullptr);
  ui->setupUi(this);
  setFont(QFont(FONT_TYPE, FONT_SIZE, QFont::Bold), LINE_HEIGHT);
  initializeContainer();
  setLayout();
}

void FontPanel::loadStyles() {
  ui->frame->setStyleSheet("\
      QToolButton {   \
          background-color: rgba(89, 89, 89, 100); \
          border: none \
      } \
      QToolButton:checked{ \
          background-color: rgba(20, 20, 20, 100); \
          border: none \
      } \
  ");
}

void FontPanel::registerEvents() {
  auto spin_event = QOverload<double>::of(&QDoubleSpinBox::valueChanged);
  auto spin_int_event = QOverload<int>::of(&QSpinBox::valueChanged);

  connect(ui->fontComboBox, &QFontComboBox::currentFontChanged, main_window_->canvas(), &Canvas::setFont);

  connect(ui->fontComboBox, &QFontComboBox::currentFontChanged, [=](QFont font) {
    font_.setFamily(font.family());
    // ui->fontComboBox->setCurrentFont(font_);//bad feeling
    emit fontChanged(font);
  });

  connect(ui->fontSizeSpinBox, spin_int_event, [=](double value) {
    font_.setPointSize(value);
    // ui->fontSizeSpinBox->setValue(value);
    emit fontPointSizeChanged(value);
  });

  connect(ui->letterSpacingSpinBox, spin_event, [=](double value) {
    font_.setLetterSpacing(QFont::SpacingType::AbsoluteSpacing, value);
    // ui->letterSpacingSpinBox->setValue(value);
    emit fontLetterSpacingChanged(value);
  });

  connect(ui->lineHeightSpinBox, spin_event, main_window_->canvas(), &Canvas::setLineHeight);

  connect(ui->lineHeightSpinBox, spin_event, [=](double value) {
    emit lineHeightChanged(value);
  });

  connect(ui->boldToolButton, &QToolButton::toggled, [=](bool checked) {
    font_.setBold(checked);
    // ui->boldToolButton->setChecked(checked);
    emit fontBoldChanged(checked);
  });

  connect(ui->italicToolButton, &QToolButton::toggled, [=](bool checked) {
    font_.setItalic(checked);
    // ui->italicToolButton->setChecked(checked);
    emit fontItalicChanged(checked);
  });

  connect(ui->underlineToolButton, &QToolButton::toggled, [=](bool checked) {
    font_.setUnderline(checked);
    // ui->underlineToolButton->setChecked(checked);
    emit fontUnderlineChanged(checked);
  });

  connect(main_window_->canvas(), &Canvas::selectionsChanged, this, [=]() {
    QFont first_qfont;
    double first_linehight;
    bool first_find = false, changed = false;
    for (auto &shape : main_window_->canvas()->document().selections()) {
      if (shape->type() == ::Shape::Type::Text) {
        auto *t = (TextShape *) shape.get();
        if(!first_find) {
          first_linehight = t->lineHeight();
          first_qfont = t->font();
          first_find = true;
        }
        if(first_find && first_qfont.family() != t->font().family()) {
          changed = true;
          ui->fontComboBox->blockSignals(true);
          ui->fontComboBox->setCurrentText("");
          ui->fontComboBox->blockSignals(false);
        }
        if(first_find && first_qfont.pointSize() != t->font().pointSize()) {
          changed = true;
          ui->fontSizeSpinBox->blockSignals(true);
          // QPalette palette_setting = ui->fontSizeSpinBox->palette();
          // palette_setting.setColor(QPalette::Text, QColor(000, 255, 000));
          // ui->fontSizeSpinBox->setPalette(palette_setting);
          ui->fontSizeSpinBox->blockSignals(false);
        }
        if(first_find && first_qfont.letterSpacing() != t->font().letterSpacing()) {
          changed = true;
          ui->letterSpacingSpinBox->blockSignals(true);
          ui->letterSpacingSpinBox->blockSignals(false);
        }
        if(first_find && first_qfont.bold() != t->font().bold()) {
          changed = true;
          ui->boldToolButton->blockSignals(true);
          ui->boldToolButton->setChecked(false);
          font_.setBold(false);
          ui->boldToolButton->blockSignals(false);
        }
        if(first_find && first_qfont.italic() != t->font().italic()) {
          changed = true;
          ui->italicToolButton->blockSignals(true);
          ui->italicToolButton->setChecked(false);
          font_.setItalic(false);
          ui->italicToolButton->blockSignals(false);
        }
        if(first_find && first_qfont.underline() != t->font().underline()) {
          changed = true;
          ui->underlineToolButton->blockSignals(true);
          ui->underlineToolButton->setChecked(false);
          font_.setUnderline(false);
          ui->underlineToolButton->blockSignals(false);
        }
        if(first_find && first_linehight != t->lineHeight()) {
          changed = true;
          ui->lineHeightSpinBox->blockSignals(true);
          ui->lineHeightSpinBox->blockSignals(false);
        }
      }
    }
    if(first_find && !changed) {
      setFont(first_qfont, first_linehight);
      ui->fontComboBox->setCurrentText(ui->fontComboBox->currentFont().family());
    }
  });
}

void FontPanel::setFont(QFont font, float line_height) {
  font_ = font;
  ui->fontComboBox->setCurrentFont(font);
  ui->fontSizeSpinBox->setValue(font.pointSize());
  ui->letterSpacingSpinBox->setValue(font.letterSpacing());
  ui->lineHeightSpinBox->setValue(line_height);
  ui->boldToolButton->setChecked(font.bold());
  qInfo() << "font.bold()" << font.bold();
  ui->italicToolButton->setChecked(font.italic());
  qInfo() << "font.italic()" << font.italic();
  ui->underlineToolButton->setChecked(font.underline());
  qInfo() << "font.underline()" << font.underline();
}

FontPanel::~FontPanel() {
  delete ui;
}

QFont FontPanel::font() {
  return font_;
}

double FontPanel::lineHeight() {
  return ui->lineHeightSpinBox->value();
}

void FontPanel::setFont(const QFont &font) {
  // QFont current_font = ui->fontComboBox->currentFont();
  // current_font.setFamily(font.family());
  // ui->fontComboBox->setCurrentFont(current_font);
  return ui->fontComboBox->setCurrentFont(font);
}

void FontPanel::setPointSize(int point_size) {
  return ui->fontSizeSpinBox->setValue(point_size);
}

void FontPanel::setLetterSpacing(double spacing) {
  return ui->letterSpacingSpinBox->setValue(spacing);
}

void FontPanel::setBold(bool bold) {
  return ui->boldToolButton->setChecked(bold);
}

void FontPanel::setItalic(bool italic) {
  return ui->italicToolButton->setChecked(italic);
}

void FontPanel::setUnderline(bool underline) {
  return ui->underlineToolButton->setChecked(underline);
}

void FontPanel::setLayout() {
  ui->boldToolButton->setIcon(QIcon(isDarkMode() ? ":/images/dark/icon-bold.png" : ":/images/icon-bold.png"));
  ui->italicToolButton->setIcon(QIcon(isDarkMode() ? ":/images/dark/icon-I.png" : ":/images/icon-I.png"));
  ui->underlineToolButton->setIcon(QIcon(isDarkMode() ? ":/images/dark/icon-U.png" : ":/images/icon-U.png"));
}

void FontPanel::setLineHeight(double line_height) {
  ui->lineHeightSpinBox->setValue(line_height);
}
