// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "CascSourceReader.h"

#include "GameLocale.h"

#include <string>
#include <vector>

namespace
{
    std::vector<std::string> resourceCandidates(std::string_view resource, GameLocale locale)
    {
        const std::string path(resource);
        const std::string localeNamespace(toCascLocaleNamespace(locale));

        std::vector<std::string> candidates;
        candidates.push_back("locales/" + localeNamespace + "/Assets/SD/" + path);
        candidates.push_back("locales/" + localeNamespace + "/Assets/" + path);
        candidates.push_back("SD/" + path);

        candidates.push_back(path);

        return candidates;
    }
}

CascSourceReader::CascSourceReader(const LogicalSource &source): locale_(source.locale)
{
    if (source.format != SourceFormat::Casc) {
        return;
    }

    archive_.open(source.storage);
}

bool CascSourceReader::isOpen() const
{
    return archive_.isOpen();
}

bool CascSourceReader::contains(
    std::string_view resource
) const
{
    for (const std::string &candidate : resourceCandidates(resource, locale_))
    {
        if (archive_.contains(candidate)) {
            return true;
        }
    }

    return false;
}

bool CascSourceReader::extract(
    std::string_view resource,
    const std::filesystem::path &destination
) const
{
    for (const std::string &candidate : resourceCandidates(resource, locale_))
    {
        if (!archive_.contains(candidate)) {
            continue;
        }

        if (archive_.extract(candidate, destination)) {
            return true;
        }
    }

    return false;
}
