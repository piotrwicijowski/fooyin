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

#include "gui/dsp/dspnumericcontrolservice.h"

#include "core/engine/dsp/dspchainstore.h"
#include "core/engine/dsp/dspregistry.h"
#include "gui/dsp/dspsettingsregistry.h"

#include <gui/dsp/dspnumericcontrol.h>
#include <gui/dsp/dspsettingsprovider.h>
#include <utils/settings/settingsmanager.h>

#include <gtest/gtest.h>

#include <QDataStream>
#include <QIODevice>
#include <QTemporaryDir>

using namespace Qt::StringLiterals;

namespace Fooyin::Testing {
namespace {
constexpr auto NumericDspId = "test.dsp.numeric";

QByteArray encodeDouble(const double value)
{
    QByteArray settings;
    QDataStream stream{&settings, QIODevice::WriteOnly};
    stream.setVersion(QDataStream::Qt_6_0);
    stream << value;
    return settings;
}

class FakeNumericDsp final : public DspNode
{
public:
    [[nodiscard]] QString name() const override
    {
        return QStringLiteral("Test Numeric");
    }

    [[nodiscard]] QString id() const override
    {
        return QString::fromLatin1(NumericDspId);
    }

    [[nodiscard]] bool supportsLiveSettings() const override
    {
        return true;
    }

    void prepare(const AudioFormat& /*format*/) override { }
    void process(ProcessingBufferList& /*chunks*/) override { }
};

class FakeNumericSettingsProvider final : public DspSettingsProvider,
                                          public DspNumericSettingsProvider
{
public:
    [[nodiscard]] QString id() const override
    {
        return QString::fromLatin1(NumericDspId);
    }

    DspSettingsDialog* createSettingsWidget(QWidget* /*parent*/) override
    {
        return nullptr;
    }

    [[nodiscard]] DspNumericControlInfo numericControlInfo() const override
    {
        return {.minValue = 0.25, .maxValue = 4.0, .step = 0.01, .decimals = 2, .suffix = QStringLiteral("x")};
    }

    [[nodiscard]] std::optional<double> numericValue(const QByteArray& settings) const override
    {
        if(settings.isEmpty()) {
            return 1.0;
        }

        QDataStream stream{settings};
        stream.setVersion(QDataStream::Qt_6_0);
        double value{0.0};
        stream >> value;
        return stream.status() == QDataStream::Ok ? std::optional<double>{value} : std::nullopt;
    }

    [[nodiscard]] QByteArray settingsWithNumericValue(const double value) const override
    {
        return encodeDouble(value);
    }
};

class DspNumericControlServiceTest : public ::testing::Test
{
protected:
    DspNumericControlServiceTest()
        : m_settings{m_tempDir.filePath(QStringLiteral("dsp_numeric_control.ini"))}
        , m_chainStore{&m_settings, &m_dspRegistry}
        , m_service{&m_chainStore, &m_settingsRegistry}
    {
        EXPECT_TRUE(m_tempDir.isValid());

        m_dspRegistry.registerDsp({
            .id      = QString::fromLatin1(NumericDspId),
            .name    = QStringLiteral("Test Numeric"),
            .factory = []() { return std::make_unique<FakeNumericDsp>(); },
        });
        m_settingsRegistry.registerProvider(std::make_unique<FakeNumericSettingsProvider>());
    }

    void setChain(const Engine::DspChains& chain)
    {
        m_chainStore.setActiveChain(chain);
    }

    QTemporaryDir m_tempDir;
    SettingsManager m_settings;
    DspRegistry m_dspRegistry;
    DspChainStore m_chainStore;
    DspSettingsRegistry m_settingsRegistry;
    DspNumericControlServiceImpl m_service;
};
} // namespace

TEST_F(DspNumericControlServiceTest, SupportsRegisteredNumericProvider)
{
    EXPECT_TRUE(m_service.supportsNumericControl(QString::fromLatin1(NumericDspId)));
    EXPECT_FALSE(m_service.supportsNumericControl(QStringLiteral("test.dsp.unknown")));
}

TEST_F(DspNumericControlServiceTest, ReturnsActiveTargetsWithDecodedValues)
{
    Engine::DspChains chain;
    chain.masterChain.push_back(Engine::DspDefinition{.id          = QString::fromLatin1(NumericDspId),
                                                      .name        = QStringLiteral("Tempo"),
                                                      .hasSettings = true,
                                                      .enabled     = true,
                                                      .instanceId  = 101,
                                                      .settings    = encodeDouble(1.5)});
    setChain(chain);

    const auto targets = m_service.targetsFor(QString::fromLatin1(NumericDspId));
    ASSERT_EQ(targets.size(), 1U);
    EXPECT_EQ(targets.front().scope, Engine::DspChainScope::Master);
    EXPECT_EQ(targets.front().instanceId, 101U);
    EXPECT_EQ(targets.front().name, QStringLiteral("Test Numeric"));
    EXPECT_TRUE(targets.front().enabled);
    EXPECT_TRUE(targets.front().supportsLive);
    EXPECT_DOUBLE_EQ(targets.front().value, 1.5);
    EXPECT_DOUBLE_EQ(targets.front().control.minValue, 0.25);
    EXPECT_DOUBLE_EQ(targets.front().control.maxValue, 4.0);
    EXPECT_DOUBLE_EQ(targets.front().control.step, 0.01);
    EXPECT_EQ(targets.front().control.suffix, QStringLiteral("x"));
}

TEST_F(DspNumericControlServiceTest, ReturnsMultipleMatchingTargets)
{
    Engine::DspChains chain;
    chain.masterChain.push_back(Engine::DspDefinition{.id          = QString::fromLatin1(NumericDspId),
                                                      .name        = {},
                                                      .hasSettings = true,
                                                      .instanceId  = 101,
                                                      .settings    = encodeDouble(1.1)});
    chain.perTrackChain.push_back(Engine::DspDefinition{.id          = QString::fromLatin1(NumericDspId),
                                                        .name        = {},
                                                        .hasSettings = true,
                                                        .instanceId  = 202,
                                                        .settings    = encodeDouble(1.2)});
    setChain(chain);

    const auto targets = m_service.targetsFor(QString::fromLatin1(NumericDspId));
    ASSERT_EQ(targets.size(), 2U);
    EXPECT_EQ(targets.front().scope, Engine::DspChainScope::Master);
    EXPECT_EQ(targets.back().scope, Engine::DspChainScope::PerTrack);
}

TEST_F(DspNumericControlServiceTest, SetValuePersistsUpdatedSettings)
{
    Engine::DspChains chain;
    chain.masterChain.push_back(Engine::DspDefinition{.id          = QString::fromLatin1(NumericDspId),
                                                      .name        = {},
                                                      .hasSettings = true,
                                                      .instanceId  = 101,
                                                      .settings    = encodeDouble(1.0)});
    setChain(chain);

    ASSERT_TRUE(m_service.setValue(Engine::DspChainScope::Master, 101, 2.25, true));

    const auto targets = m_service.targetsFor(QString::fromLatin1(NumericDspId));
    ASSERT_EQ(targets.size(), 1U);
    EXPECT_DOUBLE_EQ(targets.front().value, 2.25);
}

TEST_F(DspNumericControlServiceTest, UnsupportedProvidersReturnNoTargets)
{
    Engine::DspChains chain;
    chain.masterChain.push_back(Engine::DspDefinition{.id          = QStringLiteral("test.dsp.unknown"),
                                                      .name        = {},
                                                      .hasSettings = true,
                                                      .instanceId  = 101,
                                                      .settings    = encodeDouble(1.0)});
    setChain(chain);

    EXPECT_TRUE(m_service.targetsFor(QStringLiteral("test.dsp.unknown")).empty());
    EXPECT_FALSE(m_service.setValue(Engine::DspChainScope::Master, 101, 2.0, true));
}
} // namespace Fooyin::Testing
