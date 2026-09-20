// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "MpqArchive.h"

#include <string>

MpqArchive::MpqArchive(const std::filesystem::path &path)
{
    open(path);
}

MpqArchive::~MpqArchive()
{
    close();
}

bool MpqArchive::open(const std::filesystem::path &path)
{
    close();

    const std::string filename = path.string();

    if (!SFileOpenArchive(
            filename.c_str(),
            0,
            STREAM_FLAG_READ_ONLY,
            &handle_))
    {
        handle_ = nullptr;
        return false;
    }

    return true;
}

void MpqArchive::close()
{
    if (handle_ != nullptr) {
        SFileCloseArchive(handle_);
        handle_ = nullptr;
    }
}

bool MpqArchive::isOpen() const
{
    return handle_ != nullptr;
}

bool MpqArchive::contains(std::string_view member) const
{
    if (!isOpen()) {
        return false;
    }

    HANDLE file = nullptr;

    const std::string memberName(member);

    if (!SFileOpenFileEx(handle_, memberName.c_str(), SFILE_OPEN_FROM_MPQ, &file)) {
        return false;
    }

    SFileCloseFile(file);

    return true;
}

bool MpqArchive::extract(std::string_view member, const std::filesystem::path &destination) const
{
    if (!isOpen()) {
        return false;
    }

    const std::string memberName(member);
    const std::string destinationName = destination.string();

    return SFileExtractFile(
        handle_,
        memberName.c_str(),
        destinationName.c_str(),
        SFILE_OPEN_FROM_MPQ
    );
}
