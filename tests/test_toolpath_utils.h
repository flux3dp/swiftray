#pragma once

#include <gtest/gtest.h>
#include <toolpath_exporter/toolpath-utils.h>

TEST(ToolpathUtilsTest, RectBordersAreAlwaysRightBottomExclusive) {
  // QRect::right()/bottom() would report 6/8 here (the last pixel).
  const RectBorders borders = getRectBorders(QRect(4, 7, 3, 2));
  EXPECT_EQ(borders.left, 4);
  EXPECT_EQ(borders.top, 7);
  EXPECT_EQ(borders.right_exclusive, 7);
  EXPECT_EQ(borders.bottom_exclusive, 9);
}

TEST(ToolpathUtilsTest, ImageBBoxSnapsOutwardAndClipsToLimit) {
  // Partially covered edge pixels must stay in the box...
  EXPECT_EQ(getImageBBox(QRectF(4.25, 7.5, 3.5, 2.25), QRect(0, 0, 100, 100)),
            QRect(4, 7, 4, 3));
  // ...but never past the addressable area.
  EXPECT_EQ(getImageBBox(QRectF(-1.5, -2.5, 20, 20), QRect(0, 0, 10, 10)),
            QRect(0, 0, 10, 10));
}

TEST(ToolpathUtilsTest, ImageBBoxIsEmptyWhenFullyOutsideLimit) {
  EXPECT_TRUE(getImageBBox(QRectF(-40, -40, 20, 20), QRect(0, 0, 10, 10))
                  .isEmpty());
}
