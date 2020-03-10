#include "middleviewdelegate.h"
#include "common/vnoteitem.h"
#include "common/vnoteforlder.h"
#include "common/utils.h"
#include "common/standarditemcommon.h"
#include "db/vnoteitemoper.h"

#include "db/vnotefolderoper.h"

#include <QLineEdit>
#include <QPainter>

#include <DApplicationHelper>

static VNoteFolderOper FolderOper;

struct VNoteTextPHelper {
    VNoteTextPHelper(QPainter *painter, QFontMetrics fontMetrics, QRect nameRect)
        : m_fontMetrics(fontMetrics)
        , m_nameRect(nameRect)
        , m_painter(painter)
    {
        if (nullptr != m_painter) {
            m_pens[OldPen] = m_painter->pen();

            DPalette pl;
            QColor folderNamePenColor = pl.color(DPalette::Highlight);
            m_pens[FolderPen] = folderNamePenColor;
        }
    }

    struct Text {
        QString text;
        QRect   rect;
        bool    isKeyword {false};
    };

    enum {
        OldPen,
        FolderPen,
        PenCount
    };

    void spiltByKeyword(const QString &text, const QRegExp &keyword);
    void paintText(bool isSelected = false);

    QVector<Text> m_textsVector;
    QFontMetrics m_fontMetrics;
    QPen         m_pens[PenCount];
    QRect        m_nameRect;
    QPainter    *m_painter {nullptr};
};

void VNoteTextPHelper::spiltByKeyword(const QString &text, const QRegExp &keyword)
{
    //Check if text exceed the name rect, elide the
    //text first
    QString elideText = m_fontMetrics.elidedText(text, Qt::ElideRight, m_nameRect.width());

    int keyLen = keyword.pattern().length();
    int textLen = text.length();
    int startPos = 0;
    int pos = 0;

    m_textsVector.clear();

    if (!keyword.isEmpty()) {
        while ((pos = elideText.indexOf(keyword, startPos)) != -1) {
            Text tb;

            if (startPos != pos) {
                int extraLen = pos - startPos;
                tb.text = elideText.mid(startPos, extraLen);
                startPos += extraLen;

                tb.rect = QRect(0, 0
                                , qMax<int>(
                                    m_fontMetrics.width(tb.text)
                                    , m_fontMetrics.boundingRect(tb.text).width()
                                )
                                , m_fontMetrics.height()
                               );

                m_textsVector.push_back(tb);

                tb.text = elideText.mid(pos, keyLen);
                tb.rect = QRect(0, 0
                                , qMax<int>(
                                    m_fontMetrics.width(tb.text)
                                    , m_fontMetrics.boundingRect(tb.text).width()
                                )
                                , m_fontMetrics.height()
                               );
                tb.isKeyword = true;
                m_textsVector.push_back(tb);
            } else {
                tb.text = elideText.mid(pos, keyLen);
                tb.rect = QRect(0, 0
                                , qMax<int>(
                                    m_fontMetrics.width(tb.text)
                                    , m_fontMetrics.boundingRect(tb.text).width()
                                )
                                , m_fontMetrics.height()
                               );
                tb.isKeyword = true;
                m_textsVector.push_back(tb);
            }

            startPos += keyLen;
        }
    }

    if (startPos < elideText.length()) {
        Text tb;

        tb.text = elideText.mid(startPos, (textLen - startPos));
        tb.rect = QRect(0, 0
                        , qMax<int>(
                            m_fontMetrics.width(tb.text)
                            , m_fontMetrics.boundingRect(tb.text).width()
                        )
                        , m_fontMetrics.height()
                       );

        m_textsVector.push_back(tb);
    }
}

void VNoteTextPHelper::paintText(bool isSelected)
{
    int currentX = m_nameRect.x();
    int currentY = m_nameRect.y();

    for (auto it : m_textsVector) {
        int w = it.rect.width();
        int h = m_nameRect.height();

        it.rect.setX(currentX);
        it.rect.setY(currentY);
        it.rect.setSize(QSize(w, h));
        if (it.isKeyword) {
            //If the item is selected,don't need highlight keyword
            m_painter->setPen((isSelected ? m_pens[OldPen] : m_pens[FolderPen]));
        } else {
            m_painter->setPen(m_pens[OldPen]);
        }

        m_painter->drawText(it.rect, it.text);

        currentX += it.rect.width();
    }

    //Restore default pen
    m_painter->setPen(m_pens[OldPen]);
}
MiddleViewDelegate::MiddleViewDelegate(QAbstractItemView *parent)
    : DStyledItemDelegate(parent)
    , m_parentView(parent)
{
    m_parentPb = DApplicationHelper::instance()->palette(m_parentView);
    connect(DApplicationHelper::instance(), &DApplicationHelper::themeTypeChanged, this,
            &MiddleViewDelegate::handleChangeTheme);
}

void MiddleViewDelegate::setSearchKey(const QRegExp &key)
{
    m_searchKey = key;
}

void MiddleViewDelegate::handleChangeTheme()
{
    m_parentPb = DApplicationHelper::instance()->palette(m_parentView);
    m_parentView->update(m_parentView->currentIndex());
}

void MiddleViewDelegate::setEnableItem(bool enable)
{
    m_enableItem = enable;
}

QSize MiddleViewDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index);
    if (m_searchKey.isEmpty()) {
        return QSize(option.rect.width(), 74);
    } else {
        return QSize(option.rect.width(), 102);
    }
}

QWidget *MiddleViewDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                                        const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)
    QLineEdit *editBox = new QLineEdit(parent);
    editBox->setFixedSize(option.rect.width() - 60, option.rect.height() - 20);
    return editBox;
}

void MiddleViewDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QVariant var = index.data(Qt::UserRole + 1);
    VNoteItem *data = static_cast<VNoteItem*>(StandardItemCommon::getStandardItemData(index));
    if(data){
        QLineEdit *edit = static_cast<QLineEdit *>(editor);
        edit->setText(data->noteTitle);
    }
}

void MiddleViewDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                    const QModelIndex &index) const
{
    Q_UNUSED(model);
    QLineEdit *edit = static_cast<QLineEdit *>(editor);
    QString newTitle = edit->text();

    //Update note title
    VNoteItem *note = static_cast<VNoteItem*>(StandardItemCommon::getStandardItemData(index));
    if(note){
        //Truncate the title name if exceed 64 chars
        if (newTitle.length() > MAX_TITLE_LEN) {
            newTitle = newTitle.left(MAX_TITLE_LEN);
        }

        if (!newTitle.isEmpty() && (note->noteTitle != newTitle)) {
            VNoteItemOper noteOps(note);
            noteOps.modifyNoteTitle(newTitle);
        }
    }
}

void MiddleViewDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                                            const QModelIndex &index) const
{
    Q_UNUSED(index)
    QLineEdit *edit = static_cast<QLineEdit *>(editor);
    edit->move(option.rect.x() + 30, option.rect.y() + 10);
}

void MiddleViewDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
    if (m_searchKey.isEmpty()) {
        paintNormalItem(painter, option, index);
    } else {
        paintSearchItem(painter, option, index);
    }
}

void MiddleViewDelegate::paintItemBase(QPainter *painter, const QStyleOptionViewItem &option,
                                     const QRect &rect, bool &isSelect) const
{
    QPainterPath path;
    const int radius = 8;
    path.moveTo(rect.bottomRight() - QPoint(0, radius));
    path.lineTo(rect.topRight() + QPoint(0, radius));
    path.arcTo(QRect(QPoint(rect.topRight() - QPoint(radius * 2, 0)), QSize(radius * 2, radius * 2)), 0, 90);
    path.lineTo(rect.topLeft() + QPoint(radius, 0));
    path.arcTo(QRect(QPoint(rect.topLeft()), QSize(radius * 2, radius * 2)), 90, 90);
    path.lineTo(rect.bottomLeft() - QPoint(0, radius));
    path.arcTo(QRect(QPoint(rect.bottomLeft() - QPoint(0, radius * 2)), QSize(radius * 2, radius * 2)), 180, 90);
    path.lineTo(rect.bottomLeft() + QPoint(radius, 0));
    path.arcTo(QRect(QPoint(rect.bottomRight() - QPoint(radius * 2, radius * 2)), QSize(radius * 2, radius * 2)), 270, 90);
    if (option.state & QStyle::State_Selected) {
        QColor fillColor = option.palette.color(DPalette::Normal, DPalette::Highlight);
//        if (enable) {
//            fillColor = option.palette.color(DPalette::Normal, DPalette::Highlight);
//        } else {
//            fillColor = option.palette.color(DPalette::Disabled, DPalette::Highlight);
//        }
        painter->setBrush(QBrush(fillColor));
        painter->fillPath(path, painter->brush());
        painter->setPen(QPen(Qt::white));
        isSelect = true;
    } else {
        isSelect = false;
        if (m_enableItem == false) {
            painter->setBrush(QBrush(m_parentPb.color(DPalette::Disabled, DPalette::ItemBackground)));
            painter->fillPath(path, painter->brush());
            painter->setPen(QPen(m_parentPb.color(DPalette::Disabled, DPalette::WindowText)));
        } else {
            if (option.state & QStyle::State_MouseOver) {
                painter->setBrush(QBrush(m_parentPb.color(DPalette::Light)));
                painter->fillPath(path, painter->brush());
                painter->setPen(QPen(m_parentPb.color(DPalette::Normal, DPalette::WindowText)));
            } else {
                painter->setBrush(QBrush(m_parentPb.color(DPalette::Normal, DPalette::ItemBackground)));
                painter->fillPath(path, painter->brush());
                painter->setPen(QPen(m_parentPb.color(DPalette::Normal, DPalette::WindowText)));
            }
        }
    }
}
void MiddleViewDelegate::paintNormalItem(QPainter *painter, const QStyleOptionViewItem &option,
                                       const QModelIndex &index) const
{
    VNoteItem *data = static_cast<VNoteItem*>(StandardItemCommon::getStandardItemData(index));
    if(!data){
        return;
    }
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHints(QPainter::SmoothPixmapTransform);
    QRect paintRect(option.rect.x() + 10, option.rect.y() + 5,
                    option.rect.width() - 20, option.rect.height() - 10);
    bool isSelect = false;
    paintItemBase(painter, option, paintRect, isSelect);
    painter->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T6));
    QFontMetrics fontMetrics = painter->fontMetrics();
    int space = (paintRect.height() - fontMetrics.height() * 2) / 2 + paintRect.top();
    QRect nameRect(paintRect.left() + 20, space, paintRect.width() - 40, fontMetrics.height());
    space += fontMetrics.height();
    QRect timeRect(paintRect.left() + 20, space, paintRect.width() - 40, fontMetrics.height());
    QString elideText = fontMetrics.elidedText(data->noteTitle, Qt::ElideRight, nameRect.width());
    painter->drawText(nameRect, Qt::AlignLeft | Qt::AlignVCenter, elideText);
    if (!isSelect) {
        painter->setPen(QPen(m_parentPb.color(DPalette::Normal, DPalette::TextTips)));
    }
    painter->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T8));
    painter->drawText(timeRect, Qt::AlignLeft | Qt::AlignVCenter, Utils::convertDateTime(data->modifyTime));
    painter->restore();
}

void MiddleViewDelegate::paintSearchItem(QPainter *painter, const QStyleOptionViewItem &option,
                                       const QModelIndex &index) const
{
    VNoteItem *noteData = static_cast<VNoteItem*>(StandardItemCommon::getStandardItemData(index));
    if(!noteData){
        return;
    }
    QRect paintRect(option.rect.x() + 10, option.rect.y() + 5,
                    option.rect.width() - 20, option.rect.height() - 10);
    bool isSelect = false;
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHints(QPainter::SmoothPixmapTransform);
    painter->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T6));
    paintItemBase(painter, option, paintRect, isSelect);

    QRect itemRect = paintRect;
    itemRect.setHeight(itemRect.height() - 34);
    painter->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T6));
    QFontMetrics fontMetrics = painter->fontMetrics();
    int space = (itemRect.height() - fontMetrics.height() * 2) / 2 + itemRect.top();
    QRect nameRect(itemRect.left() + 20, space, itemRect.width() - 40, fontMetrics.height());
    space += fontMetrics.height();
    QRect timeRect(itemRect.left() + 20, space, itemRect.width() - 40, fontMetrics.height());
    VNoteTextPHelper vfnphelper(painter, fontMetrics, nameRect);
    vfnphelper.spiltByKeyword(noteData->noteTitle, m_searchKey);
    vfnphelper.paintText(isSelect);

    if (!isSelect) {
        painter->setPen(QPen(m_parentPb.color(DPalette::Normal, DPalette::TextTips)));
    }
    painter->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T8));
    painter->drawText(timeRect, Qt::AlignLeft | Qt::AlignVCenter, Utils::convertDateTime(noteData->modifyTime));
    VNoteFolder *folderData = FolderOper.getFolder(noteData->folderId);
    if (folderData) {
        QRect folderRect = itemRect;
        folderRect.setY(itemRect.bottom());
        QRect iconRect(folderRect.left() + 20, folderRect.top(), 24, 24);
        painter->drawImage(iconRect, folderData->UI.icon);
        QRect folderNameRect(iconRect.right() + 12, folderRect.top(),
                             paintRect.width() - iconRect.width() - 50, 24);
        QString elideText = fontMetrics.elidedText(folderData->name, Qt::ElideRight, folderNameRect.width());
        painter->drawText(folderNameRect, Qt::AlignLeft | Qt::AlignVCenter, elideText);
    }
    painter->restore();
}
