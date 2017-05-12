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
    if (!p->isConnected() || p->ftpClient->hasPendingCommands()) {
        goto menuexec;
    }

    //*******************************
    // ���������Ĳ˵�
    if (p->currentDirPathUrl() == QDir::separator()) {
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
