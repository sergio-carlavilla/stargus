// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "MpqSourceReader.h"

#include <algorithm>
#include <system_error>

MpqSourceReader::MpqSourceReader(const LogicalSource &source)
{
    open(source);
}

MpqSourceReader::~MpqSourceReader()
{
    archive_.close();
    cleanup();
}

bool MpqSourceReader::open(const LogicalSource &source)
{
    if (source.format != SourceFormat::Mpq) {
        return false;
    }

    /*
     * Direct physical MPQ
     */
    if (source.location == SourceLocation::Filesystem) {
        return archive_.open(source.storage);
    }

    /*
     * Nested MPQ:
     *
     *     game
     *       -> files\stardat.mpq
     *          inside INSTALL.EXE
     */
    if (source.location != SourceLocation::ArchiveMember){
        return false;
    }

    MpqArchive parent(source.storage);

    if (!parent.isOpen()) {
        return false;
    }

    std::error_code error;

    const std::filesystem::path temporaryDirectory = std::filesystem::temp_directory_path(error);
    if (error) {
        return false;
    }

    temporaryArchive_ = temporaryDirectory / ("stargus-startool-" + source.id + ".mpq");

    std::filesystem::remove(
        temporaryArchive_,
        error
    );

    error.clear();
    if (!parent.extract(source.member, temporaryArchive_)) {
        cleanup();
        return false;
    }

    if (!archive_.open(temporaryArchive_)) {
        cleanup();
        return false;
    }

    return true;
}

bool MpqSourceReader::isOpen() const
{
    return archive_.isOpen();
}

bool MpqSourceReader::contains(std::string_view resource) const
{
    if (!isOpen()) {
        return false;
    }

    return archive_.contains(
        toMpqPath(resource)
    );
}

bool MpqSourceReader::extract(std::string_view resource, const std::filesystem::path &destination) const
{
    if (!isOpen()) {
        return false;
    }

    return archive_.extract(
        toMpqPath(resource),
        destination
    );
}

void MpqSourceReader::cleanup()
{
    if (temporaryArchive_.empty()) {
        return;
    }

    std::error_code error;
    std::filesystem::remove(
        temporaryArchive_,
        error
    );

    temporaryArchive_.clear();
}

std::string MpqSourceReader::toMpqPath(std::string_view path)
{
    std::string result(path);

    std::replace(
        result.begin(),
        result.end(),
        '/',
        '\\'
    );

    return result;
}
