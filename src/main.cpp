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

#ifdef ENABLE_SENTRY
#include <sentry.h>
#endif

#ifdef Q_OS_MACOS
#include <osx/disable-app-nap.h>
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
  qInfo() << "Swiftray starting..."  << getDebugTime();
  qInfo() << "Qt version:" << QT_VERSION_STR;
  QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
  MainApplication app(argc, argv);
  QCoreApplication::setOrganizationName("FLUX");
  QCoreApplication::setOrganizationDomain("flux3dp.com");
  QCoreApplication::setApplicationName("Swiftray");
  QCoreApplication::setApplicationVersion(QT_VERSION_STR);
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
    return mainCLI(argc, argv);
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
