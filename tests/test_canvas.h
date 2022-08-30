#pragma once

#include <windows/mainwindow.h>
#include <gtest/gtest.h>
#include <QByteArray>
#include <QDebug>
#include <toolpath_exporter/generators/gcode-generator.h>
#include <toolpath_exporter/toolpath-exporter.h>

using namespace testing;
TEST(Canvas, Init) {
  Canvas canvas;
}

TEST(Canvas, Layers) {
  Canvas canvas;
  EXPECT_EQ(canvas.document().layers().count(), 1);
  canvas.addEmptyLayer();
  EXPECT_EQ(canvas.document().layers().count(), 2);
  canvas.addEmptyLayer();
  EXPECT_EQ(canvas.document().layers().count(), 3);
}

TEST(Canvas, Rectangle) {
  Canvas canvas;
  QByteArray data = QString(
       "<svg width=\"100\" height=\"100\" xmlns=\"http://www.w3.org/2000/svg\"><rect width=\"100\" height=\"100\" style=\"stroke:rgb(0,0,0);\" /></svg>").toUtf8();
  canvas.loadSVG(data);
  MachineSettings::MachineSet mach;
  GCodeGenerator gen(mach);
  ToolpathExporter exporter(&gen);
  exporter.convertStack(canvas.document().layers());
  EXPECT_EQ(
       "M5\n$H\nG90\nM3S300\nM5\nM3S300\nG1F6000S0\nG1F1200S300\nG1X5\nG1X10\nG1Y5\nG1Y10\nG1X5\nG1X0\nG1Y5\nG1Y0\nM5\n$H\n",
       gen.toString());
}