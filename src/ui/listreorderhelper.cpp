#include "listreorderhelper.h"

#include <QAbstractItemModel>
#include <QApplication>
#include <QDrag>
#include <QDropEvent>
#include <QMimeData>
#include <QMouseEvent>
#include <QFrame>
#include <QItemSelectionModel>
#include <QListView>
#include <algorithm>


ListReorderHelper::ListReorderHelper(QListView *view, MoveRows moveRows)
    : QObject(view), m_view(view), m_moveRows(std::move(moveRows))
{
    m_view->setDragEnabled(false);
    m_view->setAcceptDrops(true);
    m_view->viewport()->setAcceptDrops(true);
    m_view->viewport()->installEventFilter(this);

    m_indicator = new QFrame(m_view->viewport());
    m_indicator->setFixedHeight(2);
    m_indicator->setAutoFillBackground(true);
    m_indicator->setAttribute(Qt::WA_TransparentForMouseEvents);
    QPalette pal = m_indicator->palette();
    pal.setColor(QPalette::Window, m_view->palette().color(QPalette::Highlight));
    m_indicator->setPalette(pal);
    m_indicator->hide();
}


int ListReorderHelper::targetRow(const QPoint &pos) const
{
    const int count = m_view->model()->rowCount();
    const QModelIndex index = m_view->indexAt(pos);
    if (!index.isValid())
        return pos.y() < 0 ? 0 : count;
    const QRect rect = m_view->visualRect(index);
    return index.row() + (pos.y() > rect.center().y() ? 1 : 0);
}


bool ListReorderHelper::drop(const QPoint &pos)
{
    QList<int> rows;
    for (const QModelIndex &index : m_view->selectionModel()->selectedRows())
        rows.append(index.row());
    if (rows.isEmpty())
        return false;
    std::sort(rows.begin(), rows.end());

    const int target = targetRow(pos);
    // dropping a block directly in front of or behind itself changes nothing
    bool contiguous = rows.last() - rows.first() + 1 == rows.size();
    if (contiguous && target >= rows.first() && target <= rows.last() + 1)
        return false;

    m_moveRows(rows, target);
    return true;
}


static const char *kMimeType = "application/x-smartcardmanager-listrows";


void ListReorderHelper::startDrag()
{
    const QModelIndexList selected = m_view->selectionModel()->selectedRows();
    if (selected.isEmpty())
        return;

    auto *mime = new QMimeData;
    mime->setData(kMimeType, QByteArray("rows"));
    auto *drag = new QDrag(m_view);
    drag->setMimeData(mime);
    drag->setPixmap(m_view->viewport()->grab(m_view->visualRect(m_view->currentIndex().isValid() ? m_view->currentIndex() : selected.first())));
    drag->exec(Qt::CopyAction, Qt::CopyAction);
    hideIndicator();
}


bool ListReorderHelper::eventFilter(QObject *obj, QEvent *event)
{
    if (obj != m_view->viewport())
        return false;

    switch (event->type()) {
    case QEvent::MouseButtonPress: {
        auto *me = static_cast<QMouseEvent *>(event);
        m_pressed = me->button() == Qt::LeftButton && m_view->indexAt(me->position().toPoint()).isValid();
        m_pressPos = me->position().toPoint();
        return false;
    }
    case QEvent::MouseButtonRelease:
        m_pressed = false;
        return false;
    case QEvent::MouseMove: {
        auto *me = static_cast<QMouseEvent *>(event);
        if (!m_pressed || !(me->buttons() & Qt::LeftButton))
            return false;
        if ((me->position().toPoint() - m_pressPos).manhattanLength() < QApplication::startDragDistance())
            return false;
        m_pressed = false;
        startDrag();
        return true;
    }
    case QEvent::DragEnter:
    case QEvent::DragMove: {
        auto *drag = static_cast<QDropEvent *>(event);
        if (!drag->mimeData()->hasFormat(kMimeType))
            return false;
        drag->setDropAction(Qt::CopyAction);
        drag->accept();
        showIndicator(targetRow(drag->position().toPoint()));
        return true;
    }
    case QEvent::DragLeave:
        hideIndicator();
        return false;
    case QEvent::Drop: {
        auto *drop = static_cast<QDropEvent *>(event);
        if (!drop->mimeData()->hasFormat(kMimeType))
            return false;
        hideIndicator();
        ListReorderHelper::drop(drop->position().toPoint());
        drop->setDropAction(Qt::CopyAction);
        drop->accept();
        return true;
    }
    default:
        return false;
    }
}


void ListReorderHelper::showIndicator(int target)
{
    const int count = m_view->model()->rowCount();
    int y = 0;
    if (count > 0) {
        y = target < count ? m_view->visualRect(m_view->model()->index(target, 0)).top()
                           : m_view->visualRect(m_view->model()->index(count - 1, 0)).bottom() + 1;
    }
    m_indicator->setGeometry(0, qMax(0, y - 1), m_view->viewport()->width(), 2);
    m_indicator->raise();
    m_indicator->show();
}


void ListReorderHelper::hideIndicator()
{
    m_indicator->hide();
}
