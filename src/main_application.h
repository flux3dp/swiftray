#ifndef MAIN_APPLICATION_H
#define MAIN_APPLICATION_H

#include <QApplication>
#include <config.h>


class MainApplication : public QApplication
{
  Q_OBJECT
public:
  explicit MainApplication(int &argc,  char **argv);
  ~MainApplication();

#if defined(HAVE_SOFTWARE_UPDATE) && defined(Q_OS_WIN)
  void rejectSoftwareUpdate() { software_update_ok_ = false; }
  bool softwareUpdateCanShutdown();
  void softwareUpdateShutdownRequest();
#endif

private:
#if defined(HAVE_SOFTWARE_UPDATE) && defined(Q_OS_WIN)
  bool software_update_ok_ = false;
#endif

private Q_SLOTS:
  void cleanup();

Q_SIGNALS:
#if defined(HAVE_SOFTWARE_UPDATE) && defined(Q_OS_WIN)
  // Each of these are called from a separate thread.
  void softwareUpdateRequested();
  void softwareUpdateClose();
  void softwareUpdateQuit();
#endif
};

extern MainApplication *mainApp;

#endif // MAIN_APPLICATION_H
