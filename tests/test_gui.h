#pragma once

#include <windows/mainwindow.h>
#include <gtest/gtest.h>

using namespace testing;

TEST(GUI, MainWindow) {
  qmlRegisterType<Canvas>("Swiftray", 1, 0, "Canvas");
  MainWindow *win = new MainWindow;
  win->show();
}