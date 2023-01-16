#include "main_application.h"
#include <utils/software_update.h>
#include <constants.h>
#include <shape/text-shape.h>

#include <QApplication>


MainApplication *mainApp = NULL;

MainApplication::MainApplication(int &argc,  char **argv) :
    QApplication(argc, argv)
{
  mainApp = this;

#if defined(HAVE_SOFTWARE_UPDATE) && defined(Q_OS_WIN)
  connect(this, &MainApplication::softwareUpdateQuit, this, &QApplication::quit, Qt::QueuedConnection);
#endif

  font_ = QFont(FONT_TYPE, FONT_SIZE, QFont::Bold);
  line_height_ = LINE_HEIGHT;

  // NOTE: qApp: built-in macro of the QApplication
  connect(qApp, &QApplication::aboutToQuit, this, &MainApplication::cleanup);

}

void MainApplication::cleanup()
{
    // TODO: Currently most of the app init/cleanup/actions are handled in MainWindow, 
    //       to be moved to MainApplication in the future
    //software_update_cleanup(); 
    // Write the user's recent file(s) to disk.
    //write_profile_recent();
    //write_recent();

    // We might end up here via exit_application.
    //QThreadPool::globalInstance()->waitForDone();
}

MainApplication::~MainApplication()
{
    mainApp = NULL;
}

#if defined(HAVE_SOFTWARE_UPDATE) && defined(Q_OS_WIN)
bool MainApplication::softwareUpdateCanShutdown() {
  software_update_ok_ = true;
  // At this point the update is ready to install, but WinSparkle has
  // not yet run the installer. We need to close our "Wireshark is
  // running" mutexes along with those of our child processes, e.g.
  // dumpcap.

  // Step 1: See if we have any open files.
  Q_EMIT softwareUpdateRequested();
  if (software_update_ok_ == true) {

    // Step 2: Close the "running" mutexes.
    //Q_EMIT softwareUpdateClose();
    //close_app_running_mutex();
  }
  return software_update_ok_;
}

void MainApplication::softwareUpdateShutdownRequest() {
  // At this point the installer has been launched. Neither Wireshark nor
  // its children should have any "Wireshark is running" mutexes open.
  // The main window should be closed.

  // Step 3: Quit.
  Q_EMIT softwareUpdateQuit();
}

/** Check to see if Wireshark can shut down safely (e.g. offer to save the
 *  current capture).
 */
extern "C" int software_update_can_shutdown_callback(void) {
  return mainApp->softwareUpdateCanShutdown();
}

/** Shut down Wireshark in preparation for an upgrade.
 */
extern "C" void software_update_shutdown_request_callback(void) {
  mainApp->softwareUpdateShutdownRequest();
}
#endif // HAVE_SOFTWARE_UPDATE && Q_OS_WIN

//about font
QFont MainApplication::getFont() {
  return font_;
}

double MainApplication::getFontLineHeight() {
  return line_height_;
}

void MainApplication::getSelectShapeChange(QList<ShapePtr> shape_list) {
  bool has_txt = false;
  QSet<QString> font_family;
  QSet<int> point_size;
  QSet<qreal> letter_spacing;
  QSet<bool> bold;
  QSet<bool> italic;
  QSet<bool> underline;
  QSet<double> line_height;
  for(auto &shape : shape_list) {
    if(shape->type() == ::Shape::Type::Text) {
      TextShape *txt_shape = (TextShape*)shape.get();
      has_txt = true;
      font_family.insert(txt_shape->font().family());
      point_size.insert(txt_shape->font().pointSize());
      letter_spacing.insert(txt_shape->font().letterSpacing());
      bold.insert(txt_shape->font().bold());
      italic.insert(txt_shape->font().italic());
      underline.insert(txt_shape->font().underline());
      line_height.insert(txt_shape->lineHeight());
    }
  }
  if(has_txt) {
    if(font_family.size() == 1) {
      font_.setFamily(*font_family.begin());
    }
    if(point_size.size() == 1) {
      font_.setPointSize(*point_size.begin());
    }
    if(letter_spacing.size() == 1) {
      font_.setLetterSpacing(QFont::SpacingType::AbsoluteSpacing, *letter_spacing.begin());
    }
    if(bold.size() == 1) {
      font_.setBold(*bold.begin());
    }
    if(italic.size() == 1) {
      font_.setItalic(*italic.begin());
    }
    if(underline.size() == 1) {
      font_.setUnderline(*underline.begin());
    }
    if(line_height.size() == 1) {
      line_height_ = *line_height.begin();
    }
    Q_EMIT updateFontView(font_family, point_size, letter_spacing, bold, 
                          italic, underline, line_height);
  } else {
    font_family.insert(font_.family());
    point_size.insert(font_.pointSize());
    letter_spacing.insert(font_.letterSpacing());
    bold.insert(font_.bold());
    italic.insert(font_.italic());
    underline.insert(font_.underline());
    line_height.insert(line_height_);
    Q_EMIT updateFontView(font_family, point_size, letter_spacing, bold, 
                          italic, underline, line_height);
  }
}

void MainApplication::updateShapeFontFamily(QFont font) {
  font_.setFamily(font.family());
  Q_EMIT editShapeFontFamily(font.family());
}

void MainApplication::updateShapeFontPointSize(int point_size) {
  font_.setPointSize(point_size);
  Q_EMIT editShapeFontPointSize(point_size);
}

void MainApplication::updateShapeLetterSpacing(qreal letter_spacing) {
  font_.setLetterSpacing(QFont::SpacingType::AbsoluteSpacing, letter_spacing);
  Q_EMIT editShapeLetterSpacing(letter_spacing);
}

void MainApplication::updateShapeBold(bool bold) {
  font_.setBold(bold);
  Q_EMIT editShapeBold(bold);
}

void MainApplication::updateShapeItalic(bool italic) {
  font_.setItalic(italic);
  Q_EMIT editShapeItalic(italic);
}

void MainApplication::updateShapeUnderline(bool underline) {
  font_.setUnderline(underline);
  Q_EMIT editShapeUnderline(underline);
}

void MainApplication::updateShapeLineHeight(double line_height) {
  line_height_ = line_height;
  Q_EMIT editShapeLineHeight(line_height);
}