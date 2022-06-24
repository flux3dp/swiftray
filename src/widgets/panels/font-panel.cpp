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
    emit fontChanged(font);
  });

  connect(ui->fontSizeSpinBox, spin_int_event, [=](double value) {
    font_.setPointSize(value);
    emit fontPointSizeChanged(value);
  });

  connect(ui->letterSpacingSpinBox, spin_event, [=](double value) {
    font_.setLetterSpacing(QFont::SpacingType::AbsoluteSpacing, value);
    emit fontLetterSpacingChanged(value);
  });

  connect(ui->lineHeightSpinBox, spin_event, main_window_->canvas(), &Canvas::setLineHeight);

  connect(ui->lineHeightSpinBox, spin_event, [=](double value) {
    line_height_ = value;
    emit lineHeightChanged(value);
  });

  connect(ui->boldToolButton, &QToolButton::toggled, [=](bool checked) {
    font_.setBold(checked);
    emit fontBoldChanged(checked);
  });

  connect(ui->italicToolButton, &QToolButton::toggled, [=](bool checked) {
    font_.setItalic(checked);
    emit fontItalicChanged(checked);
  });

  connect(ui->underlineToolButton, &QToolButton::toggled, [=](bool checked) {
    font_.setUnderline(checked);
    emit fontUnderlineChanged(checked);
  });

  connect(main_window_->canvas(), &Canvas::selectionsChanged, this, [=]() {
    QFont first_qfont;
    double first_linehight;
    bool has_txt = false;
    bool is_font_changed = false, is_pt_changed = false, is_ls_changed = false, is_linehight_changed = false;
    bool is_bold_changed = false, is_italic_changed = false, is_underline_changed = false;
    for (auto &shape : main_window_->canvas()->document().selections()) {
      if (shape->type() == ::Shape::Type::Text) {
        auto *t = (TextShape *) shape.get();
        if(!has_txt) {
          first_linehight = t->lineHeight();
          first_qfont = t->font();
          has_txt = true;
        }
        if(has_txt && first_qfont.family() != t->font().family())                is_font_changed = true;
        if(has_txt && first_qfont.pointSize() != t->font().pointSize())          is_pt_changed = true;
        if(has_txt && first_qfont.letterSpacing() != t->font().letterSpacing())  is_ls_changed = true;
        if(has_txt && first_qfont.bold() != t->font().bold())                    is_bold_changed = true;
        if(has_txt && first_qfont.italic() != t->font().italic())                is_italic_changed = true;
        if(has_txt && first_qfont.underline() != t->font().underline())          is_underline_changed = true;
        if(has_txt && first_linehight != t->lineHeight())                        is_linehight_changed = true;
      }
    }
    if(has_txt) {
      if(is_font_changed) {
        ui->fontComboBox->blockSignals(true);
        ui->fontComboBox->setCurrentText("");
        ui->fontComboBox->blockSignals(false);
      }
      else {
        ui->fontComboBox->setCurrentText(first_qfont.family());
      }
      if(is_pt_changed) {
        ui->fontSizeSpinBox->blockSignals(true);
        ui->fontSizeSpinBox->setSpecialValueText(tr(" "));
        ui->fontSizeSpinBox->setValue(0);
        ui->fontSizeSpinBox->blockSignals(false);
      }
      else {
        ui->fontSizeSpinBox->setValue(first_qfont.pointSize());
      }
      if(is_ls_changed) {
        ui->letterSpacingSpinBox->blockSignals(true);
        ui->letterSpacingSpinBox->setSpecialValueText(tr(" "));
        ui->letterSpacingSpinBox->setValue(-0.1);
        ui->letterSpacingSpinBox->blockSignals(false);
      }
      else {
        ui->letterSpacingSpinBox->setValue(first_qfont.letterSpacing());
      }
      if(is_bold_changed) {
        ui->boldToolButton->blockSignals(true);
        ui->boldToolButton->setChecked(false);
        font_.setBold(false);
        ui->boldToolButton->blockSignals(false);
      }
      else {
        ui->boldToolButton->setChecked(first_qfont.bold());
      }
      if(is_italic_changed) {
        ui->italicToolButton->blockSignals(true);
        ui->italicToolButton->setChecked(false);
        font_.setItalic(false);
        ui->italicToolButton->blockSignals(false);
      }
      else {
        ui->italicToolButton->setChecked(first_qfont.italic());
      }
      if(is_underline_changed) {
        ui->underlineToolButton->blockSignals(true);
        ui->underlineToolButton->setChecked(false);
        font_.setUnderline(false);
        ui->underlineToolButton->blockSignals(false);
      }
      else {
        ui->underlineToolButton->setChecked(first_qfont.underline());
      }
      if(is_linehight_changed) {
        ui->lineHeightSpinBox->blockSignals(true);
        ui->lineHeightSpinBox->setSpecialValueText(tr(" "));
        ui->lineHeightSpinBox->setValue(0);
        ui->lineHeightSpinBox->blockSignals(false);
      }
      else {
        ui->lineHeightSpinBox->setValue(first_linehight);
      }
    }
    else {
      setFont(font_, line_height_);
      ui->fontComboBox->setCurrentText(font_.family());
    }
  });
}

void FontPanel::setFont(QFont font, float line_height) {
  font_ = font;
  line_height_ = line_height;
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
