#include <gtest/gtest.h>
#include "test_gui.h"
#include "test_canvas.h"
#include <QApplication>

int main(int argc, char *argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  QApplication app(argc, argv);
  return RUN_ALL_TESTS();
}
