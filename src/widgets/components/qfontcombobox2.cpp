
#include <widgets/components/qfontcombobox2.h>

#include <QStringListModel>
#include <QItemDelegate>
#include <QListView>
#include <QPainter>
#include <QEvent>
#include <QApplication>
#include <QDesktopWidget>
#include <QtWidgets/private/qcombobox_p.h>
#include <QtWidgets/private/qdesktopwidget_p.h>
#include <qfontdatabase.h>
#include <QDebug>

#include <windows/osxwindow.h>

static QFontDatabase::WritingSystem writingSystemFromScript(QLocale::Script script) {
  switch (script) {
    case QLocale::ArabicScript:
      return QFontDatabase::Arabic;
    case QLocale::CyrillicScript:
      return QFontDatabase::Cyrillic;
    case QLocale::GurmukhiScript:
      return QFontDatabase::Gurmukhi;
    case QLocale::SimplifiedHanScript:
      return QFontDatabase::SimplifiedChinese;
    case QLocale::TraditionalHanScript:
      return QFontDatabase::TraditionalChinese;
    case QLocale::LatinScript:
      return QFontDatabase::Latin;
    case QLocale::ArmenianScript:
      return QFontDatabase::Armenian;
    case QLocale::BengaliScript:
      return QFontDatabase::Bengali;
    case QLocale::DevanagariScript:
      return QFontDatabase::Devanagari;
    case QLocale::GeorgianScript:
      return QFontDatabase::Georgian;
    case QLocale::GreekScript:
      return QFontDatabase::Greek;
    case QLocale::GujaratiScript:
      return QFontDatabase::Gujarati;
    case QLocale::HebrewScript:
      return QFontDatabase::Hebrew;
    case QLocale::JapaneseScript:
      return QFontDatabase::Japanese;
    case QLocale::KhmerScript:
      return QFontDatabase::Khmer;
    case QLocale::KannadaScript:
      return QFontDatabase::Kannada;
    case QLocale::KoreanScript:
      return QFontDatabase::Korean;
    case QLocale::LaoScript:
      return QFontDatabase::Lao;
    case QLocale::MalayalamScript:
      return QFontDatabase::Malayalam;
    case QLocale::MyanmarScript:
      return QFontDatabase::Myanmar;
    case QLocale::TamilScript:
      return QFontDatabase::Tamil;
    case QLocale::TeluguScript:
      return QFontDatabase::Telugu;
    case QLocale::ThaanaScript:
      return QFontDatabase::Thaana;
    case QLocale::ThaiScript:
      return QFontDatabase::Thai;
    case QLocale::TibetanScript:
      return QFontDatabase::Tibetan;
    case QLocale::SinhalaScript:
      return QFontDatabase::Sinhala;
    case QLocale::SyriacScript:
      return QFontDatabase::Syriac;
    case QLocale::OriyaScript:
      return QFontDatabase::Oriya;
    case QLocale::OghamScript:
      return QFontDatabase::Ogham;
    case QLocale::RunicScript:
      return QFontDatabase::Runic;
    case QLocale::NkoScript:
      return QFontDatabase::Nko;
    default:
      return QFontDatabase::Any;
  }
}

static QFontDatabase::WritingSystem writingSystemFromLocale() {
  QStringList uiLanguages = QLocale::system().uiLanguages();
  QLocale::Script script;
  if (!uiLanguages.isEmpty())
    script = QLocale(uiLanguages.at(0)).script();
  else
    script = QLocale::system().script();
  return writingSystemFromScript(script);
}

static QFontDatabase::WritingSystem writingSystemForFont(const QFont &font, bool *hasLatin) {
  QList<QFontDatabase::WritingSystem> writingSystems = QFontDatabase().writingSystems(font.family());
//     qDebug() << font.family() << writingSystems;
  // this just confuses the algorithm below. Vietnamese is Latin with lots of special chars
  writingSystems.removeOne(QFontDatabase::Vietnamese);
  *hasLatin = writingSystems.removeOne(QFontDatabase::Latin);
  if (writingSystems.isEmpty())
    return QFontDatabase::Any;
  QFontDatabase::WritingSystem system = writingSystemFromLocale();
  if (writingSystems.contains(system))
    return system;
  if (system == QFontDatabase::TraditionalChinese
      && writingSystems.contains(QFontDatabase::SimplifiedChinese)) {
    return QFontDatabase::SimplifiedChinese;
  }
  if (system == QFontDatabase::SimplifiedChinese
      && writingSystems.contains(QFontDatabase::TraditionalChinese)) {
    return QFontDatabase::TraditionalChinese;
  }
  system = writingSystems.constLast();
  if (!*hasLatin) {
    // we need to show something
    return system;
  }
  if (writingSystems.count() == 1 && system > QFontDatabase::Cyrillic)
    return system;
  if (writingSystems.count() <= 2 && system > QFontDatabase::Armenian && system < QFontDatabase::Vietnamese)
    return system;
  if (writingSystems.count() <= 5 && system >= QFontDatabase::SimplifiedChinese && system <= QFontDatabase::Korean)
    return system;
  return QFontDatabase::Any;
}

class QFontFamilyDelegate : public QAbstractItemDelegate {
Q_OBJECT
public:
  explicit QFontFamilyDelegate(QObject *parent);

  // painting
  void paint(QPainter *painter,
             const QStyleOptionViewItem &option,
             const QModelIndex &index) const override;

  QSize sizeHint(const QStyleOptionViewItem &option,
                 const QModelIndex &index) const override;

  const QIcon truetype;
  const QIcon bitmap;
  QFontDatabase::WritingSystem writingSystem;
};

QFontFamilyDelegate::QFontFamilyDelegate(QObject *parent)
     : QAbstractItemDelegate(parent),
     // TODO (Update font icon here)
       truetype(QStringLiteral(":/qt-project.org/styles/commonstyle/images/fonttruetype-16.png")),
       bitmap(QStringLiteral(":/qt-project.org/styles/commonstyle/images/fontbitmap-16.png")),
       writingSystem(QFontDatabase::Any) {
}

void QFontFamilyDelegate::paint(QPainter *painter,
                                const QStyleOptionViewItem &option,
                                const QModelIndex &index) const {
  QString text = index.data(Qt::DisplayRole).toString();
  QFont font(option.font);
  font.setPointSize(QFontInfo(font).pointSize() * 3 / 2);
  QFont font2 = font;
  font2.setFamily(text);
  bool hasLatin = true;
  QFontDatabase::WritingSystem system = writingSystemForFont(font2, &hasLatin);
  //if (hasLatin)
  font = font2;
  QRect r = option.rect;
  if (option.state & QStyle::State_Selected) {
    painter->save();
    painter->setBrush(option.palette.highlight());
    painter->setPen(Qt::NoPen);
    painter->drawRect(option.rect);
    painter->setPen(QPen(option.palette.highlightedText(), 0));
  }
  const QIcon *icon = &bitmap;
  if (QFontDatabase().isSmoothlyScalable(text)) {
    icon = &truetype;
  }
  const QSize actualSize = icon->actualSize(r.size());
  const QRect iconRect = QStyle::alignedRect(option.direction, option.displayAlignment,
                                             actualSize, r);
  icon->paint(painter, iconRect, Qt::AlignLeft | Qt::AlignVCenter);
  if (option.direction == Qt::RightToLeft)
    r.setRight(r.right() - actualSize.width() - 4);
  else
    r.setLeft(r.left() + actualSize.width() + 4);
  QFont old = painter->font();
  painter->setFont(font);
  const Qt::Alignment textAlign = QStyle::visualAlignment(option.direction, option.displayAlignment);
  // If the ascent of the font is larger than the height of the rect,
  // we will clip the text, so it's better to align the tight bounding rect in this case
  // This is specifically for fonts where the ascent is very large compared to
  // the descent, like certain of the Stix family.
  QFontMetricsF fontMetrics(font);
  if (fontMetrics.ascent() > r.height()) {
    QRectF tbr = fontMetrics.tightBoundingRect(text);
    QRect textRect(r);
    textRect.setHeight(textRect.height() + (r.height() - tbr.height()));
    painter->drawText(textRect, Qt::AlignBottom | Qt::TextSingleLine | textAlign, text);
  } else {
    painter->drawText(r, Qt::AlignVCenter | Qt::TextSingleLine | textAlign, text);
  }
  if (writingSystem != QFontDatabase::Any)
    system = writingSystem;
  if (system != QFontDatabase::Any) {
    int w = painter->fontMetrics().horizontalAdvance(text + QLatin1String("  "));
    painter->setFont(font2);
    QString sample = QFontDatabase().writingSystemSample(system);
    if (option.direction == Qt::RightToLeft)
      r.setRight(r.right() - w);
    else
      r.setLeft(r.left() + w);
    painter->drawText(r, Qt::AlignVCenter | Qt::TextSingleLine | textAlign, sample);
  }
  painter->setFont(old);
  if (option.state & QStyle::State_Selected)
    painter->restore();
}

QSize QFontFamilyDelegate::sizeHint(const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const {
  QString text = index.data(Qt::DisplayRole).toString();
  QFont font(option.font);
//     font.setFamily(text);
  font.setPointSize(QFontInfo(font).pointSize() * 3 / 2);
  QFontMetrics fontMetrics(font);
  return QSize(fontMetrics.horizontalAdvance(text), fontMetrics.height());
}

class QFontComboBox2Private : public QComboBoxPrivate {
public:
  inline QFontComboBox2Private() { filters = QFontComboBox2::AllFonts; }

  QFontComboBox2::FontFilters filters;
  QFont currentFont;

  void _q_updateModel();

  void _q_currentChanged(const QString &);

  Q_DECLARE_PUBLIC(QFontComboBox2)
};

void QFontComboBox2Private::_q_updateModel() {
  Q_Q(QFontComboBox2);
  const int scalableMask = (QFontComboBox2::ScalableFonts | QFontComboBox2::NonScalableFonts);
  const int spacingMask = (QFontComboBox2::ProportionalFonts | QFontComboBox2::MonospacedFonts);
  QStringListModel *m = qobject_cast<QStringListModel *>(q->model());
  if (!m)
    return;
  QFontFamilyDelegate *delegate = qobject_cast<QFontFamilyDelegate *>(q->view()->itemDelegate());
  QFontDatabase::WritingSystem system = delegate ? delegate->writingSystem : QFontDatabase::Any;
  QFontDatabase fdb;
  // This only returns english name not display name
  QStringList list = fdb.families(system);
  QStringList result;
  int offset = 0;
  QFontInfo fi(currentFont);
  for (int i = 0; i < list.size(); ++i) {
    if (fdb.isPrivateFamily(list.at(i)))
      continue;
    if ((filters & scalableMask) && (filters & scalableMask) != scalableMask) {
      if (bool(filters & QFontComboBox2::ScalableFonts) != fdb.isSmoothlyScalable(list.at(i)))
        continue;
    }
    if ((filters & spacingMask) && (filters & spacingMask) != spacingMask) {
      if (bool(filters & QFontComboBox2::MonospacedFonts) != fdb.isFixedPitch(list.at(i)))
        continue;
    }
#ifdef Q_OS_MACOS
    result += familyNameFromPostScriptName(list.at(i));
#else
    result += list.at(i);
#endif
    if (list.at(i) == fi.family() || list.at(i).startsWith(fi.family() + QLatin1String(" [")))
      offset = result.count() - 1;
  }
  list = result;
  //we need to block the signals so that the model doesn't emit reset
  //this prevents the current index from changing
  //it will be updated just after this
  ///TODO: we should finda way to avoid blocking signals and have a real update of the model
  {
    const QSignalBlocker blocker(m);
    m->setStringList(list);
  }
  if (list.isEmpty()) {
    if (currentFont != QFont()) {
      currentFont = QFont();
      emit q->currentFontChanged(currentFont);
    }
  } else {
    q->setCurrentIndex(offset);
  }
}

void QFontComboBox2Private::_q_currentChanged(const QString &text) {
  Q_Q(QFontComboBox2);
  if (currentFont.family() != text) {
    currentFont.setFamily(text);
    emit q->currentFontChanged(currentFont);
  }
}
/*!
    \class QFontComboBox2
    \brief The QFontComboBox2 widget is a combobox that lets the user
    select a font family.
    \since 4.2
    \ingroup basicwidgets
    \inmodule QtWidgets
    The combobox is populated with an alphabetized list of font
    family names, such as Arial, Helvetica, and Times New Roman.
    Family names are displayed using the actual font when possible.
    For fonts such as Symbol, where the name is not representable in
    the font itself, a sample of the font is displayed next to the
    family name.
    QFontComboBox2 is often used in toolbars, in conjunction with a
    QComboBox for controlling the font size and two \l{QToolButton}s
    for bold and italic.
    When the user selects a new font, the currentFontChanged() signal
    is emitted in addition to currentIndexChanged().
    Call setWritingSystem() to tell QFontComboBox2 to show only fonts
    that support a given writing system, and setFontFilters() to
    filter out certain types of fonts as e.g. non scalable fonts or
    monospaced fonts.
    \image windowsvista-fontcombobox.png Screenshot of QFontComboBox2 on Windows Vista
    \sa QComboBox, QFont, QFontInfo, QFontMetrics, QFontDatabase, {Character Map Example}
*/
/*!
    Constructs a font combobox with the given \a parent.
*/
QFontComboBox2::QFontComboBox2(QWidget *parent)
     : QComboBox(*new QFontComboBox2Private, parent) {
  Q_D(QFontComboBox2);
  d->currentFont = font();
  setEditable(true);
  QStringListModel *m = new QStringListModel(this);
  setModel(m);
  setItemDelegate(new QFontFamilyDelegate(this));
  QListView *lview = qobject_cast<QListView *>(view());
  if (lview)
    lview->setUniformItemSizes(true);
  setWritingSystem(QFontDatabase::Any);
  connect(this, SIGNAL(currentIndexChanged(QString)),
          this, SLOT(_q_currentChanged(QString)));
  connect(qApp, SIGNAL(fontDatabaseChanged()),
          this, SLOT(_q_updateModel()));
}

/*!
    Destroys the combobox.
*/
QFontComboBox2::~QFontComboBox2() {
}

/*!
    \property QFontComboBox2::writingSystem
    \brief the writing system that serves as a filter for the combobox
    If \a script is QFontDatabase::Any (the default), all fonts are
    listed.
    \sa fontFilters
*/
void QFontComboBox2::setWritingSystem(QFontDatabase::WritingSystem script) {
  Q_D(QFontComboBox2);
  QFontFamilyDelegate *delegate = qobject_cast<QFontFamilyDelegate *>(view()->itemDelegate());
  if (delegate)
    delegate->writingSystem = script;
  d->_q_updateModel();
}

QFontDatabase::WritingSystem QFontComboBox2::writingSystem() const {
  QFontFamilyDelegate *delegate = qobject_cast<QFontFamilyDelegate *>(view()->itemDelegate());
  if (delegate)
    return delegate->writingSystem;
  return QFontDatabase::Any;
}
/*!
  \enum QFontComboBox2::FontFilter
  This enum can be used to only show certain types of fonts in the font combo box.
  \value AllFonts Show all fonts
  \value ScalableFonts Show scalable fonts
  \value NonScalableFonts Show non scalable fonts
  \value MonospacedFonts Show monospaced fonts
  \value ProportionalFonts Show proportional fonts
*/
/*!
    \property QFontComboBox2::fontFilters
    \brief the filter for the combobox
    By default, all fonts are listed.
    \sa writingSystem
*/
void QFontComboBox2::setFontFilters(FontFilters filters) {
  Q_D(QFontComboBox2);
  d->filters = filters;
  d->_q_updateModel();
}

QFontComboBox2::FontFilters QFontComboBox2::fontFilters() const {
  Q_D(const QFontComboBox2);
  return d->filters;
}

/*!
    \property QFontComboBox2::currentFont
    \brief the currently selected font
    \sa currentIndex, currentText
*/
QFont QFontComboBox2::currentFont() const {
  Q_D(const QFontComboBox2);
  return d->currentFont;
}

void QFontComboBox2::setCurrentFont(const QFont &font) {
  Q_D(QFontComboBox2);
  if (font != d->currentFont) {
    d->currentFont = font;
    d->_q_updateModel();
    if (d->currentFont == font) { //else the signal has already be emitted by _q_updateModel
      emit currentFontChanged(d->currentFont);
    }
  }
}
/*!
    \fn void QFontComboBox2::currentFontChanged(const QFont &font)
    This signal is emitted whenever the current font changes, with
    the new \a font.
    \sa currentFont
*/
/*!
    \reimp
*/
bool QFontComboBox2::event(QEvent *e) {
  if (e->type() == QEvent::Resize) {
    QListView *lview = qobject_cast<QListView *>(view());
    QDesktopWidget *widget = (QDesktopWidget *) lview;
    if (lview) {
      lview->window()->setFixedWidth(qMin(width() * 5 / 3,
                                          widget->availableGeometry(widget).width()));
    }
  }
  return QComboBox::event(e);
}

/*!
    \reimp
*/
QSize QFontComboBox2::sizeHint() const {
  QSize sz = QComboBox::sizeHint();
  QFontMetrics fm(font());
  sz.setWidth(fm.horizontalAdvance(QLatin1Char('m')) * 14);
  return sz;
}

#include "qfontcombobox2.moc"
#include "moc_qfontcombobox2.cpp"