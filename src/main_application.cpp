#include "main_application.h"
#include <utils/software_update.h>

#include <QApplication>


MainApplication *mainApp = NULL;

MainApplication::MainApplication(int &argc,  char **argv) :
    QApplication(argc, argv)
{
  mainApp = this;

#if defined(HAVE_SOFTWARE_UPDATE) && defined(Q_OS_WIN)
  connect(this, &MainApplication::softwareUpdateQuit, this, &QApplication::quit, Qt::QueuedConnection);
#endif
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
