// SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only
#ifndef STARTTOOL_H
#define STARTTOOL_H

class StartoolApp
{

    public:
        int run(int argc, char **argv);

    private:
        int runInspect(int argc, char **argv) const;
        int runExtract(int argc, char **argv) const;
        int runVerify(int argc, char **argv) const;
        int runImport(int argc, char **argv) const;

        void printHelp() const;
        void printVersion() const;

};

#endif // STARTTOOL_H
