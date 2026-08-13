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

#include <gui/dsp/dspnumericcontrol.h>

namespace Fooyin {
class DspChainStore;
class DspSettingsRegistry;

class DspNumericControlServiceImpl final : public DspNumericControlService
{
public:
    DspNumericControlServiceImpl(DspChainStore* chainStore, DspSettingsRegistry* registry);

    [[nodiscard]] bool supportsNumericControl(const QString& dspId) const override;
    [[nodiscard]] std::vector<DspNumericTarget> targetsFor(const QString& dspId) const override;
    bool setValue(Engine::DspChainScope scope, uint64_t instanceId, double value, bool persist) override;

private:
    [[nodiscard]] const DspNumericSettingsProvider* numericProviderFor(const QString& dspId) const;
    [[nodiscard]] QString dspIdFor(Engine::DspChainScope scope, uint64_t instanceId) const;

    DspChainStore* m_chainStore;
    DspSettingsRegistry* m_registry;
};
} // namespace Fooyin
