#pragma once

#include <QList>
#include <QPointer>

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

  void blockInputSignals() {
    for (auto obj : inputs_) {
      if (obj) obj->blockSignals(true);
    }
  };

  void unblockInputSignals() {
    for (auto obj : inputs_) {
      if (obj) obj->blockSignals(false);
    }
  };

  void addInput(std::initializer_list<QPointer<QObject>> input) {
    inputs_.append(input);
  }

  QList<QPointer<QObject>> inputs_;
};