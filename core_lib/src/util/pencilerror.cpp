/*

Pencil2D - Traditional Animation Software
Copyright (C) 2005-2007 Patrick Corrieri & Pascal Naidon
Copyright (C) 2012-2020 Matthew Chiawen Chang

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

*/

#include "pencilerror.h"
#include <map>
#include <QSysInfo>
#include <QLocale>

DebugDetails::DebugDetails()
{
}

DebugDetails::DebugDetails(const QString& title)
{
    addSection(title);
}

void DebugDetails::addSection(const QString& title)
{
    mDetails.append({"- [" + title + "] -", mCurrentDepth, true });
    mCurrentDepth += 1;
}

void DebugDetails::collect(const DebugDetails& d)
{
    for (const Line& line : d.mDetails)
    {
        mDetails.append( { line.text, line.depth + mCurrentDepth, line.isSection });
    }
}

QString DebugDetails::str()
{
    appendSystemInfo();
    return render("\n", "  ");
}

QString DebugDetails::html()
{
    appendSystemInfo();
    return render("<br>", "&nbsp;&nbsp;");
}

QString DebugDetails::render(const QString& lineBreak, const QString& indentUnit) const
{
    QStringList lines;

    for (int i = 0; i < mDetails.size(); i++)
    {
        const Line& line = mDetails[i];
        if (line.isSection)
        {
            if (!lines.isEmpty() && !lines.last().isEmpty()) {
                lines.append("");
            }
            lines.append(indentUnit.repeated(line.depth) + line.text);
            if (!mDetails[i + 1].isSection)
                lines.append("");
        }
        else
        {
            lines.append(indentUnit.repeated(line.depth) + line.text);
        }
    }
    return lines.join(lineBreak);
}

DebugDetails& DebugDetails::operator<<(const QString& s)
{
    mDetails.append({ s, mCurrentDepth });
    return *this;
}

void DebugDetails::append(const QString &desc)
{
    *this << desc;
}

void DebugDetails::appendSystemInfo()
{
    if (mDetails.empty() || mDetails.last().text == "end")
        return;

    addSection("System Info");

    QString version(APP_VERSION);
    if (version.startsWith("99.0.0")) {
        append("Pencil2D version: " APP_VERSION " (nightly)");
    } else if (version == "0.0.0.0") {
        append("Pencil2D version: " APP_VERSION " (dev)");
    } else {
        append("Pencil2D version: " APP_VERSION " (stable)");
    }

    #if defined(GIT_EXISTS)
        append("Commit: " S__GIT_COMMIT_HASH);
    #endif
    append("Build ABI: " + QSysInfo::buildAbi());
    append("Kernel: " + QSysInfo::kernelType() + ", " + QSysInfo::kernelVersion());
    append("Operating System: " + QSysInfo::prettyProductName());
    append("Language: " + QLocale::system().name());
}

Status::Status(ErrorCode code)
    : mCode(code)
{
}

Status::Status(const ErrorCode code, const QString& description)
    : mCode(code)
    , mDescription(description)
{
}

Status::Status(Status::ErrorCode eCode, const DebugDetails& detailsList)
    : mCode(eCode)
    , mDetails(detailsList)
{
}

Status::Status(Status::ErrorCode eCode, const DebugDetails& detailsList, QString title, QString description)
    : mCode(eCode)
    , mTitle(title)
    , mDescription(description)
    , mDetails(detailsList)
{
}


QString Status::msg() const
{
    static std::map<ErrorCode, QString> msgMap =
    {
        // error messages.
        { OK,                    tr("Everything ok.") },
        { FAIL,                  tr("Ooops, Something went wrong.") },
        { FILE_NOT_FOUND,        tr("File doesn't exist.") },
        { ERROR_FILE_CANNOT_OPEN,    tr("Cannot open file.") },
        { ERROR_INVALID_XML_FILE,    tr("The file is not a valid xml document.") },
        { ERROR_INVALID_PENCIL_FILE, tr("The file is not valid pencil document.") },
    };

    auto it = msgMap.find(mCode);
    if (it == msgMap.end())
    {
        return msgMap[FAIL];
    }
    return msgMap[mCode];
}

bool Status::operator==(Status::ErrorCode code) const
{
    return (mCode == code);
}

bool Status::operator!=(Status::ErrorCode code) const
{
    return (mCode != code);
}
