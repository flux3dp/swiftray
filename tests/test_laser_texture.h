#pragma once

#include <gtest/gtest.h>
#include <QImage>
#include <toolpath_exporter/toolpath-utils.h>

TEST(LaserTextureTest, NoiseWhiteStaysWhiteAndIsDeterministic) {
  LaserTextureParams params;  // defaults: mode 1 (noise)
  QImage img(20, 20, QImage::Format_Grayscale8);
  img.fill(Qt::white);
  applyLaserTexture(&img, params);
  for (int y = 0; y < img.height(); ++y)
    for (int x = 0; x < img.width(); ++x)
      EXPECT_EQ(qGray(img.pixel(x, y)), 255);

  QImage black_a(20, 20, QImage::Format_Grayscale8);
  black_a.fill(Qt::black);
  QImage black_b = black_a.copy();
  applyLaserTexture(&black_a, params);
  applyLaserTexture(&black_b, params);
  EXPECT_EQ(black_a, black_b);  // seeded noise: identical across runs
  QImage solid_black(20, 20, QImage::Format_Grayscale8);
  solid_black.fill(Qt::black);
  EXPECT_NE(black_a, solid_black);  // noise actually changed something
  bool varied = false;
  for (int x = 1; x < black_a.width() && !varied; ++x)
    varied = black_a.pixel(x, 0) != black_a.pixel(0, 0);
  EXPECT_TRUE(varied);
}

TEST(LaserTextureTest, StripeLightensOnlyStripeLines) {
  LaserTextureParams params;
  params.mode = 2;
  params.stripe_angle = 0;      // horizontal stripes
  params.stripe_interval = 1;   // every 1mm
  params.stripe_intensity = 50;
  params.pixel_size_x = params.pixel_size_y = 0.1;  // stripes on rows 0, 10, ...
  QImage img(20, 20, QImage::Format_Grayscale8);
  img.fill(Qt::black);
  applyLaserTexture(&img, params);
  EXPECT_EQ(qGray(img.pixel(5, 0)), 128);  // 50% ink removed on the line
  EXPECT_EQ(qGray(img.pixel(5, 10)), 128);
  EXPECT_EQ(qGray(img.pixel(5, 5)), 0);  // between lines untouched
}

TEST(LaserTextureTest, Argb32PreservesAlphaAndSkipsTransparent) {
  LaserTextureParams params;
  params.mode = 2;
  params.stripe_angle = 0;
  params.stripe_interval = 1;
  params.stripe_intensity = 100;
  params.pixel_size_x = params.pixel_size_y = 0.1;
  QImage img(4, 4, QImage::Format_ARGB32);
  img.fill(qRgba(0, 0, 0, 0));
  img.setPixel(1, 0, qRgba(0, 0, 0, 200));  // on a stripe line
  applyLaserTexture(&img, params);
  EXPECT_EQ(img.pixel(0, 0), qRgba(0, 0, 0, 0));      // transparent untouched
  EXPECT_EQ(img.pixel(1, 0), qRgba(255, 255, 255, 200));  // ink removed, alpha kept
}
