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

#pragma once

#include "fygui_export.h"

#include <core/engine/enginedefs.h>

#include <QByteArray>
#include <QString>

#include <cstdint>
#include <optional>
#include <vector>

namespace Fooyin {
struct FYGUI_EXPORT DspNumericControlInfo
{
    double minValue{0.0};
    double maxValue{0.0};
    double step{0.0};
    int decimals{2};
    QString suffix;
};

struct FYGUI_EXPORT DspNumericTarget
{
    Engine::DspChainScope scope{Engine::DspChainScope::Master};
    uint64_t instanceId{0};
    QString dspId;
    QString name;
    bool enabled{true};
    bool supportsLive{false};
    double value{0.0};
    DspNumericControlInfo control;
};

/*! Converts DSP-specific settings payloads to/from a single public numeric value. */
class FYGUI_EXPORT DspNumericSettingsProvider
{
public:
    virtual ~DspNumericSettingsProvider() = default;

    [[nodiscard]] virtual DspNumericControlInfo numericControlInfo() const                     = 0;
    [[nodiscard]] virtual std::optional<double> numericValue(const QByteArray& settings) const = 0;
    [[nodiscard]] virtual QByteArray settingsWithNumericValue(double value) const              = 0;
};

/*! Public GUI-plugin service for discovering and updating numeric DSP parameters. */
class FYGUI_EXPORT DspNumericControlService
{
public:
    virtual ~DspNumericControlService() = default;

    [[nodiscard]] virtual bool supportsNumericControl(const QString& dspId) const                       = 0;
    [[nodiscard]] virtual std::vector<DspNumericTarget> targetsFor(const QString& dspId) const          = 0;
    virtual bool setValue(Engine::DspChainScope scope, uint64_t instanceId, double value, bool persist) = 0;
};
} // namespace Fooyin
