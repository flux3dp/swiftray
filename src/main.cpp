#include <QApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QLocale>
#include <QTranslator>
#include <QDebug>
#include <canvas/canvas.h>
#include <windows/osxwindow.h>
#include <windows/mainwindow.h>

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
  QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
  QApplication app(argc, argv);
  QCoreApplication::setOrganizationName("FLUX");
  QCoreApplication::setOrganizationDomain("flux3dp.com");
  QCoreApplication::setApplicationName("Beam Studio Native");
  QCoreApplication::setApplicationVersion(QT_VERSION_STR);

  // CLI
  if (argc > 1 && strcmp(argv[1], "cli") == 0) {
    return mainCLI(argc, argv);
  }

  // Set app icon
  app.setWindowIcon(QIcon(":/images/icon.png"));
  
  // Force anti-aliasing
  
  /*QSurfaceFormat format = QSurfaceFormat::defaultFormat();
  format.setSamples(8);
  QSurfaceFormat::setDefaultFormat(format);*/
  
  // Set translator
  QTranslator translator;
  const QStringList uiLanguages = QLocale::system().uiLanguages();

  for (const QString &locale : uiLanguages) {
    const QString baseName = QLocale(locale).name();
    qInfo() << "Loading language" << locale;

    if (translator.load(":/i18n/" + locale)) {
      qInfo() << "Success loaded";
      app.installTranslator(&translator);
      break;
    }
  }
  
  // Load Canvas to QML Engine
  qmlRegisterType<Canvas>("Vecty", 1, 0, "Canvas");
  
  // Load MainWindow
  MainWindow win;
  win.show();
#ifdef MACOS
  setOSXWindowTitleColor(&win);
#endif
  return app.exec();
}
