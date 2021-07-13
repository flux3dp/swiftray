#ifndef BASE_CONTAINER_H
#define BASE_CONTAINER_H


/**
    \class BaseContainer
    \brief A class template for widget containers
*/
class BaseContainer {

public:
  BaseContainer() = default;

  void initializeContainer() {
    loadWidgets();
    loadStyles();
    loadSettings();
    registerEvents();
  }

protected:
  
  /** Overridable function for "Adding widgets programmatically" */
  virtual void loadWidgets() {};

  /** Overridable function for "Set styles or load specific QSS" */
  virtual void loadStyles() {};

  /** Overridable function for "Load preferences" */
  virtual void loadSettings() {};

  /** Overridable function for "Connect events" */
  virtual void registerEvents() {};
};

#endif