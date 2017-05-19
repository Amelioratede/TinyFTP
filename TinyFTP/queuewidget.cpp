#include "queuewidget.h"
#include "ftpclient.h"
#include "tinyftp.h"
#include "remotedirwidget.h"
#include "localdirwidget.h"

qint64 Task::id = 0;
QMutex Task::mutex;

QueueWidget::QueueWidget(const QString &title, QWidget * parent/* = 0*/)
	: QDockWidget(title, parent)
{
	tabWidget = new QTabWidget(parent);
	queueTreeWidget = new QTreeWidget(parent);
	queueDelegate = new QueueDelegate(queueTreeWidget);
	taskThread = new TaskThread(parent);
	queueTreeWidget->setItemDelegateForColumn(7, queueDelegate);
	queueTreeWidget->header()->setStretchLastSection(true);
	queueTreeWidget->setAlternatingRowColors(true);
	queueTreeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
	queueTreeWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
	queueTreeWidget->setSortingEnabled(true);
	queueTreeWidget->setRootIsDecorated(false);
    queueTreeWidget->setHeaderLabels(QStringList() << tr("TaskID") << tr("����") << tr("��С") << tr("������") 
        << tr("Դ·��") << tr("Ŀ��·��") << tr("״̬") << tr("����") << tr("�ٶ�") << tr("ʣ��ʱ��"));

	setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
	setFeatures(QDockWidget::DockWidgetVerticalTitleBar | QDockWidget::DockWidgetClosable
		| QDockWidget::DockWidgetFloatable);
	tabWidget->addTab(queueTreeWidget, tr("����"));
	tabWidget->setTabPosition(QTabWidget::South);
	setWidget(tabWidget);
}

QueueWidget::~QueueWidget()
{

}

void QueueWidget::addTask(Task *task)
{
	QTreeWidgetItem *item = new QTreeWidgetItem(queueTreeWidget);
	item->setText(0, QString::number(task->taskId()));
	item->setData(1, Qt::DecorationRole, task->icon);
	item->setText(1, task->fileName);
	item->setText(2, fileSizeUnitTranslator(task->fileSize));
	item->setText(3, task->urlAddress.host());
	if (task->type == taskType_Download) {
		item->setText(4, task->downloadRemoteDirPathUrl);
		item->setText(5, task->downloadLocalDirPath);
	} else if (task->type == taskType_Upload) {
		item->setText(4, task->uploadLocalDirPath);
		item->setText(5, task->uploadRemoteDirPathUrl);
	}
	item->setText(6, tr("�ȴ�"));
	queueTreeWidget->openPersistentEditor(item, 7);
	taskThread->addTask(task);
	if (!taskThread->isRunning()) {
		taskThread->start();
	}
}

QueueDelegate::QueueDelegate(QObject *parent /*= 0*/)
	: QStyledItemDelegate(parent)
{

}

QWidget * QueueDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	if (index.column() == 7) {
		return new QProgressBar(parent);
	}
	return QStyledItemDelegate::createEditor(parent, option, index);
}

TaskThread::TaskThread(QObject *parent /*= 0*/)
	: QThread(parent)
{
	parentTinyFTP = static_cast<TinyFTP*>(parent);
	isStop = true;
	for (int i = 0; i < TASK_THREAD_MAX_FTP_CLIENT; i++) {
		ftpClients.append(new FTPClient(parentTinyFTP));
	}
}

TaskThread::~TaskThread()
{
	isStop = true;
}

void TaskThread::addTask(Task *task)
{
	QMutexLocker locker(&tasksQueueMutex);
	tasksQueue.enqueue(task);
}

void TaskThread::run()
{
	isStop = false;
	while (!isStop) {
        {
            QMutexLocker lockerQueue(&tasksQueueMutex);
            if (!tasksQueue.isEmpty()) {
                FTPClient *f = idleFtpClient();
                if (f) {
                    Task *task = tasksQueue.dequeue();
					f->disconnect(SIGNAL(ftpMsg(const QString &)));
					f->disconnect(SIGNAL(refreshLocalDirWidget()));		// ˢ�±���Ŀ¼��
					f->disconnect(SIGNAL(refreshRemoteDirWidget()));	// ˢ��Զ��Ŀ¼��
                    connect(f, SIGNAL(ftpMsg(const QString &)), task->parent(), SLOT(writeLog(const QString &)));
					connect(f, SIGNAL(refreshLocalDirWidget()), parentTinyFTP->localCurrentWidget(), SLOT(refresh()));
					connect(f, SIGNAL(refreshRemoteDirWidget()), task->parent(), SLOT(refresh()));

                    //*******************************
                    // ����Task
                    f->sendMsg(tr(">>>>>>>>>>>��ʼ����[%1]<<<<<<<<<<<").arg(task->taskName()));
                    f->connectToHost(task->urlAddress.host(), task->urlAddress.port());
                    f->login(task->urlAddress.userName(), task->urlAddress.password());
                    /*sleep(3);*/
                    if (task->type == taskType_Download) {
                        f->sendMsg(tr("��ʼ���� %1 �� %2").arg(task->downloadRemoteDirPathUrl + tr("/") + task->fileName).arg(
                            task->downloadLocalDirPath + tr("/") + task->fileName));
                        f->download(task->downloadRemoteDirPathUrl, task->downloadLocalDirPath,
                            task->fileName, task->isDir);
                    } else if (task->type == taskType_Upload) {
                        f->sendMsg(tr("��ʼ�ϴ� %1 �� %2").arg(task->uploadRemoteDirPathUrl + tr("/") + task->fileName).arg(
                            task->uploadLocalDirPath + tr("/") + task->fileName));
                        f->upload(task->uploadRemoteDirPathUrl, task->uploadLocalDirPath + tr("/") + task->fileName);
                    }
                    delete task;
                } else {
                    sleep(1);
                }
            } 
        }

		sleep(1);
	}
	isStop = true;
}

void TaskThread::stop()
{
	isStop = true;
}

bool TaskThread::isRunning()
{
	return !isStop;
}

FTPClient * TaskThread::idleFtpClient()
{
	QMutexLocker locker(&ftpClientsMutex);
	foreach (FTPClient *f, ftpClients) {
		if (f->idle())
			return f;
	}
	return 0;
}

Task::Task(QWidget *parent/* = 0*/)
    : parentWidget(parent)
{
	{
		QMutexLocker locker(&mutex);
		objId = ++id;
	}
    parentWidget = static_cast<RemoteDirWidget*>(parent);
	objName = QObject::tr("Task%1").arg(objId);
}

QString Task::taskName() const
{
	return objName;
}

qint64 Task::taskId() const
{
	return objId;
}

QWidget * Task::parent()
{
    return parentWidget;
}
