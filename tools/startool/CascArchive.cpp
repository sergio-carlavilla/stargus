// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only

#include "CascArchive.h"

#include <array>
#include <fstream>
#include <string>
#include <system_error>

CascArchive::CascArchive(const std::filesystem::path &path)
{
    open(path);
}

CascArchive::~CascArchive()
{
    close();
}

bool CascArchive::open(const std::filesystem::path &path)
{
    close();

    const std::string storagePath = path.string();

    if (!CascOpenStorage(storagePath.c_str(), 0, &handle_)) {
        handle_ = nullptr;
        return false;
    }

    return true;
}

void CascArchive::close()
{
    if (handle_ != nullptr) {
        CascCloseStorage(handle_);
        handle_ = nullptr;
    }
}

bool CascArchive::isOpen() const
{
    return handle_ != nullptr;
}

bool CascArchive::contains(std::string_view member) const
{
    if (!isOpen()) {
        return false;
    }

    HANDLE file = nullptr;

    const std::string memberName(member);

    if (!CascOpenFile(handle_, memberName.c_str(), 0, 0, &file)) {
        return false;
    }

    CascCloseFile(file);

    return true;
}

bool CascArchive::extract(
    std::string_view member,
    const std::filesystem::path &destination
) const
{
    if (!isOpen()) {
        return false;
    }

    HANDLE file = nullptr;

    const std::string memberName(member);

    if (!CascOpenFile(handle_, memberName.c_str(), 0, 0, &file)) {
        return false;
    }

    const std::filesystem::path parent = destination.parent_path();
    if (!parent.empty()) {
        std::error_code error;

        std::filesystem::create_directories(
            parent,
            error
        );

        if (error) {
            CascCloseFile(file);
            return false;
        }
    }

    std::ofstream output(
        destination,
        std::ios::binary | std::ios::trunc
    );

    if (!output) {
        CascCloseFile(file);
        return false;
    }

    std::array<char, 0x10000> buffer{};

    bool success = true;

    while (true) {
        DWORD bytesRead = 0;

        if (!CascReadFile(file, buffer.data(), static_cast<DWORD>(buffer.size()), &bytesRead)) {
            success = false;
            break;
        }

        if (bytesRead == 0) {
            break;
        }

        output.write(buffer.data(), static_cast<std::streamsize>(bytesRead));
        if (!output) {
            success = false;
            break;
        }
    }

    CascCloseFile(file);

    if (!success) {
        output.close();

        std::error_code error;
        std::filesystem::remove(destination, error);

        return false;
    }

    return true;
}
