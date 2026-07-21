// theme.h
#pragma once
#include <QString>

inline QString getBlackFoxQSS() {
  // Colors taken from blackfox.json for Zed (my current IDE)
  return R"(
        /* --- Base colors and windows --- */
        QMainWindow, QWidget {
            background-color: #1E1F22;
            color: #C2D1E0;
            font-family: "Inter", "Cantarell", "Segoe UI", sans-serif;
            font-size: 10pt;
        }

        /* --- Sidebar (File Tree) --- */
        QTreeWidget {
            background-color: #2B2D30;
            border: none;
            border-right: 1px solid #292A2E;
            color: #C2D1E0;
            outline: none;
        }
        QTreeWidget::item {
            padding: 4px 8px;
            border: none;
        }
        QTreeWidget::item:hover {
            background-color: #393D3F;
        }
        QTreeWidget::item:selected {
            background-color: #484A51;
            color: #A5D0F8; /* Accent blue from JSON */
        }
        QTreeWidget::branch {
            background-color: #2B2D30;
        }

        /* --- Console and Text Fields --- */
        QTextEdit, QPlainTextEdit {
            background-color: #1E1F22;
            color: #C2D1E0;
            border: 1px solid #292A2E;
            selection-background-color: #484A51;
            selection-color: #C2D1E0;
        }

        /* --- Input Fields (Commands, Host, Port) --- */
        QLineEdit, QSpinBox {
            background-color: #2B2D30;
            color: #C2D1E0;
            border: 1px solid #292A2E;
            border-radius: 4px;
            padding: 4px 8px;
            selection-background-color: #A5D0F8;
            selection-color: #1E1F22;
        }
        QLineEdit:focus, QSpinBox:focus {
            border: 1px solid #9FB3C6; /* border.focused from JSON */
        }

        /* --- Buttons and Menu (Status Bar) --- */
        QPushButton {
            background-color: #393D3F;
            color: #C2D1E0;
            border: 1px solid #292A2E;
            border-radius: 4px;
            padding: 4px 12px;
        }
        QPushButton:hover {
            background-color: #484A51;
        }
        QPushButton:pressed {
            background-color: #2B2D30;
        }
        QPushButton:flat {
            background-color: transparent;
            border: none;
        }
        QPushButton:flat:hover {
            background-color: #393D3F;
        }

        QMenu {
            background-color: #2B2D30;
            border: 1px solid #292A2E;
            color: #C2D1E0;
            padding: 4px;
        }
        QMenu::item:selected {
            background-color: #393D3F;
            color: #A5D0F8;
            border-radius: 3px;
        }

        /* --- Scrollbars (Making them thin and aesthetic) --- */
        QScrollBar:vertical {
            background: #1E1F22;
            width: 10px;
            margin: 0px;
            border: none;
        }
        QScrollBar::handle:vertical {
            background: #525456;
            min-height: 20px;
            border-radius: 5px;
            margin: 2px;
        }
        QScrollBar::handle:vertical:hover {
            background: #6c7086;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }

        QScrollBar:horizontal {
            background: #1E1F22;
            height: 10px;
            margin: 0px;
            border: none;
        }
        QScrollBar::handle:horizontal {
            background: #525456;
            min-width: 20px;
            border-radius: 5px;
            margin: 2px;
        }
        QScrollBar::handle:horizontal:hover {
            background: #6c7086;
        }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0px;
        }

        /* --- Splitters --- */
        QSplitter::handle {
            background-color: #292A2E;
        }
        QSplitter::handle:horizontal {
            width: 2px;
        }
        QSplitter::handle:vertical {
            height: 2px;
        }

        /* --- Tabs and Menu Bar (If added) --- */
        QMenuBar {
            background-color: #1E1F22;
            color: #C2D1E0;
            border-bottom: 1px solid #292A2E;
        }
        QMenuBar::item:selected {
            background-color: #393D3F;
        }
        QMenu::separator {
            height: 1px;
            background: #292A2E;
            margin: 4px 8px;
        }

        /* --- Status Bar --- */
        QStatusBar {
            background-color: #1E1F22;
            border-top: 1px solid #292A2E;
            color: #bac2de;
        }
        QStatusBar::item {
            border: none;
        }
    )";
}
