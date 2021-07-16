#pragma once

#include <windows/mainwindow.h>
#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

using namespace testing;

TEST(GUI, MainWindow) {
  qmlRegisterType<Canvas>("Vecty", 1, 0, "Canvas");
  MainWindow *win = new MainWindow;
  win->show();
}