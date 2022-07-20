#include <QApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QLocale>
#include <QSettings>
#include <QTranslator>
#include <QDebug>
#include <canvas/canvas.h>
#include <windows/osxwindow.h>
#include <windows/mainwindow.h>
#include <sentry/include/sentry.h>

#ifdef Q_OS_MACOS
#define MACOS
#endif

int mainCLI(int argc, char *argv[]) {
  qInfo() << "Swiftray CLI interface";
  Canvas vcanvas;
  QFile file(argv[2]);
  Q_ASSERT_X(file.exists(), "Swiftray CLI", "File not found");
  Q_ASSERT_X(file.open(QFile::ReadOnly), "Swiftray CLI", "Can not open the file");
  QByteArray data = file.readAll();
  vcanvas.loadSVG(data);
  //vcanvas.exportGcode();
  return 0;
}

int main(int argc, char *argv[]) {
  // Launch Crashpad with Sentry
  sentry_options_t *options = sentry_options_new();
  sentry_options_set_dsn(options, "https://f27889563d3b4cefb80c5afaca760fdb@o28957.ingest.sentry.io/6586888");
  #ifdef Q_OS_MACOS
  sentry_options_set_handler_path(options, "../Resources/crashpad_handler");
  #endif
  sentry_options_set_release(options, "Swiftray@1.0.0");
  sentry_init(options);
  // Make sure everything flushes
  auto sentryClose = qScopeGuard([] { sentry_close(); });

  // Test event
  //sentry_capture_event(sentry_value_new_message_event(
  //  SENTRY_LEVEL_INFO, // level
  //  "custom",          // logger
  //  "It works!"        // message
  //));

  QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
  QApplication app(argc, argv);
  QCoreApplication::setOrganizationName("FLUX");
  QCoreApplication::setOrganizationDomain("flux3dp.com");
  QCoreApplication::setApplicationName("Swiftray");
  QCoreApplication::setApplicationVersion(QT_VERSION_STR);

  // CLI
  if (argc > 1 && strcmp(argv[1], "cli") == 0) {
    return mainCLI(argc, argv);
  }

  // Set app icon
  app.setWindowIcon(QIcon(":/images/icon.png"));

  // load Open Sans font(addApplicationFont fail in Mac)
  #ifdef Q_OS_WIN
  int id = QFontDatabase::addApplicationFont(":/fonts/open-sans-latin.ttf");
  if(id != -1) {
    QFont font("Open Sans");
    font.setStyleHint(QFont::Monospace);
    font.setPixelSize(12);
    QApplication::setFont(font);
  }
  #endif
  
  // Force anti-aliasing
  
  /*QSurfaceFormat format = QSurfaceFormat::defaultFormat();
  format.setSamples(8);
  QSurfaceFormat::setDefaultFormat(format);*/

  // Set translator
  QSettings settings;
  QString locale;
  QVariant language_code = settings.value("window/language", 0);

  switch(language_code.toInt()) {
    case 0:
      locale = "en-US";
      break;
    case 1:
      locale = "zh-Hant-TW";
      break;
    default:
      locale = "en-US";
      break;
  }

  QTranslator translator;
  translator.load(":/i18n/" + locale);
  app.installTranslator(&translator);

  // Load Canvas to QML Engine
  qmlRegisterType<Canvas>("Swiftray", 1, 0, "Canvas");
  
  // Load MainWindow
  MainWindow win;
  win.show();
  return app.exec();
}
