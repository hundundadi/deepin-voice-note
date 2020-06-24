/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
*
* Author:     liuyanga <liuyanga@uniontech.com>
*
* Maintainer: liuyanga <liuyanga@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "textnoteitem.h"
#include "textnoteedit.h"
#include "common/utils.h"

#include <QVBoxLayout>
#include <QAbstractTextDocumentLayout>
#include <QClipboard>
#include <QDebug>

#include <DStyle>
#include <DApplicationHelper>

TextNoteItem::TextNoteItem(VNoteBlock *noteBlock, QWidget *parent)
    : DetailItemWidget(parent)
    , m_noteBlock(noteBlock)
{
    initUi();
    initConnection();
    Utils::blockToDocument(m_noteBlock,m_textEdit->document());
    onChangeTheme();
}

void TextNoteItem::initUi()
{
    m_textEdit = new TextNoteEdit(this);
    m_textEdit->setContextMenuPolicy(Qt::NoContextMenu);
    m_textEdit->document()->setDocumentMargin(0);
    DStyle::setFocusRectVisible(m_textEdit, false);
    DPalette pb = DApplicationHelper::instance()->palette(m_textEdit);
    pb.setBrush(DPalette::Button, QColor(0, 0, 0, 0));
    m_textEdit->setPalette(pb);
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(0, 5, 0, 5);
    layout->addWidget(m_textEdit);
    this->setLayout(layout);
}

void TextNoteItem::initConnection()
{
    QTextDocument *document = m_textEdit->document();
    QAbstractTextDocumentLayout *documentLayout = document->documentLayout();
    connect(documentLayout, &QAbstractTextDocumentLayout::documentSizeChanged, this, [ = ] {
        m_textEdit->setFixedHeight(static_cast<int>(document->size().height()));
        this->setFixedHeight(m_textEdit->height() + 10);
        onTextCursorChange();
    });
    connect(m_textEdit, SIGNAL(textChanged()), this, SLOT(onTextChange()));
    connect(m_textEdit, SIGNAL(sigFocusIn()), this, SIGNAL(sigFocusIn()));
    connect(m_textEdit, SIGNAL(sigFocusOut()), this, SIGNAL(sigFocusOut()));
    connect(m_textEdit, SIGNAL(selectionChanged()), this, SIGNAL(sigSelectionChanged()));
    connect(m_textEdit, SIGNAL(cursorPositionChanged()), this, SLOT(onTextCursorChange()));

    connect(DApplicationHelper::instance(), &DApplicationHelper::themeTypeChanged,
               this, &TextNoteItem::onChangeTheme);

}

void TextNoteItem::updateSearchKey(QString searchKey)
{
    if (m_noteBlock) {
        if(m_textDocumentUndo == true && m_serchKey == searchKey){
            return;
        }
        m_serchKey = searchKey;
        m_isSearching = true;
        if(m_textDocumentUndo == false && m_searchCount){
           Utils::setDefaultColor(m_textEdit->document(), m_textEdit->palette().text().color());
        }
        DPalette pb;
        m_searchCount = Utils::highTextEdit(m_textEdit->document(), m_serchKey, pb.color(DPalette::Highlight),m_textDocumentUndo);
        m_textDocumentUndo = m_searchCount == 0 ? false : true;
        m_isSearching = false;
    }
}

void TextNoteItem::setTextCursor(const QTextCursor &cursor)
{
    m_textEdit->setTextCursor(cursor);
}

QTextCursor TextNoteItem::getTextCursor()
{
    return  m_textEdit->textCursor();
}

VNoteBlock *TextNoteItem::getNoteBlock()
{
    return  m_noteBlock;
}

bool TextNoteItem::textIsEmpty()
{
    return  m_textEdit->document()->isEmpty();
}

QRect TextNoteItem::getCursorRect()
{
    return m_textEdit->cursorRect(m_textEdit->textCursor());
}

void TextNoteItem::selectText(const QPoint &globalPos, QTextCursor::MoveOperation op)
{
    return m_textEdit->selectText(globalPos, op);
}

void TextNoteItem::selectAllText()
{
    if(m_selectAll == false){
        if (!textIsEmpty()) {
            m_textEdit->selectAll();
        }
        m_selectAll = true;
    }

}

void TextNoteItem::selectText(QTextCursor::MoveOperation op)
{
    return m_textEdit->moveCursor(op, QTextCursor::KeepAnchor);
}

void TextNoteItem::removeSelectText()
{
    return m_textEdit->removeSelectText();
}

void TextNoteItem::clearSelection()
{
    m_selectAll = false;
    return m_textEdit->clearSelection();
}

bool TextNoteItem::hasSelection()
{
    return m_selectAll || m_textEdit->hasSelection();
}

QTextDocumentFragment TextNoteItem::getSelectFragment()
{
    return  getTextCursor().selection();
}

void TextNoteItem::setFocus()
{
    return m_textEdit->setFocus();
}

bool TextNoteItem::hasFocus()
{
    return  m_textEdit->hasFocus();
}

bool TextNoteItem::isSelectAll()
{
    return m_selectAll;
}

QTextDocument* TextNoteItem::getTextDocument()
{
    return m_textEdit->document();
}

void TextNoteItem::pasteText()
{
    QClipboard *board = QApplication::clipboard();
    if (board) {
        QString clipBoardText = board->text();
        m_textEdit->insertPlainText(clipBoardText);
    }
}

void TextNoteItem::onTextChange()
{
    if(m_isSearching == false){
        if(m_searchCount){
            m_textDocumentUndo = false;
        }
        emit sigTextChanged();
    }
}

void TextNoteItem::onTextCursorChange()
{
    if (this->hasFocus()){
        int height = m_textEdit->cursorRect().bottom() + 5;
        if(m_lastCursorHeight != height){
           m_lastCursorHeight = height;
           emit sigCursorHeightChange(this, height);
        }
    }
}

void TextNoteItem::setLastCursorHeight(int height)
{
    m_lastCursorHeight = height;
}

void TextNoteItem::onChangeTheme()
{
    DPalette appDp = DApplicationHelper::instance()->applicationPalette();
    DPalette dp = DApplicationHelper::instance()->palette(m_textEdit);
    dp.setBrush(DPalette::Highlight, appDp.color(DPalette::Normal,DPalette::Highlight));
    dp.setBrush(DPalette::HighlightedText, appDp.color(DPalette::Normal,DPalette::HighlightedText));
    m_textEdit->setPalette(dp);

    m_searchCount = 1;
    m_textDocumentUndo = false;
    updateSearchKey(m_serchKey);
}