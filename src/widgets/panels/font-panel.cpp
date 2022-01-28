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
    ui->fontComboBox->setCurrentFont(font_);
    emit fontSettingChanged();
  });

  connect(ui->fontSizeSpinBox, spin_int_event, [=](double value) {
    font_.setPointSize(value);
    ui->fontComboBox->setCurrentFont(font_);
    emit fontSettingChanged();
  });

  connect(ui->letterSpacingSpinBox, spin_event, [=](double value) {
    font_.setLetterSpacing(QFont::SpacingType::AbsoluteSpacing, value);
    ui->fontComboBox->setCurrentFont(font_);
    emit fontSettingChanged();
  });

  connect(ui->lineHeightSpinBox, spin_event, main_window_->canvas(), &Canvas::setLineHeight);

  connect(ui->lineHeightSpinBox, spin_event, [=](double value) {
    emit lineHeightChanged(value);
  });

  connect(ui->boldToolButton, &QToolButton::toggled, [=](bool checked) {
    font_.setBold(checked);
    ui->fontComboBox->setCurrentFont(font_);
    emit fontSettingChanged();
  });

  connect(ui->italicToolButton, &QToolButton::toggled, [=](bool checked) {
    font_.setItalic(checked);
    ui->fontComboBox->setCurrentFont(font_);
    emit fontSettingChanged();
  });

  connect(ui->underlineToolButton, &QToolButton::toggled, [=](bool checked) {
    font_.setUnderline(checked);
    ui->fontComboBox->setCurrentFont(font_);
    emit fontSettingChanged();
  });

  connect(main_window_->canvas(), &Canvas::selectionsChanged, this, [=]() {
    for (auto &shape : main_window_->canvas()->document().selections()) {
      if (shape->type() == ::Shape::Type::Text) {
        auto *t = (TextShape *) shape.get();
        setFont(t->font(), t->lineHeight());
        break;
      }
    }
  });
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

void FontPanel::setLayout() {
  ui->boldToolButton->setIcon(QIcon(isDarkMode() ? ":/images/dark/icon-bold.png" : ":/images/icon-bold.png"));
  ui->italicToolButton->setIcon(QIcon(isDarkMode() ? ":/images/dark/icon-I.png" : ":/images/icon-I.png"));
  ui->underlineToolButton->setIcon(QIcon(isDarkMode() ? ":/images/dark/icon-U.png" : ":/images/icon-U.png"));
}

void FontPanel::setLineHeight(double line_height) {
  ui->lineHeightSpinBox->setValue(line_height);
}
