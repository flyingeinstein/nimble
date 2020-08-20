#pragma once

#include "NimbleConfig.h"
#include "NimbleEnum.h"

namespace Nimble {

class ModuleConfig
{
    public:
        /// @todo fill with methods to load/store configuration
        /// A ModuleConfig can return new ModuleConfig objects that reference internal json or yaml nodes

        virtual bool nextConfig(ModuleConfig& config);
};

} // ns:Nimble

