/*
 *  Kaidan - A user-friendly XMPP client for every device!
 *
 *  Copyright (C) 2016-2019 Kaidan developers and contributors
 *  (see the LICENSE file for a full list of copyright authors)
 *
 *  Kaidan is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  In addition, as a special exception, the author of Kaidan gives
 *  permission to link the code of its release with the OpenSSL
 *  project's "OpenSSL" library (or with modified versions of it that
 *  use the same license as the "OpenSSL" library), and distribute the
 *  linked executables. You must obey the GNU General Public License in
 *  all respects for all of the code used other than "OpenSSL". If you
 *  modify this file, you may extend this exception to your version of
 *  the file, but you are not obligated to do so.  If you do not wish to
 *  do so, delete this exception statement from your version.
 *
 *  Kaidan is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kaidan.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Utils.h"
#include <QColor>
#include <QClipboard>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QMimeDatabase>
#include <QUrl>
#include <QStandardPaths>
#include "qxmpp-exts/QXmppColorGenerator.h"

Utils::Utils(QObject *parent)
        : QObject(parent)
{
}

QString Utils::getResourcePath(const QString &name) const
{
	// We generally prefer to first search for files in application resources
	if (QFile::exists(":/" + name))
		return QString("qrc:/" + name);

	// list of file paths where to search for the resource file
	QStringList pathList;
	// add relative path from binary (only works if installed)
	pathList << QCoreApplication::applicationDirPath() + QString("/../share/") + QString(APPLICATION_NAME);
	// get the standard app data locations for current platform
	pathList << QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);
#ifdef UBUNTU_TOUCH
	pathList << QString("./share/") + QString(APPLICATION_NAME);
#endif
#ifndef NDEBUG
#ifdef DEBUG_SOURCE_PATH
	// add source directory (only for debug builds)
	pathList << QString(DEBUG_SOURCE_PATH) + QString("/data");
#endif
#endif

	// search for file in directories
	for (int i = 0; i < pathList.size(); i++) {
		// open directory
		QDir directory(pathList.at(i));
		// look up the file
		if (directory.exists(name)) {
			// found the file, return the path
			return QUrl::fromLocalFile(directory.absoluteFilePath(name)).toString();
		}
	}

	// no file found
	qWarning() << "[main] Could NOT find media file:" << name;
	return "";
}

bool Utils::isImageFile(const QUrl &fileUrl) const
{
	QMimeType type = QMimeDatabase().mimeTypeForUrl(fileUrl);
	return type.inherits("image/jpeg") || type.inherits("image/png");
}

void Utils::copyToClipboard(const QString &text) const
{
	QGuiApplication::clipboard()->setText(text);
}

QString Utils::fileNameFromUrl(const QUrl &url) const
{
	return QUrl(url).fileName();
}

QString Utils::fileSizeFromUrl(const QUrl &url) const
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0) // Qt 5.10 or later
	return QLocale::system().formattedDataSize(QFileInfo(QUrl(url).toLocalFile())
	                                           .size());
#else
	// before Qt 5.10 there was no formattedDataSize() method:
	// sizes will always be in MiB
	double size = QFileInfo(QUrl(url).toLocalFile()).size();
	return QString::number(qRound(size / 1024.0 / 10.24) / 100.0).append(" MiB");
#endif
}

QString Utils::formatMessage(const QString &message) const
{
	// escape all special XML chars (like '<' and '>')
	// and spilt into words for processing
	return processMsgFormatting(message.toHtmlEscaped().split(" "));
}

QColor Utils::getUserColor(const QString &nickName) const
{
	QXmppColorGenerator::RGBColor color = QXmppColorGenerator::generateColor(nickName);
	return QColor(color.red, color.green, color.blue);
}

QString Utils::processMsgFormatting(const QStringList &list, bool isFirst) const
{
	if (list.isEmpty())
		return "";

	// link highlighting
	if (list.first().startsWith("https://") || list.first().startsWith("http://"))
		return (isFirst ? "" : " ") + QString("<a href='%1'>%1</a>").arg(list.first())
		       + processMsgFormatting(list.mid(1), false);

	return (isFirst ? "" : " ") + list.first() + processMsgFormatting(list.mid(1), false);
}
