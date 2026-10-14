#pragma once

#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

#include <xbyak/xbyak.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace std::literals;

// wingdi.h #defines GetObject to GetObjectA, which breaks
// BGSDefaultObjectManager::GetObject (docs Trap B pattern).
#ifdef GetObject
#	undef GetObject
#endif
