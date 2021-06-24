
#ifndef QFONTCOMBOBOX2_H
#define QFONTCOMBOBOX2_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/qcombobox.h>
#include <QtGui/qfontdatabase.h>

QT_REQUIRE_CONFIG(fontcombobox);
QT_BEGIN_NAMESPACE
class QFontComboBox2Private;

class Q_WIDGETS_EXPORT QFontComboBox2 : public QComboBox {
Q_OBJECT
  Q_PROPERTY(QFontDatabase::WritingSystem writingSystem READ writingSystem WRITE setWritingSystem)
  Q_PROPERTY(FontFilters fontFilters READ fontFilters WRITE setFontFilters)
  Q_PROPERTY(QFont currentFont READ currentFont WRITE setCurrentFont NOTIFY currentFontChanged)
public:
  explicit QFontComboBox2(QWidget *parent = nullptr);

  ~QFontComboBox2();

  void setWritingSystem(QFontDatabase::WritingSystem);

  QFontDatabase::WritingSystem writingSystem() const;

  enum FontFilter {
    AllFonts = 0,
    ScalableFonts = 0x1,
    NonScalableFonts = 0x2,
    MonospacedFonts = 0x4,
    ProportionalFonts = 0x8
  };
  Q_DECLARE_FLAGS(FontFilters, FontFilter)

  Q_FLAG(FontFilters)

  void setFontFilters(FontFilters filters);

  FontFilters fontFilters() const;

  QFont currentFont() const;

  QSize sizeHint() const override;

public Q_SLOTS:

  void setCurrentFont(const QFont &f);

Q_SIGNALS:

  void currentFontChanged(const QFont &f);

protected:
  bool event(QEvent *e) override;

private:
  Q_DISABLE_COPY(QFontComboBox2)

  Q_DECLARE_PRIVATE(QFontComboBox2)

  Q_PRIVATE_SLOT(d_func(), void _q_currentChanged(const QString &))
  Q_PRIVATE_SLOT(d_func(), void _q_updateModel())
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QFontComboBox2::FontFilters)

QT_END_NAMESPACE
#endif
