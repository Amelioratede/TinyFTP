#include "remotedirtreeview.h"
#include "remotedirwidget.h"
#include "dirtreemodel.h"

RemoteDirTreeView::RemoteDirTreeView(QWidget *parent)
    : QTreeView(parent)
{
    parentRemoteDirWidget = static_cast<RemoteDirWidget*>(parent);
}

RemoteDirTreeView::~RemoteDirTreeView()
{

}

void RemoteDirTreeView::contextMenuEvent(QContextMenuEvent *event)
{
    RemoteDirWidget *p = parentRemoteDirWidget;
    //*******************************
    // Ĭ�� ʹ�� ���в˵���
    QList<QAction*> actions = p->contextMenu->actions();
    foreach (QAction* action, actions)
        action->setEnabled(false);

    //*******************************
    // ��ǰδ�����ϻ���������δִ���꣬���������ѡ��
    if (/*!p->isConnected() || */p->ftpClient->hasPendingCommands()) {
        goto menuexec;
    }

    //*******************************
    // ���������Ĳ˵�
    if (p->currentDirPathUrl() == tr("/")) {
        p->dotdotAction->setEnabled(false);
    } else {
        DirTreeModel *r = static_cast<DirTreeModel*>(p->remoteDirTreeView->model());
        Node *dotdotNode = static_cast<Node*>(r->index(0, 0).internalPointer());
        p->dotdotAction->setEnabled(dotdotNode->fileName == tr(".."));
    }
    p->refreshAction->setEnabled(true);
    p->newDirAction->setEnabled(true);
menuexec:
    p->contextMenu->exec(QCursor::pos());
}

int RemoteDirTreeView::rowCount()
{
    DirTreeModel *d = static_cast<DirTreeModel *>(model());
    return d ? d->rowCount() : 0;
}

int RemoteDirTreeView::columnCount()
{
    DirTreeModel *d = static_cast<DirTreeModel *>(model());
    return d ? d->columnCount() : 0;
}

void RemoteDirTreeView::setRootPath(const QString path)
{
    DirTreeModel *d = static_cast<DirTreeModel *>(model());
    if (d)
        d->setRootPath(path);
}

void RemoteDirTreeView::sort(int column, Qt::SortOrder order /* = Qt::AscendingOrder */)
{
    DirTreeModel *d = static_cast<DirTreeModel *>(model());
    if (d)
        d->sort(column, order);
}

QString RemoteDirTreeView::currentDirPath() const
{
    DirTreeModel *d = static_cast<DirTreeModel *>(model());
    return d ? d->currentDirPath() : QString();
}

Node* RemoteDirTreeView::item(int row, int column) const
{
    DirTreeModel *d = static_cast<DirTreeModel *>(model());
    if (!d) 
        return 0;
    return static_cast<Node*>(d->index(row, column).internalPointer());
}

void RemoteDirTreeView::resizeColumnsToContents()
{
    for (int i = 0; i < model()->columnCount(); i++) {
        resizeColumnToContents(i);
    }
}
