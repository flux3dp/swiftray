#include <QApplication>
#include <QWidget>
#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include <QLocale>
#include <QTranslator>
#include <QDebug>
#include <canvas/canvas.h>
#include <window/osxwindow.h>
#include <window/mainwindow.h>

int mainCLI(int argc, char *argv[]) {
  qInfo() << "Vecty CLI interface";
  Canvas vcanvas;
  QFile file(argv[2]);
  Q_ASSERT_X(file.exists(), "Vecty CLI", "File not found");
  Q_ASSERT_X(file.open(QFile::ReadOnly), "Vecty CLI", "Can not open the file");
  QByteArray data = file.readAll();
  vcanvas.loadSVG(data);
  vcanvas.exportGcode();
  return 0;
}

int main(int argc, char *argv[]) {
  QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  QApplication app(argc, argv);

  QCoreApplication::setOrganizationName("FLUX");
  QCoreApplication::setOrganizationDomain("flux3dp.com");
  QCoreApplication::setApplicationName("beambird");

  if (argc > 1 && strcmp(argv[1], "cli") == 0) {
    return mainCLI(argc, argv);
  }
  app.setAttribute(Qt::AA_UseHighDpiPixmaps);

  //app.setStyle("fusion");

  // Force anti-aliasing
  QSurfaceFormat format = QSurfaceFormat::defaultFormat();
  format.setSamples(8);
  QSurfaceFormat::setDefaultFormat(format);

  QTranslator translator;
  const QStringList uiLanguages = QLocale::system().uiLanguages();

  for (const QString &locale : uiLanguages) {
    const QString baseName = "vecty_" + QLocale(locale).name();

    if (translator.load(":/i18n/" + baseName)) {
      app.installTranslator(&translator);
      break;
    }
  }

  qmlRegisterType<Canvas>("Vecty", 1, 0, "Canvas");
  /*QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  QGuiApplication app(argc, argv);
  QQmlApplicationEngine engine;
  const QUrl url(QStringLiteral("qrc:/main.qml"));
  QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
  &app, [url](QObject * obj, const QUrl & objUrl) {
      if (!obj && url == objUrl)
          QCoreApplication::exit(-1);
  }, Qt::QueuedConnection);
  engine.load(url);*/
  QCoreApplication::setOrganizationName("QtProject");
  QCoreApplication::setApplicationName("Application Example");
  QCoreApplication::setApplicationVersion(QT_VERSION_STR);
  MainWindow win;
  win.show();
  setOSXWindowTitleColor(&win);
  return app.exec();
}
