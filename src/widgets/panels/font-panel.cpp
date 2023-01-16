#include <QFontComboBox>
#include "font-panel.h"
#include "ui_font-panel.h"
#include <constants.h>

FontPanel::FontPanel(QWidget *parent, bool is_dark_mode) :
     QFrame(parent),
     ui(new Ui::FontPanel),
     BaseContainer() {
  assert(parent != nullptr);
  ui->setupUi(this);
  initializeContainer();
  setLayout(is_dark_mode);
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

  connect(ui->fontComboBox, &QFontComboBox::currentFontChanged, [=](QFont font) {
    Q_EMIT editShapeFontFamily(font);
  });

  connect(ui->fontSizeSpinBox, spin_int_event, [=](double value) {
    Q_EMIT editShapeFontPointSize(value);
  });

  connect(ui->letterSpacingSpinBox, spin_event, [=](double value) {
    Q_EMIT editShapeLetterSpacing(value);
  });

  connect(ui->lineHeightSpinBox, spin_event, [=](double value) {
    Q_EMIT editShapeLineHeight(value);
  });

  connect(ui->boldToolButton, &QToolButton::toggled, [=](bool checked) {
    Q_EMIT editShapeBold(checked);
  });

  connect(ui->italicToolButton, &QToolButton::toggled, [=](bool checked) {
    Q_EMIT editShapeItalic(checked);
  });

  connect(ui->underlineToolButton, &QToolButton::toggled, [=](bool checked) {
    Q_EMIT editShapeUnderline(checked);
  });
}

FontPanel::~FontPanel() {
  delete ui;
}

void FontPanel::setLayout(bool is_dark_mode) {
  ui->boldToolButton->setIcon(QIcon(is_dark_mode ? ":/resources/images/dark/icon-Bold.png" : ":/resources/images/icon-Bold.png"));
  ui->italicToolButton->setIcon(QIcon(is_dark_mode ? ":/resources/images/dark/icon-I.png" : ":/resources/images/icon-I.png"));
  ui->underlineToolButton->setIcon(QIcon(is_dark_mode ? ":/resources/images/dark/icon-U.png" : ":/resources/images/icon-U.png"));
}

void FontPanel::setFontFamily(QString font_family) {
  QFont font;
  font.setFamily(font_family);
  ui->fontComboBox->blockSignals(true);
  ui->fontComboBox->setCurrentFont(font);
  ui->fontComboBox->blockSignals(false);
}

void FontPanel::setPointSize(int point_size) {
  ui->fontSizeSpinBox->blockSignals(true);
  ui->fontSizeSpinBox->setValue(point_size);
  ui->fontSizeSpinBox->blockSignals(false);
}

void FontPanel::setLetterSpacing(double spacing) {
  ui->letterSpacingSpinBox->blockSignals(true);
  ui->letterSpacingSpinBox->setValue(spacing);
  ui->letterSpacingSpinBox->blockSignals(false);
}

void FontPanel::setBold(bool bold) {
  ui->boldToolButton->blockSignals(true);
  ui->boldToolButton->setChecked(bold);
  ui->boldToolButton->blockSignals(false);
}

void FontPanel::setItalic(bool italic) {
  ui->italicToolButton->blockSignals(true);
  ui->italicToolButton->setChecked(italic);
  ui->italicToolButton->blockSignals(false);
}

void FontPanel::setUnderline(bool underline) {
  ui->underlineToolButton->blockSignals(true);
  ui->underlineToolButton->setChecked(underline);
  ui->underlineToolButton->blockSignals(false);
}

void FontPanel::setLineHeight(double line_height) {
  ui->lineHeightSpinBox->blockSignals(true);
  ui->lineHeightSpinBox->setValue(line_height);
  ui->lineHeightSpinBox->blockSignals(false);
}

//this part must follow mainwindow
void FontPanel::updateFontView(QSet<QString> font_familys, 
                    QSet<int> point_sizes, 
                    QSet<qreal> letter_spacings, 
                    QSet<bool> bolds, 
                    QSet<bool> italics, 
                    QSet<bool> underlines, 
                    QSet<double> line_heights) {
  if(font_familys.size() == 1) {
    setFontFamily(*font_familys.begin());
  } else if(!font_familys.empty()) {
    ui->fontComboBox->blockSignals(true);
    ui->fontComboBox->setCurrentText("");
    ui->fontComboBox->blockSignals(false);
  }
  if(point_sizes.size() == 1) {
    setPointSize(*point_sizes.begin());
  } else if(!point_sizes.empty()) {
    ui->fontSizeSpinBox->blockSignals(true);
    ui->fontSizeSpinBox->setSpecialValueText(tr(" "));
    ui->fontSizeSpinBox->setValue(0);
    ui->fontSizeSpinBox->blockSignals(false);
  }
  if(letter_spacings.size() == 1) {
    setLetterSpacing(*letter_spacings.begin());
  } else if(!letter_spacings.empty()) {
    ui->letterSpacingSpinBox->blockSignals(true);
    ui->letterSpacingSpinBox->setSpecialValueText(tr(" "));
    ui->letterSpacingSpinBox->setValue(-0.1);
    ui->letterSpacingSpinBox->blockSignals(false);
  }
  if(bolds.size() == 1) {
    setBold(*bolds.begin());
  } else if(!bolds.empty()) {
    ui->boldToolButton->blockSignals(true);
    ui->boldToolButton->setChecked(false);
    ui->boldToolButton->blockSignals(false);
  }
  if(italics.size() == 1) {
    setItalic(*italics.begin());
  } else if(!italics.empty()) {
    ui->italicToolButton->blockSignals(true);
    ui->italicToolButton->setChecked(false);
    ui->italicToolButton->blockSignals(false);
  }
  if(underlines.size() == 1) {
    setUnderline(*underlines.begin());
  } else if(!underlines.empty()) {
    ui->underlineToolButton->blockSignals(true);
    ui->underlineToolButton->setChecked(false);
    ui->underlineToolButton->blockSignals(false);
  }
  if(line_heights.size() == 1) {
    setLineHeight(*line_heights.begin());
  } else if(!line_heights.empty()) {
    ui->lineHeightSpinBox->blockSignals(true);
    ui->lineHeightSpinBox->setSpecialValueText(tr(" "));
    ui->lineHeightSpinBox->setValue(0);
    ui->lineHeightSpinBox->blockSignals(false);
  }
}

void FontPanel::hideEvent(QHideEvent *event) {
  Q_EMIT panelShow(false);
}

void FontPanel::showEvent(QShowEvent *event) {
  Q_EMIT panelShow(true);
}
