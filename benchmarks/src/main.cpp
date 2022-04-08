/*

Pencil2D - Traditional Animation Software
Copyright (C) 2012-2020 Matthew Chiawen Chang

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

*/

#include <hayai/hayai.hpp>

#include <QApplication>
#include <QDebug>

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);

#ifdef DEBUG
    std::cout << "Warning: Program was compiled in debug mode, timings may not be accurate." << std::endl;
#endif

    hayai::ConsoleOutputter consoleOutputter;

    hayai::Benchmarker::AddOutputter(consoleOutputter);
    hayai::Benchmarker::RunAllTests();

    return 0;
}
