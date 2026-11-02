#pragma once

#include "FileOpener.hpp"

namespace Diligent::FileOpeners
{
    Opener CreatePrefabFileOpener(StatusReporter reportStatus);
}
