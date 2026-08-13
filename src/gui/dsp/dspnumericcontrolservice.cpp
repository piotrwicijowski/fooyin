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

#include "dspnumericcontrolservice.h"

#include "dspsettingsregistry.h"

#include <core/engine/dsp/dspchainstore.h>
#include <core/engine/dsp/dspnode.h>

#include <algorithm>

namespace Fooyin {
DspNumericControlServiceImpl::DspNumericControlServiceImpl(DspChainStore* chainStore, DspSettingsRegistry* registry)
    : m_chainStore{chainStore}
    , m_registry{registry}
{ }

bool DspNumericControlServiceImpl::supportsNumericControl(const QString& dspId) const
{
    return numericProviderFor(dspId) != nullptr;
}

std::vector<DspNumericTarget> DspNumericControlServiceImpl::targetsFor(const QString& dspId) const
{
    std::vector<DspNumericTarget> targets;
    if(!m_chainStore) {
        return targets;
    }

    const auto* provider = numericProviderFor(dspId);
    if(!provider) {
        return targets;
    }

    const auto chain                = m_chainStore->activeChain();
    const auto control              = provider->numericControlInfo();
    const bool supportsLiveSettings = [this, &dspId]() {
        if(auto node = m_chainStore->createDsp(dspId)) {
            return node->supportsLiveSettings();
        }
        return false;
    }();
    const auto appendMatchingTargets = [&targets, provider, &control, supportsLiveSettings,
                                        &dspId](const Engine::DspChain& subChain, const Engine::DspChainScope scope) {
        for(const auto& entry : subChain) {
            if(entry.id != dspId) {
                continue;
            }

            const auto value = provider->numericValue(entry.settings);
            if(!value.has_value()) {
                continue;
            }

            targets.push_back(DspNumericTarget{
                .scope        = scope,
                .instanceId   = entry.instanceId,
                .dspId        = entry.id,
                .name         = entry.name.isEmpty() ? entry.id : entry.name,
                .enabled      = entry.enabled,
                .supportsLive = supportsLiveSettings,
                .value        = *value,
                .control      = control,
            });
        }
    };

    appendMatchingTargets(chain.masterChain, Engine::DspChainScope::Master);
    appendMatchingTargets(chain.perTrackChain, Engine::DspChainScope::PerTrack);

    return targets;
}

bool DspNumericControlServiceImpl::setValue(const Engine::DspChainScope scope, const uint64_t instanceId,
                                            const double value, const bool persist)
{
    if(!m_chainStore || instanceId == 0) {
        return false;
    }

    const QString dspId  = dspIdFor(scope, instanceId);
    const auto* provider = numericProviderFor(dspId);
    if(!provider) {
        return false;
    }

    return m_chainStore->updateLiveDspSettings(scope, instanceId, provider->settingsWithNumericValue(value), persist);
}

const DspNumericSettingsProvider* DspNumericControlServiceImpl::numericProviderFor(const QString& dspId) const
{
    if(!m_registry) {
        return nullptr;
    }

    auto* provider = m_registry->providerFor(dspId);
    return dynamic_cast<const DspNumericSettingsProvider*>(provider);
}

QString DspNumericControlServiceImpl::dspIdFor(const Engine::DspChainScope scope, const uint64_t instanceId) const
{
    if(!m_chainStore) {
        return {};
    }

    const auto chain        = m_chainStore->activeChain();
    const auto& targetChain = scope == Engine::DspChainScope::Master ? chain.masterChain : chain.perTrackChain;
    const auto it           = std::ranges::find(targetChain, instanceId, &Engine::DspDefinition::instanceId);
    return it == targetChain.cend() ? QString{} : it->id;
}
} // namespace Fooyin
