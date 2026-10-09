// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "FilesystemSourceReader.h"

#include <algorithm>
#include <string>
#include <system_error>

namespace
{
    bool isInside(
        const std::filesystem::path &root,
        const std::filesystem::path &candidate
    )
    {
        const auto mismatch = std::mismatch(
            root.begin(), root.end(), candidate.begin(), candidate.end()
        );
        return mismatch.first == root.end();
    }
}

FilesystemSourceReader::FilesystemSourceReader(const LogicalSource &source)
{
    if (source.format != SourceFormat::Directory ||
        source.location != SourceLocation::Filesystem) {
        return;
    }

    std::error_code error;
    root_ = std::filesystem::canonical(source.storage, error);
    if (error) {
        root_.clear();
        return;
    }

    open_ = std::filesystem::is_directory(root_, error) && !error;
}

bool FilesystemSourceReader::isOpen() const
{
    return open_;
}

std::optional<std::filesystem::path> FilesystemSourceReader::resolve(
    std::string_view resource
) const
{
    if (
        !isOpen() ||
        resource.empty() ||
        resource.find('\0') != std::string_view::npos ||
        resource.find('\\') != std::string_view::npos ||
        resource.find(':') != std::string_view::npos
    ) {
        return std::nullopt;
    }

    const std::filesystem::path relative{std::string(resource)};
    if (relative.is_absolute() || relative.has_root_path()) {
        return std::nullopt;
    }

    for (const auto &part : relative) {
        if (part == ".." || part == "." || part.empty()) {
            return std::nullopt;
        }
    }

    std::error_code error;
    const auto candidate = std::filesystem::canonical(root_ / relative, error);
    if (error || !isInside(root_, candidate)) {
        return std::nullopt;
    }

    if (!std::filesystem::is_regular_file(candidate, error) || error) {
        return std::nullopt;
    }

    return candidate;
}

bool FilesystemSourceReader::contains(std::string_view resource) const
{
    return resolve(resource).has_value();
}

bool FilesystemSourceReader::extract(
    std::string_view resource,
    const std::filesystem::path &destination
) const
{
    const auto source = resolve(resource);
    if (!source) {
        return false;
    }

    std::error_code error;
    return std::filesystem::copy_file(
        *source,
        destination,
        std::filesystem::copy_options::overwrite_existing,
        error
    ) && !error;
}
