#ifndef BASE_CONTAINER_H
#define BASE_CONTAINER_H

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
  virtual void loadWidgets() {};

  virtual void loadStyles() {};

  virtual void loadSettings() {};

  virtual void registerEvents() {};
};

#endif