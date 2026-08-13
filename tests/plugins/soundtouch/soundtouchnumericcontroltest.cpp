/*
 * Fooyin
 * Copyright © 2026, Luke Taylor <luket@pm.me>
 *
 * Fooyin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Fooyin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Fooyin.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "soundtouchsettingswidget.h"

#include <gtest/gtest.h>

using namespace Qt::StringLiterals;

namespace Fooyin::Testing {
TEST(SoundTouchNumericControlTest, EmptySettingsDecodeToDefaultTempo)
{
    SoundTouch::SoundTouchTempoSettingsProvider provider;

    const auto value = provider.numericValue({});
    ASSERT_TRUE(value.has_value());
    EXPECT_DOUBLE_EQ(*value, 1.0);
}

TEST(SoundTouchNumericControlTest, EncodedTempoValuesRoundTrip)
{
    SoundTouch::SoundTouchTempoSettingsProvider provider;

    const QByteArray settings = provider.settingsWithNumericValue(1.25);
    const auto value          = provider.numericValue(settings);
    ASSERT_TRUE(value.has_value());
    EXPECT_DOUBLE_EQ(*value, 1.25);
}

TEST(SoundTouchNumericControlTest, OutOfRangeValuesClampToTempoLimits)
{
    SoundTouch::SoundTouchTempoSettingsProvider provider;

    const auto lowValue = provider.numericValue(provider.settingsWithNumericValue(0.01));
    ASSERT_TRUE(lowValue.has_value());
    EXPECT_DOUBLE_EQ(*lowValue, 0.25);

    const auto highValue = provider.numericValue(provider.settingsWithNumericValue(8.0));
    ASSERT_TRUE(highValue.has_value());
    EXPECT_DOUBLE_EQ(*highValue, 4.0);
}

TEST(SoundTouchNumericControlTest, MetadataMatchesTempoWidgetContract)
{
    SoundTouch::SoundTouchTempoSettingsProvider provider;

    const auto info = provider.numericControlInfo();
    EXPECT_DOUBLE_EQ(info.minValue, 0.25);
    EXPECT_DOUBLE_EQ(info.maxValue, 4.0);
    EXPECT_DOUBLE_EQ(info.step, 0.01);
    EXPECT_EQ(info.decimals, 2);
    EXPECT_EQ(info.suffix, u"x"_s);
}
} // namespace Fooyin::Testing
