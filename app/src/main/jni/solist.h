#pragma once

#include <set>

namespace Solist {
    std::set<std::string_view> FindPathsFromSolist(std::string_view keyword);

    std::string FindZygiskFromPreloads();
}
