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

import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import org.kde.kirigami 2.0 as Kirigami

Popup {
    property var emojiModel
    property var textArea
    property string emojiCategory: "people"

    ColumnLayout {
        anchors.fill: parent

        GridView {
            Layout.fillWidth: true
            Layout.fillHeight: true

            cellWidth: Kirigami.Units.gridUnit * 2.5
            cellHeight: Kirigami.Units.gridUnit * 2.5

            boundsBehavior: Flickable.DragOverBounds

            clip: true

            model: emojiModel.model[emojiCategory]

            delegate: ItemDelegate {
                width: Kirigami.Units.gridUnit * 2
                height: Kirigami.Units.gridUnit * 2

                contentItem: Text {
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter

                    font.pointSize: 20
                    font.family: "NotoColorEmoji"
                    text: modelData.unicode
                }

                hoverEnabled: true
                ToolTip.text: modelData.shortname
                ToolTip.visible: hovered

                onClicked: textArea.insert(textArea.cursorPosition, modelData.unicode)
            }

            ScrollBar.vertical: ScrollBar {}
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 2

            color: Kirigami.Theme.highlightColor
        }

        Row {
            Repeater {
                model: ListModel {
                    ListElement { label: "😏"; category: "people" }
                    ListElement { label: "🌲"; category: "nature" }
                    ListElement { label: "🍛"; category: "food"}
                    ListElement { label: "🚁"; category: "activity" }
                    ListElement { label: "🚅"; category: "travel" }
                    ListElement { label: "💡"; category: "objects" }
                    ListElement { label: "🔣"; category: "symbols" }
                    ListElement { label: "🏁"; category: "flags" }
                }

                delegate: ItemDelegate {
                    width: Kirigami.Units.gridUnit * 2
                    height: Kirigami.Units.gridUnit * 2

                    contentItem: Text {
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter

                        font.pointSize: 20
                        font.family: "NotoColorEmoji"
                        text: label
                    }

                    hoverEnabled: true
                    ToolTip.text: category
                    ToolTip.visible: hovered

                    onClicked: emojiCategory = category
                }
            }
        }
    }
}
