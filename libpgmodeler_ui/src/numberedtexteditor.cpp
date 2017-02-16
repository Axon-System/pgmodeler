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

#include "numberedtexteditor.h"
#include "generalconfigwidget.h"
#include "pgmodeleruins.h"
#include "pgmodelerns.h"
#include <QFileDialog>
#include <QTextBlock>
#include <QScrollBar>
#include <QDebug>

bool NumberedTextEditor::line_nums_visible=true;
bool NumberedTextEditor::highlight_lines=true;
QColor NumberedTextEditor::line_hl_color=Qt::yellow;
QFont NumberedTextEditor::default_font=QFont(QString("DejaVu Sans Mono"), 10);
int NumberedTextEditor::tab_width=0;

NumberedTextEditor::NumberedTextEditor(QWidget * parent, bool allow_ext_files) : QPlainTextEdit(parent)
{
	this->allow_ext_files = allow_ext_files;
	line_number_wgt=new LineNumbersWidget(this);
	top_widget = nullptr;
	load_file_btn = link_file_btn = clear_btn	 = nullptr;

	if(allow_ext_files)
	{
		QPalette pal;
		QHBoxLayout *hbox = new QHBoxLayout;
		QFont font = this->font();

		font.setPointSizeF(font.pointSizeF() * 0.95f);

		top_widget = new QWidget(this);
		top_widget->setAutoFillBackground(true);

		pal.setColor(QPalette::Window, LineNumbersWidget::getBackgroundColor());
		top_widget->setPalette(pal);
		top_widget->setVisible(allow_ext_files);
		top_widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

		hbox->setContentsMargins(2,2,2,2);
		hbox->addSpacerItem(new QSpacerItem(10,10, QSizePolicy::Expanding));

		load_file_btn = new QToolButton(top_widget);
		load_file_btn->setIcon(QPixmap(PgModelerUiNS::getIconPath("abrir")));
		load_file_btn->setIconSize(QSize(16,16));
		load_file_btn->setAutoRaise(true);
		load_file_btn->setText(trUtf8("Load file"));
		load_file_btn->setToolTip(trUtf8("Load the object's source code from an external file"));
		load_file_btn->setFont(font);
		load_file_btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
		hbox->addWidget(load_file_btn);
		connect(load_file_btn, SIGNAL(clicked(bool)), this, SLOT(loadFile()));

		link_file_btn = new QToolButton(top_widget);
		link_file_btn->setIcon(QPixmap(PgModelerUiNS::getIconPath("fileref")));
		link_file_btn->setIconSize(QSize(16,16));
		link_file_btn->setAutoRaise(true);
		link_file_btn->setText(trUtf8("Link file"));
		link_file_btn->setToolTip(trUtf8("Reference an external file as object's source code.\nThe file is read everytime the object's definition is generated."));
		link_file_btn->setFont(font);
		link_file_btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
		hbox->addWidget(link_file_btn);

		connect(link_file_btn, &QToolButton::clicked, [=](){
				loadFile(true);
		});

		clear_btn = new QToolButton(top_widget);
		clear_btn->setIcon(QPixmap(PgModelerUiNS::getIconPath("limpartexto")));
		clear_btn->setIconSize(QSize(16,16));
		clear_btn->setAutoRaise(true);
		clear_btn->setText(trUtf8("Clear"));
		clear_btn->setFont(font);
		clear_btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
		clear_btn->setDisabled(true);

		connect(clear_btn, &QToolButton::clicked, [=](){
			this->clear();
			this->setReadOnly(false);
		});

		hbox->addWidget(clear_btn);
		top_widget->setLayout(hbox);

		connect(this, &NumberedTextEditor::textChanged, [=](){
			clear_btn->setEnabled(!this->toPlainText().isEmpty());
		});
	}

	setWordWrapMode(QTextOption::NoWrap);

	connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(highlightCurrentLine()));
	connect(this, SIGNAL(updateRequest(QRect,int)), this, SLOT(updateLineNumbers(void)));
	connect(this, SIGNAL(blockCountChanged(int)), this, SLOT(updateLineNumbersSize()));

	setCustomContextMenuEnabled(true);
}

void NumberedTextEditor::setCustomContextMenuEnabled(bool enabled)
{
	if(!enabled)
	{
		setContextMenuPolicy(Qt::NoContextMenu);
		disconnect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu()));
	}
	else
	{
		setContextMenuPolicy(Qt::CustomContextMenu);
		connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu()), Qt::UniqueConnection);
	}
}

void NumberedTextEditor::setDefaultFont(const QFont &font)
{
	default_font=font;
}

void NumberedTextEditor::setLineNumbersVisible(bool value)
{
	line_nums_visible=value;
}

void NumberedTextEditor::setHighlightLines(bool value)
{
	highlight_lines=value;
}

void NumberedTextEditor::setLineHighlightColor(const QColor &color)
{
	line_hl_color=color;
}

void NumberedTextEditor::setTabWidth(int value)
{
	if(value < 0)
		tab_width=0;
	else
		tab_width=value;
}

int NumberedTextEditor::getTabWidth(void)
{
	if(tab_width == 0)
		return(80);
	else
	{
		QFontMetrics fm(default_font);
		return(tab_width * fm.width(' '));
	}
}

void NumberedTextEditor::showContextMenu(void)
{
	QMenu *ctx_menu;
	QAction *act=nullptr;

	ctx_menu=createStandardContextMenu();

	if(!isReadOnly())
	{
		ctx_menu->addSeparator();

		act=ctx_menu->addAction(trUtf8("Upper case"), this, SLOT(changeSelectionToUpper()), QKeySequence(QString("Ctrl+U")));
		act->setEnabled(textCursor().hasSelection());

		act=ctx_menu->addAction(trUtf8("Lower case"), this, SLOT(changeSelectionToLower()), QKeySequence(QString("Ctrl+Shift+U")));
		act->setEnabled(textCursor().hasSelection());

		ctx_menu->addSeparator();

		act=ctx_menu->addAction(trUtf8("Ident right"), this, SLOT(identSelectionRight()), QKeySequence(QString("Tab")));
		act->setEnabled(textCursor().hasSelection());

		act=ctx_menu->addAction(trUtf8("Ident left"), this, SLOT(identSelectionLeft()), QKeySequence(QString("Shift+Tab")));
		act->setEnabled(textCursor().hasSelection());
	}

	ctx_menu->exec(QCursor::pos());
	delete(ctx_menu);
}

void NumberedTextEditor::changeSelectionToLower(void)
{
	changeSelectionCase(true);
}

void NumberedTextEditor::changeSelectionToUpper(void)
{
	changeSelectionCase(false);
}

void NumberedTextEditor::changeSelectionCase(bool lower)
{
	QTextCursor cursor=textCursor();

	if(cursor.hasSelection())
	{
		int start=cursor.selectionStart(),
				end=cursor.selectionEnd();

		if(!lower)
			cursor.insertText(cursor.selectedText().toUpper());
		else
			cursor.insertText(cursor.selectedText().toLower());

		cursor.setPosition(start);
		cursor.setPosition(end, QTextCursor::KeepAnchor);
		setTextCursor(cursor);
	}
}

void NumberedTextEditor::identSelectionRight(void)
{
	identSelection(true);
}

void NumberedTextEditor::identSelectionLeft(void)
{
	identSelection(false);
}

void NumberedTextEditor::identSelection(bool ident_right)
{
	QTextCursor cursor=textCursor();

	if(cursor.hasSelection())
	{
		QStringList lines;
		int start=-1,	end=-1,
				factor=(ident_right ? 1 : -1),	count=0;

		/* Forcing the selection of the very beggining of the first line and
		as well the end of the last line to avoid moving chars and break words wrongly */
		start=toPlainText().lastIndexOf(QChar('\n'), cursor.selectionStart());
		end=toPlainText().indexOf(QChar('\n'), cursor.selectionEnd());

		cursor.setPosition(start, QTextCursor::MoveAnchor);
		cursor.setPosition(end, QTextCursor::KeepAnchor);
		lines=cursor.selectedText().split(QChar(QChar::ParagraphSeparator));

		for(int i=0; i < lines.size(); i++)
		{
			if(!lines[i].isEmpty())
			{
				if(ident_right)
				{
					lines[i].prepend(QChar('\t'));
					count++;
				}
				else if(lines[i].at(0)==QChar('\t'))
				{
					lines[i].remove(0,1);
					count++;
				}
			}
		}

		if(count > 0)
		{
			cursor.insertText(lines.join(QChar('\n')));

			//Preserving the selection in the text to permit user perform several identations
			cursor.setPosition(start);
			cursor.setPosition(end + (count * factor), QTextCursor::KeepAnchor);
			setTextCursor(cursor);
		}
	}
}

void NumberedTextEditor::loadFile(bool only_ref)
{
	QFileDialog sql_file_dlg;

	sql_file_dlg.setDefaultSuffix(QString("sql"));
	sql_file_dlg.setFileMode(QFileDialog::AnyFile);
	sql_file_dlg.setNameFilter(trUtf8("SQL file (*.sql);;All files (*.*)"));
	sql_file_dlg.setModal(true);

	if(only_ref)
		sql_file_dlg.setWindowTitle(trUtf8("Link file"));
	else
		sql_file_dlg.setWindowTitle(trUtf8("Load file"));

	sql_file_dlg.setAcceptMode(QFileDialog::AcceptOpen);
	sql_file_dlg.exec();

	if(sql_file_dlg.result()==QDialog::Accepted)
	{
		if(only_ref)
		{
			/* When referencing an external file we register its path relatively to the application's working dir,
			 also we need to store the last modification date to avoid excessive loading when generating the object's source */
			QDir dir(qApp->applicationDirPath());
			this->setPlainText(GlobalAttributes::FILE_REF_TAG
												 .arg(dir.relativeFilePath(sql_file_dlg.selectedFiles().at(0)))
												 .arg(QFileInfo(sql_file_dlg.selectedFiles().at(0)).lastModified().toMSecsSinceEpoch()));
		}
		else
		{
			QFile file;
			file.setFileName(sql_file_dlg.selectedFiles().at(0));

			if(!file.open(QFile::ReadOnly))
				throw Exception(Exception::getErrorMessage(ERR_FILE_DIR_NOT_ACCESSED)
												.arg(sql_file_dlg.selectedFiles().at(0))
												,ERR_FILE_DIR_NOT_ACCESSED ,__PRETTY_FUNCTION__,__FILE__,__LINE__);

			this->clear();
			this->setPlainText(file.readAll());
			file.close();
		}

		this->setReadOnly(only_ref);
		clear_btn->setEnabled(!this->toPlainText().isEmpty());
	}
}

void NumberedTextEditor::setFocus(void)
{
	QPlainTextEdit::setFocus();
	this->highlightCurrentLine();
}

void NumberedTextEditor::updateLineNumbers(void)
{
	line_number_wgt->setVisible(line_nums_visible);
	if(!line_nums_visible) return;

	setFont(default_font);
	line_number_wgt->setFont(default_font);

	QTextBlock block = firstVisibleBlock();
	int block_number = block.blockNumber(),
			//Calculates the first block postion (in widget coordinates)
			top = static_cast<int>(blockBoundingGeometry(block).translated(contentOffset()).top()),
			bottom = top +  static_cast<int>(blockBoundingRect(block).height()),
			dy = top;
	unsigned first_line=0, line_count=0;

	// Calculates the visible lines by iterating over the visible/valid text blocks.
	while(block.isValid())
	{
		if(block.isVisible())
		{
			line_count++;
			if(first_line==0)
				first_line=static_cast<unsigned>(block_number + 1);
		}

		block = block.next();
		top = bottom;
		bottom = top + static_cast<int>(blockBoundingRect(block).height());
		++block_number;

		/* Check if the line count converted to widget coordinates is higher than the widget height.
	   This is done to avoid draw line numbers that are beyond the widget's height */
		if((static_cast<int>(line_count) * fontMetrics().height()) > this->height())
			break;
	}

	line_number_wgt->drawLineNumbers(first_line, line_count, dy);

	if(this->tabStopWidth()!=NumberedTextEditor::getTabWidth())
		this->setTabStopWidth(NumberedTextEditor::getTabWidth());
}

void NumberedTextEditor::updateLineNumbersSize(void)
{
	int py = (allow_ext_files && top_widget ? top_widget->height() : 0);

	if(line_nums_visible)
	{
		QRect rect=contentsRect();

		setViewportMargins(getLineNumbersWidth(), py, 0, 0);
		line_number_wgt->setGeometry(QRect(rect.left(), rect.top() + py, getLineNumbersWidth(), rect.height() - py));

		if(top_widget)
			top_widget->setGeometry(rect.left(), rect.top(),
															rect.width() - (this->verticalScrollBar()->isVisible() ? this->verticalScrollBar()->width() : 0),
															top_widget->height());
	}
	else
		setViewportMargins(0, py, 0, 0);
}

int NumberedTextEditor::getLineNumbersWidth(void)
{
	int digits=1, max=qMax(1, blockCount());

	while(max >= 10)
	{
		max /= 10;
		++digits;
	}

	return(15 + fontMetrics().width(QChar('9')) * digits);
}

void NumberedTextEditor::resizeEvent(QResizeEvent *event)
{
	QPlainTextEdit::resizeEvent(event);
	updateLineNumbersSize();
}

void NumberedTextEditor::keyPressEvent(QKeyEvent *event)
{
	if(!isReadOnly() && textCursor().hasSelection())
	{
		if(event->key()==Qt::Key_U && event->modifiers()!=Qt::NoModifier)
		{
			if(event->modifiers()==Qt::ControlModifier)
				changeSelectionToUpper();
			else if(event->modifiers()==(Qt::ControlModifier | Qt::ShiftModifier))
				changeSelectionToLower();
		}
		else if(event->key()==Qt::Key_Tab || event->key()==Qt::Key_Backtab)
		{
			if(event->key()==Qt::Key_Tab)
				identSelectionRight();
			else if(event->key()==Qt::Key_Backtab)
				identSelectionLeft();
		}
		else
			QPlainTextEdit::keyPressEvent(event);
	}
	else
		QPlainTextEdit::keyPressEvent(event);
}

void NumberedTextEditor::highlightCurrentLine(void)
{
	QList<QTextEdit::ExtraSelection> extraSelections;

	if(highlight_lines && !isReadOnly())
	{
		QTextEdit::ExtraSelection selection;
		selection.format.setBackground(line_hl_color);
		selection.format.setProperty(QTextFormat::FullWidthSelection, true);
		selection.cursor = textCursor();
		selection.cursor.clearSelection();
		extraSelections.append(selection);
	}

	setExtraSelections(extraSelections);
}
