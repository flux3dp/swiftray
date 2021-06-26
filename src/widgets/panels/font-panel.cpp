#include <QFontComboBox>
#include "font-panel.h"
#include "ui_font-panel.h"
#include <canvas/canvas.h>
#include <widgets/components/spinbox-helper.h>

FontPanel::FontPanel(QWidget *parent, Canvas *canvas) :
     QFrame(parent),
     canvas_(canvas),
     ui(new Ui::FontPanel) {
  assert(parent != nullptr && canvas != nullptr);
  ui->setupUi(this);
  loadStyles();
  registerEvents();
  setFont(QFont("Tahoma", 150, QFont::Bold), 1.5);
}

void FontPanel::loadStyles() {
  ((SpinBoxHelper<QSpinBox> *) ui->fontSize)->lineEdit()->setStyleSheet("padding: 0 8px;");
  ((SpinBoxHelper<QDoubleSpinBox> *) ui->letterSpacing)->lineEdit()->setStyleSheet("padding: 0 8px;");
  ((SpinBoxHelper<QDoubleSpinBox> *) ui->lineHeight)->lineEdit()->setStyleSheet("padding: 0 8px;");
}

void FontPanel::registerEvents() {
  auto spin_event = QOverload<double>::of(&QDoubleSpinBox::valueChanged);
  auto spin_int_event = QOverload<int>::of(&QSpinBox::valueChanged);

  connect(ui->fontComboBox, &QFontComboBox2::currentFontChanged, canvas_, &Canvas::setFont);

  connect(ui->fontSize, spin_int_event, [=](double value) {
    font_.setPointSize(value);
    ui->fontComboBox->setCurrentFont(font_);
  });

  connect(ui->letterSpacing, spin_event, [=](double value) {
    font_.setLetterSpacing(QFont::SpacingType::AbsoluteSpacing, value);
    ui->fontComboBox->setCurrentFont(font_);
  });

  connect(ui->lineHeight, spin_event, canvas_, &Canvas::setLineHeight);

  connect(ui->bold, &QCheckBox::toggled, [=](bool checked) {
    font_.setBold(checked);
    ui->fontComboBox->setCurrentFont(font_);
  });

  connect(ui->italic, &QCheckBox::toggled, [=](bool checked) {
    font_.setItalic(checked);
    ui->fontComboBox->setCurrentFont(font_);
  });

  connect(ui->underline, &QCheckBox::toggled, [=](bool checked) {
    font_.setUnderline(checked);
    ui->fontComboBox->setCurrentFont(font_);
  });

  connect(canvas_, &Canvas::selectionsChanged, this, [=]() {
    for (auto &shape : canvas_->document().selections()) {
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

void FontPanel::setFont(QFont font, float line_height) {
  font_ = font;
  ui->fontComboBox->setCurrentFont(font);
  ui->fontSize->setValue(font.pointSize());
  ui->letterSpacing->setValue(font.letterSpacing());
  ui->lineHeight->setValue(line_height);
  ui->bold->setChecked(font.bold());
  ui->italic->setChecked(font.italic());
  ui->underline->setChecked(font.underline());
}