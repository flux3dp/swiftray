#include <QApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QLocale>
#include <QSettings>
#include <QTranslator>
#include <QtGlobal>
#include <QDebug>
#include <main_application.h>
#include <canvas/canvas.h>
#include <windows/osxwindow.h>
#include <windows/mainwindow.h>
#include <string>
#include <debug/debug-timer.h>
#include <config.h>
#include <utils/executable_path.h>

#ifdef ENABLE_SENTRY
#include <sentry.h>
#endif

#ifdef Q_OS_MACOS
#include <osx/disable-app-nap.h>
#endif

int handle_cli_mode(int argc, char *argv[]) {
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

void init_debugger() {
  // Launch Crashpad with Sentry
  sentry_options_t *options_ = sentry_options_new();
  QString database_path = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/sentry-native";
  sentry_options_set_database_path(options_, database_path.toStdString().c_str());
  sentry_options_set_dsn(options_, SENTRY_DSN);
  QString crashpad_path = QString::fromStdString(get_executable_dir() + "/crashpad_handler");
  #ifdef Q_OS_WIN
    crashpad_path += ".exe";
  #endif
  // Checking crashbad_handler exists
  qInfo() << "Sentry DSN:" << SENTRY_DSN;
  qInfo() << "Crashpad path:" << crashpad_path << "exist" << QFile::exists(crashpad_path);
  sentry_options_set_handler_path(options_, crashpad_path.toStdString().c_str());
  sentry_options_set_release(options_,
      std::string("Swiftray@")
      .append(VERSION_STRING)
      .c_str()
  );
  sentry_options_set_debug(options_, 1); // More details for debug
  sentry_options_set_require_user_consent(options_, true);
  sentry_init(options_);
  sentry_user_consent_give();
}

void cause_crash() {
    QString *str = new QString("Hello");
    str = nullptr;
    qInfo() << str->toStdString().c_str();
}


int main(int argc, char *argv[]) {
  qInfo() << "Swiftray Version:" << VERSION_STRING;
  qInfo() << "Qt Version:" << QT_VERSION_STR;

  QCoreApplication::setOrganizationName("FLUX");
  QCoreApplication::setOrganizationDomain("flux3dp.com");
  QCoreApplication::setApplicationName("Swiftray");
  QCoreApplication::setApplicationVersion(QT_VERSION_STR);
  QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

  MainApplication app(argc, argv);

  init_debugger();
  cause_crash();
  
  #ifdef Q_OS_MACOS
  disableAppNap();
  #endif
  #ifdef Q_OS_WIN
  QQuickStyle::setStyle("Fusion");
  #endif
  // Test event
  //sentry_capture_event(sentry_value_new_message_event(
  //  SENTRY_LEVEL_INFO, // level
  //  "custom",          // logger
  //  "It works!"        // message
  //));

  // CLI
  if (argc > 1 && strcmp(argv[1], "cli") == 0) {
    return handle_cli_mode(argc, argv);
  }

  if (argc > 1 && strcmp(argv[1], "--daemon") == 0) {
    // Daemon mode
    qInfo() << "Swiftray daemon mode";
    return app.exec();
  }

  // Set app icon
  app.setWindowIcon(QIcon(":/resources/images/icon.png"));

  QSettings settings("flux", "swiftray");
  // load Open Sans font(addApplicationFont fail in Mac)
  QVariant font_size = settings.value("window/font_size", 0);
  QFont current_font = QApplication::font();
  current_font.setPixelSize(font_size.toInt());
  QApplication::setFont(current_font);
  
  // Force anti-aliasing
  
  /*QSurfaceFormat format = QSurfaceFormat::defaultFormat();
  format.setSamples(8);
  QSurfaceFormat::setDefaultFormat(format);*/

  // Set translator
  QString locale;
  QVariant language_code = settings.value("window/language", 0);

  switch(language_code.toInt()) {
    case 0:
      locale = "en-US";
      break;
    case 1:
      locale = "zh-Hant-TW";
      break;
    case 2:
      locale = "ja-JP";
      break;
    default:
      locale = "en-US";
      break;
  }

#ifdef ENABLE_SENTRY
  // Make sure everything flushes
  auto sentryClose = qScopeGuard([] { sentry_close(); });
#endif

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
