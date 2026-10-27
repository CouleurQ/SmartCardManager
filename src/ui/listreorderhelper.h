#ifndef LISTREORDERHELPER_H
#define LISTREORDERHELPER_H

#include <QObject>
#include <QList>
#include <QPoint>
#include <functional>

class QListView;
class QFrame;


// Lets the user reorder the items of a list view by dragging them. Drag and drop are handled
// completely here (the view's own drag & drop is not used) and delegated to a callback, so it works
// for any model. The drag is a copy action so the view never removes the dragged rows by itself.
class ListReorderHelper : public QObject
{
    Q_OBJECT

public:
    // moveRows(rows, target): move the (sorted) rows so that they end up in front of row 'target'
    // (target == rowCount() appends)
    using MoveRows = std::function<void(const QList<int> &rows, int target)>;

    ListReorderHelper(QListView *view, MoveRows moveRows);

    // row (0..rowCount) in front of which an item dropped at viewport position 'pos' is inserted
    int targetRow(const QPoint &pos) const;
    // moves the selected rows to the drop position; returns false if nothing was moved
    bool drop(const QPoint &pos);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void startDrag();
    void showIndicator(int target);
    void hideIndicator();

    QListView *m_view;
    MoveRows m_moveRows;
    QFrame *m_indicator;
    QPoint m_pressPos;
    bool m_pressed = false;
};

#endif
