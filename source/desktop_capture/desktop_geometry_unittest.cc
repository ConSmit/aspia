//
// Aspia Project
// Copyright (C) 2018 Dmitry Chapyshev <dmitry@aspia.ru>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.
//

#include <gtest/gtest.h>

#include "desktop_capture/desktop_geometry.h"

namespace aspia {

TEST(desktop_rect_test, union_between_two_non_empty_rects)
{
    DesktopRect rect = DesktopRect::makeLTRB(1, 1, 2, 2);
    rect.unionWith(DesktopRect::makeLTRB(-2, -2, -1, -1));
    ASSERT_TRUE(rect.isEqual(DesktopRect::makeLTRB(-2, -2, 2, 2)));
}

TEST(desktop_rect_test, union_with_empty_rect)
{
    DesktopRect rect = DesktopRect::makeWH(1, 1);
    rect.unionWith(DesktopRect());
    ASSERT_TRUE(rect.isEqual(DesktopRect::makeWH(1, 1)));

    rect = DesktopRect::makeXYWH(1, 1, 2, 2);
    rect.unionWith(DesktopRect());
    ASSERT_TRUE(rect.isEqual(DesktopRect::makeXYWH(1, 1, 2, 2)));

    rect = DesktopRect::makeXYWH(1, 1, 2, 2);
    rect.unionWith(DesktopRect::makeXYWH(3, 3, 0, 0));
    ASSERT_TRUE(rect.isEqual(DesktopRect::makeXYWH(1, 1, 2, 2)));
}

TEST(desktop_rect_test, empty_rect_union_with_non_empty_one)
{
    DesktopRect rect;
    rect.unionWith(DesktopRect::makeWH(1, 1));
    ASSERT_TRUE(rect.isEqual(DesktopRect::makeWH(1, 1)));

    rect = DesktopRect();
    rect.unionWith(DesktopRect::makeXYWH(1, 1, 2, 2));
    ASSERT_TRUE(rect.isEqual(DesktopRect::makeXYWH(1, 1, 2, 2)));

    rect = DesktopRect::makeXYWH(3, 3, 0, 0);
    rect.unionWith(DesktopRect::makeXYWH(1, 1, 2, 2));
    ASSERT_TRUE(rect.isEqual(DesktopRect::makeXYWH(1, 1, 2, 2)));
}

TEST(desktop_rect_test, empty_rect_union_with_empty_one)
{
    DesktopRect rect;
    rect.unionWith(DesktopRect());
    ASSERT_TRUE(rect.isEmpty());

    rect = DesktopRect::makeXYWH(1, 1, 0, 0);
    rect.unionWith(DesktopRect());
    ASSERT_TRUE(rect.isEmpty());

    rect = DesktopRect();
    rect.unionWith(DesktopRect::makeXYWH(1, 1, 0, 0));
    ASSERT_TRUE(rect.isEmpty());

    rect = DesktopRect::makeXYWH(1, 1, 0, 0);
    rect.unionWith(DesktopRect::makeXYWH(-1, -1, 0, 0));
    ASSERT_TRUE(rect.isEmpty());
}

TEST(desktop_rect_test, scale)
{
    DesktopRect rect = DesktopRect::makeXYWH(100, 100, 100, 100);
    rect.scale(1.1, 1.1);
    ASSERT_EQ(rect.top(), 100);
    ASSERT_EQ(rect.left(), 100);
    ASSERT_EQ(rect.width(), 110);
    ASSERT_EQ(rect.height(), 110);

    rect = DesktopRect::makeXYWH(100, 100, 100, 100);
    rect.scale(0.01, 0.01);
    ASSERT_EQ(rect.top(), 100);
    ASSERT_EQ(rect.left(), 100);
    ASSERT_EQ(rect.width(), 1);
    ASSERT_EQ(rect.height(), 1);

    rect = DesktopRect::makeXYWH(100, 100, 100, 100);
    rect.scale(1.1, 0.9);
    ASSERT_EQ(rect.top(), 100);
    ASSERT_EQ(rect.left(), 100);
    ASSERT_EQ(rect.width(), 110);
    ASSERT_EQ(rect.height(), 90);

    rect = DesktopRect::makeXYWH(0, 0, 100, 100);
    rect.scale(1.1, 1.1);
    ASSERT_EQ(rect.top(), 0);
    ASSERT_EQ(rect.left(), 0);
    ASSERT_EQ(rect.width(), 110);
    ASSERT_EQ(rect.height(), 110);

    rect = DesktopRect::makeXYWH(0, 100, 100, 100);
    rect.scale(1.1, 1.1);
    ASSERT_EQ(rect.top(), 100);
    ASSERT_EQ(rect.left(), 0);
    ASSERT_EQ(rect.width(), 110);
    ASSERT_EQ(rect.height(), 110);
}

}  // namespace webrtc
