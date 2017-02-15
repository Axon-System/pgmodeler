/*
# PostgreSQL Database Modeler (pgModeler)
#
# Copyright 2006-2016 - Raphael Araújo e Silva <raphael@pgmodeler.com.br>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation version 3.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# The complete text of GPLv3 is at LICENSE file on source code root directory.
# Also, you can get the complete GNU General Public License at <http://www.gnu.org/licenses/>
*/

/**
\ingroup libpgmodeler_ui
\class NumberedTextEditor
\brief Implements a simple text editor with a line number block attached to it. This class is
based upon the code editor example provided by Qt
*/

#ifndef NUMBERED_TEXT_EDITOR_H
#define NUMBERED_TEXT_EDITOR_H

#include <QPlainTextEdit>
#include <QMenu>
#include <QToolButton>
#include "linenumberswidget.h"

class NumberedTextEditor : public QPlainTextEdit {
	private:
		Q_OBJECT

		//! \brief Controls if line numbers must be visible or not
		static bool line_nums_visible,

		//! \brief Controls if current line must be highlighted
		highlight_lines;

		//! \brief Line highlight color
		static QColor line_hl_color;

		//! \brief Default font for texts
		static QFont default_font;

		//! \brief Default tab size for texts
		static int tab_width;

		//! \brief Widget used to expose document line numbers
		LineNumbersWidget *line_number_wgt;

		//! \brief Indicates if the source code can be loaded/reference from external files enabling the top_widget usage
		bool allow_ext_files;

		QWidget *top_widget;

		QToolButton *load_file_btn, *link_file_btn, *clear_btn;

		//! \brief Determines and returns the line numbers widget width
		int getLineNumbersWidth(void);

	protected:
		void resizeEvent(QResizeEvent *event);
		void keyPressEvent(QKeyEvent *event);

	public:
		NumberedTextEditor(QWidget * parent = 0, bool allow_ext_files = false);

		static void setDefaultFont(const QFont &font);
		static void setLineNumbersVisible(bool value);
		static void setHighlightLines(bool value);
		static void setLineHighlightColor(const QColor &color);
		static void setTabWidth(int value);
		static int getTabWidth(void);

		/*! brief Disable the custom context menu designed specifically for this class.
		This method is useful when the user needs to create another context menu that executes actions
		differents from the original ones */
		void setCustomContextMenuEnabled(bool enabled);

	private slots:
		void showContextMenu(void);

		void changeSelectionToLower(void);
		void changeSelectionToUpper(void);
		void changeSelectionCase(bool lower);

		void identSelectionRight(void);
		void identSelectionLeft(void);
		void identSelection(bool ident_right);

		void loadFile(bool only_ref = false);

	public slots:
		//! \brief Grabs the keyboard input and also highlight the current line
		void setFocus(void);

		//! \brief Draw the line numbers according to the current visible lines
		void updateLineNumbers(void);

		/*! \brief Configures the line numbers widget sizes (w,h) depending on the current
		visible lines and the text editor height */
		void updateLineNumbersSize(void);

		//! \brief Colors the background of the line where the cursor is
		void highlightCurrentLine(void);
};

#endif
