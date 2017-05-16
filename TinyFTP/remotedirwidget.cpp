#include "common.h"
#include "remotedirwidget.h"
#include "localdirwidget.h"
#include "tinyftp.h"
#include "dirtreemodel.h"
#include "remotedirtreeview.h"

RemoteDirWidget::RemoteDirWidget(QWidget *parent)
	: QWidget(parent)
{
	parentTinyFtp = reinterpret_cast<TinyFTP*>(parent);
	currentCommand = CMD_NONE;

	remoteDirTreeModel = new DirTreeModel(this);

	remoteDirTreeView = new RemoteDirTreeView(this);
    remoteDirTreeView->setModel(remoteDirTreeModel);
    remoteDirTreeView->header()->setStretchLastSection(true);   
    remoteDirTreeView->resizeColumnsToContents();
	remoteDirTreeView->setAlternatingRowColors(true);
	remoteDirTreeView->setSelectionMode(QAbstractItemView::ContiguousSelection);
	remoteDirTreeView->setSelectionBehavior(QAbstractItemView::SelectRows);
	remoteDirTreeView->setSortingEnabled(true);
	remoteDirTreeView->sortByColumn(0, Qt::AscendingOrder);
    remoteDirTreeView->setItemsExpandable(false);
    remoteDirTreeView->setRootIsDecorated(false);
    remoteDirTreeView->setExpandsOnDoubleClick(false);
    remoteDirTreeView->setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::DoubleClicked);

	remoteDirFileSystemModel = new QFileSystemModel(this);
	remoteDirFileSystemModel->setFilter(QDir::AllDirs | QDir::Drives | 
		QDir::NoDot | QDir::NoDotDot | QDir::NoDotAndDotDot);

	remoteDirComboTreeView = new QTreeView;
	remoteDirComboTreeView->setModel(remoteDirFileSystemModel);
	remoteDirComboTreeView->resizeColumnToContents(0);
	remoteDirComboTreeView->hideColumn(1);
	remoteDirComboTreeView->hideColumn(2);
	remoteDirComboTreeView->hideColumn(3);
	remoteDirComboTreeView->setHeaderHidden(true);
	remoteDirComboTreeView->setItemsExpandable(true);

	remoteDirComboBox = new QComboBox(this);
	remoteDirComboBox->setEnabled(false);

	connectButton = new QToolButton(this);
	connectButton->setText(tr("�Ͽ�"));
	connectButton->setEnabled(false);
	dotdotDirToolButton = new QToolButton(this);
	dotdotDirToolButton->setText(tr("�ϼ�Ŀ¼"));
	dotdotDirToolButton->setEnabled(false);
	refreshDirToolButton = new QToolButton(this);
	refreshDirToolButton->setText(tr("ˢ��"));
	refreshDirToolButton->setEnabled(false);

	logTextEdit = new QTextEdit(this);
	logTextEdit->setFixedHeight(100);
	logTextEdit->setReadOnly(true);
	remoteDirStatusBar = new QStatusBar(this);

    ftpClient = new QFtp(this);

	QHBoxLayout *topHBoxLayout = new QHBoxLayout;
	topHBoxLayout->addWidget(connectButton);
	topHBoxLayout->addWidget(dotdotDirToolButton);
	topHBoxLayout->addWidget(refreshDirToolButton);
	topHBoxLayout->addWidget(remoteDirComboBox);

	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addLayout(topHBoxLayout);
	mainLayout->addWidget(remoteDirTreeView);
	mainLayout->addWidget(logTextEdit);
	mainLayout->addWidget(remoteDirStatusBar);
	mainLayout->setStretch(1,1);
	setLayout(mainLayout);

	//*******************************
	// treeview context menu
	contextMenu = new QMenu(this);
	dotdotAction = new QAction(tr("�ϼ�Ŀ¼"), this);
	downloadAction = new QAction(tr("����"), this);
	queueAction = new QAction(tr("����"), this);
	refreshAction = new QAction(tr("ˢ��"), this);
	sendToAction = contextMenu->addMenu(new QMenu(tr("���͵�"), this));
	editAction = new QAction(tr("�༭"), this);
	readAction = new QAction(tr("�鿴"), this);
	changePermissionAction = new QAction(tr("�����ļ�Ȩ��"), this);
	delAction = new QAction(tr("ɾ��"), this);
	renameAction = new QAction(tr("������"), this);
	newDirAction = new QAction(tr("�½��ļ���"), this);
	propertyAction = new QAction(tr("����"), this);
	contextMenu->addAction(dotdotAction);
	contextMenu->addSeparator();
	contextMenu->addAction(downloadAction);
	contextMenu->addAction(queueAction);
	contextMenu->addSeparator();
	contextMenu->addAction(refreshAction);
	contextMenu->addAction(sendToAction);
	contextMenu->addAction(editAction);
	contextMenu->addAction(readAction);
	contextMenu->addAction(changePermissionAction);
	contextMenu->addSeparator();
	contextMenu->addAction(delAction);
	contextMenu->addAction(renameAction);
	contextMenu->addAction(newDirAction);
	contextMenu->addSeparator();
	contextMenu->addAction(propertyAction);
	isListing = false;

/*	connect(remoteDirTableView, SIGNAL(doubleClicked(const QModelIndex &)), remoteDirTableModel, SLOT(setRootIndex(const QModelIndex &)));*/
    connect(connectButton, SIGNAL(clicked()), this, SLOT(connectOrDisconnect()));
	connect(dotdotDirToolButton, SIGNAL(clicked()), this, SLOT(dotdot()));
    connect(remoteDirTreeView, SIGNAL(doubleClicked(const QModelIndex &)), 
		this, SLOT(setRootIndex(const QModelIndex &)));
    connect(ftpClient, SIGNAL(listInfo(const QUrlInfo &)), 
		this, SLOT(ftpListInfo(const QUrlInfo &)));
    connect(ftpClient, SIGNAL(done(bool)), 
		this, SLOT(ftpDone(bool)));
    connect(ftpClient, SIGNAL(commandFinished(int,bool)), 
		this, SLOT(ftpCommandFinished(int,bool)));
    connect(ftpClient, SIGNAL(commandStarted(int)), 
		this, SLOT(ftpCommandStarted(int)));
	connect(ftpClient, SIGNAL(stateChanged(int)), 
		this, SLOT(ftpStateChanged(int)));
	connect(remoteDirTreeView, SIGNAL(pressed(const QModelIndex &)), 
		this, SLOT(showContextMenu(const QModelIndex &)));
	connect(this, SIGNAL(ftpCommandDone(QFtp::Command, bool)), 
		parentTinyFtp, SLOT(ftpCommandDone(QFtp::Command, bool)));
	connect(refreshDirToolButton, SIGNAL(clicked()), this, SLOT(refresh()));
    connect(remoteDirTreeModel, SIGNAL(editingFinished(const QModelIndex &)), this, SLOT(editingFinished(const QModelIndex &)));
	//*******************************
	// context menu slots & signals
	connect(dotdotAction, SIGNAL(triggered()), this, SLOT(dotdot()));
	connect(downloadAction, SIGNAL(triggered()), this, SLOT(download()));
	connect(queueAction, SIGNAL(triggered()), this, SLOT(queue()));
	connect(refreshAction, SIGNAL(triggered()), this, SLOT(refresh()));
	connect(editAction, SIGNAL(triggered()), this, SLOT(edit()));
	connect(readAction, SIGNAL(triggered()), this, SLOT(read()));
	connect(changePermissionAction, SIGNAL(triggered()), this, SLOT(changePermission()));
	connect(delAction, SIGNAL(triggered()), this, SLOT(del()));
	connect(renameAction, SIGNAL(triggered()), this, SLOT(rename()));
	connect(newDirAction, SIGNAL(triggered()), this, SLOT(newDir()));
	connect(propertyAction, SIGNAL(triggered()), this, SLOT(property()));
}

RemoteDirWidget::~RemoteDirWidget()
{

}

bool RemoteDirWidget::listing() const
{
	return isListing;
}

void RemoteDirWidget::setListing(bool isDoing)
{
	isListing = isDoing;
}

void RemoteDirWidget::writeLog(const QString &logData)
{
    logTextEdit->append("[" + QDateTime::currentDateTime().toString("hh:mm:ss")  + "] " + logData);
}

void RemoteDirWidget::connectToHost(const QString &address, const QString &port, 
    const QString &username/* = QString()*/, const QString &password/* = QString()*/)
{
    QString strAddress = address;

    urlAddress.setHost(strAddress);
	urlAddress.setPort(port.toInt());
	urlAddress.setUserName(username);
	urlAddress.setPassword(password);

	connectButton->setText(tr("����"));
	dotdotDirToolButton->setEnabled(false);
	refreshDirToolButton->setEnabled(false);
	remoteDirComboBox->setEnabled(false);

	if (isConnected()) {
		disconnect();
	}
    ftpClient->connectToHost(urlAddress.host(), urlAddress.port());
}

bool RemoteDirWidget::isConnected() const
{
	return ftpClient->state() != QFtp::Unconnected;
}

QString RemoteDirWidget::currentDirPathUrl() const
{
	return url(currentDirPath());
}

QString RemoteDirWidget::currentFilePathUrl() const
{
	return url(currentFilePath());
}

QString RemoteDirWidget::currentDirPath() const
{
    DirTreeModel *d = static_cast<DirTreeModel*>(remoteDirTreeView->model());
	return d ? d->currentDirPath() : QString();
}

QString RemoteDirWidget::currentFilePath() const
{
	Node *node = static_cast<Node*>(remoteDirTreeView->currentIndex().internalPointer());
	if (!node) {
		return QString();
	}
	return node->filePath;
}

void RemoteDirWidget::reconnect()
{
    connectButton->setText(tr("����"));
    /*connectButton->setEnabled(false);*/
    dotdotDirToolButton->setEnabled(false);
    refreshDirToolButton->setEnabled(false);
    remoteDirComboBox->setEnabled(false);
	ftpClient->connectToHost(urlAddress.host(), urlAddress.port());
}

void RemoteDirWidget::reset()
{
	refresh();
}

void RemoteDirWidget::closeEvent(QCloseEvent *event)
{
	if (isConnected()) {
        disconnect();
    }
}

void RemoteDirWidget::download()
{
    if (!isConnected()) {
        reconnect();
        return ;
    }
	currentCommand = CMD_DOWNLOAD;
	Node *node = static_cast<Node*>(remoteDirTreeView->currentIndex().internalPointer());
	QString path = currentFilePathUrl();
	int lstIdx = path.lastIndexOf(tr("/"));
	currentDownloadBaseDir = path.left(lstIdx);
	currentDownloadRelativeDir = path.mid(lstIdx);

	// 1. �ļ�ֱ������
	// 2. Ŀ¼�ݹ�����
	if (node->isDir) {
		pendingDownloadRelativeDirs.append(currentDownloadRelativeDir);
		processDirectory();
	} else {
		LocalDirWidget *l = parentTinyFtp->localCurrentWidget();
		currentDownloadLocalDir = l->currentDirPath();

		QFile *file = new QFile(currentDownloadLocalDir + tr("/")
			+ node->fileName);		

		if (!file->open(QIODevice::WriteOnly)) {
			writeLog(tr("Warning: Cannot write file %1: %2").arg(
				l->currentDirPath() + tr("/") + 
				file->fileName()).arg(file->errorString()));
		} else {
			ftpClient->get(encoded(path), file);
		}
		openedDownloadingFiles.append(file);
	}
}

void RemoteDirWidget::upload(const QString &filePath)
{
    if (!isConnected()) {
        reconnect();
        return ;
    }
    currentCommand = CMD_UPLOAD;
    QFileInfo fileInfo(filePath);
    currentUploadBaseDir = currentDirPathUrl();
    currentUploadRelativeDir = tr("/") + fileInfo.fileName();
    if (fileInfo.isDir()) {
        pendingUploadRelativeDirs.append(currentUploadRelativeDir);
        processDirectory();
    } else {
        QFile *file = new QFile(filePath);		
        if (!file->open(QIODevice::ReadOnly)) {
            writeLog(tr("Warning: Cannot read file %1: %2").arg(
                filePath).arg(file->errorString()));
        } else {
			ftpClient->cd(encoded(currentUploadBaseDir));
			ftpClient->put(file, encoded(fileInfo.fileName()));
		}
		openedUploadingFiles.append(file);
    }
}

void RemoteDirWidget::processDirectory()
{
	if (currentCommand == CMD_DOWNLOAD) {
		if (pendingDownloadRelativeDirs.isEmpty()) {
			//*******************************
			// �������
			writeLog(tr("�����ļ����������"));

			//*******************************
			// �ñ��ش��ڽ������ã���ʾ�ļ����غ��Ŀ¼��
			LocalDirWidget *l = parentTinyFtp->localCurrentWidget();
			l->reset();
			currentCommand = CMD_NONE;
			return ;
		}
		LocalDirWidget *l = parentTinyFtp->localCurrentWidget();
		currentDownloadRelativeDir = pendingDownloadRelativeDirs.takeFirst();
		currentDownloadLocalDir = l->currentDirPath() + currentDownloadRelativeDir;
		if (QFileInfo(currentDownloadLocalDir).exists()) {
			//*******************************
			// �Ƿ���и����ļ�
		}
		QDir().mkdir(currentDownloadLocalDir);
		ftpClient->cd(encoded(currentDownloadBaseDir + currentDownloadRelativeDir));
		ftpClient->list();
	} else if (currentCommand == CMD_UPLOAD) {
		if (pendingUploadRelativeDirs.isEmpty()) {
			//*******************************
			// �������
			writeLog(tr("�����ļ����ϴ����"));

			//*******************************
			// ��Զ�̴��ڽ������ã���ʾ�ļ����غ��Ŀ¼��
			RemoteDirWidget *r = parentTinyFtp->remoteCurrentWidget();
			r->reset();
			currentCommand = CMD_NONE;
			return ;
		}
		LocalDirWidget *l = parentTinyFtp->localCurrentWidget();
		currentUploadRelativeDir = pendingUploadRelativeDirs.takeFirst();
		currentUploadLocalDir = l->currentDirPath() + currentUploadRelativeDir;
		QString dirPath = currentUploadBaseDir + currentUploadRelativeDir;
		QString cacheFilePath = currentDirPath() + currentUploadRelativeDir;
		if (!QDir(cacheFilePath).exists()) {
			QDir().mkdir(cacheFilePath);
		}
		ftpClient->mkdir(encoded(dirPath));
		ftpClient->cd(encoded(dirPath));
		foreach (QFileInfo fileInfo, QDir(currentUploadLocalDir).entryInfoList(
			QDir::AllEntries | QDir::NoSymLinks | QDir::NoDotAndDotDot)) {
				if (fileInfo.isDir()) {
					pendingUploadRelativeDirs.append(currentUploadRelativeDir + tr("/") + fileInfo.fileName());
				} else {
					QFile *file = new QFile(fileInfo.absoluteFilePath());

					if (!file->open(QIODevice::ReadOnly)) {
						writeLog(tr("Warning: Cannot read file %1: %2").arg(
							QDir::fromNativeSeparators(
							file->fileName())).arg(file->errorString()));
					} else {
						ftpClient->put(file, encoded(fileInfo.fileName()));
					}
					openedUploadingFiles.append(file);
				}
		}
	} else if (currentCommand == CMD_DEL) {
        hasDir = false;
        if (pendingDelRelativeDirs.isEmpty()) {
			//*******************************
			// �������
			writeLog(tr("�����ļ���ɾ�����"));
            
			//*******************************
			// ��Զ�̴��ڽ������ã���ʾ�ļ����غ��Ŀ¼��
			RemoteDirWidget *r = parentTinyFtp->remoteCurrentWidget();
			r->reset();
			currentCommand = CMD_NONE;
			return ;
		}

		currentDelRelativeDir = pendingDelRelativeDirs.top();
        ftpClient->cd(encoded(currentDelBaseDir + currentDelRelativeDir));
        ftpClient->list();
	}
}

void RemoteDirWidget::listDirectoryFiles(const QString &dirName/* = ""*/)
{
	setListing(true);
	currentListDir = dirName.isEmpty() ? currentDirPathUrl() :
        QDir::fromNativeSeparators(QDir::cleanPath(currentDirPathUrl() + tr("/") + dirName));
	currentListLocalDir = QDir::fromNativeSeparators(QDir::cleanPath(cacheDir + currentListDir));
    delDir(currentListLocalDir);
	QDir().mkpath(currentListLocalDir);
	ftpClient->list(encoded(currentListDir));
}

void RemoteDirWidget::ftpListInfo(const QUrlInfo &urlInfo)
{
	if (listing()) {
		if (urlInfo.isFile()) {
            QString path = currentListLocalDir + tr("/")
                + decoded(urlInfo.name());
            QFile file(path);
            if (!file.exists()) {
                if (!file.open(QIODevice::WriteOnly)) {
                    writeLog(tr("Warning: Cannot create the cache file ") +
                        QDir::fromNativeSeparators(
                        file.fileName()) +
                        ": " + file.errorString());
                }
            }
		} else if (urlInfo.isDir() && !urlInfo.isSymLink() && urlInfo.name() != tr(".") && urlInfo.name() != tr("..")) {
			QString localDir = currentListLocalDir + tr("/") + decoded(urlInfo.name());
			delDir(localDir);
			QDir().mkpath(localDir);
		}
        if (filesInfoMap.count(decoded(urlInfo.name()))) {
            writeLog(tr("Error: filesInfoMap has the same key: %1").arg(decoded(urlInfo.name())));
            return ;
        }
        filesInfoMap[decoded(urlInfo.name())] = urlInfo;
	} 
	
	if (currentCommand == CMD_DOWNLOAD) {
		if (urlInfo.isFile()) {
            QFile *file = new QFile(currentDownloadLocalDir + tr("/")
                + decoded(urlInfo.name()));

            if (!file->open(QIODevice::WriteOnly)) {
                writeLog(tr("Warning: Cannot write file %1: %2").arg(
                    QDir::fromNativeSeparators(
                    file->fileName())).arg(file->errorString()));
            } else {
                ftpClient->get(urlInfo.name(), file);
            }
            openedDownloadingFiles.append(file);
		} else if (urlInfo.isDir() && !urlInfo.isSymLink()) {
			pendingDownloadRelativeDirs.append(currentDownloadRelativeDir + tr("/") + decoded(urlInfo.name()));
		}
	} else if (currentCommand == CMD_DEL) {
        if (urlInfo.isFile()) {
            QString dirPath = currentDelBaseDir + currentDelRelativeDir + tr("/") + urlInfo.name();
            QString path = cacheFilePath() + dirPath;
            QFile file(decoded(path));
            if (file.exists()) {
                file.remove();
            }
            ftpClient->remove(urlInfo.name());
        } else if (urlInfo.isDir() && !urlInfo.isSymLink()) {
            hasDir = true;
            pendingDelRelativeDirs.push(currentDelRelativeDir + tr("/") + decoded(urlInfo.name()));
        }
	}
}

void RemoteDirWidget::ftpDone(bool error)
{
	if (listing()) {
		if (currentListDir.isEmpty() || error) {
			writeLog(tr("Error: ") + ftpClient->errorString());
			filesInfoMap.clear();
			setListing(false);
			return ;
		} else {
			remoteDirTreeView->setRootPath(currentListLocalDir);
			remoteDirTreeView->resizeColumnsToContents();
			if (remoteDirTreeView->rowCount()) {
				for (int row = 0; row < remoteDirTreeView->rowCount(); row++) {
                    Node *n1 = remoteDirTreeView->item(row, 1);
                    Node *n2 = remoteDirTreeView->item(row, 3);
                    QUrlInfo urlInfo = filesInfoMap[n1->fileName];
                    if (n1->isFile) {
                        n1->fileSize = urlInfo.size();
                    }
                    n2->modifyDate = urlInfo.lastModified().toString("yyyy/MM/dd hh:mm");
				}
			}
			filesInfoMap.clear();
			setListing(false);
		}
	} 
	
	if (currentCommand == CMD_DOWNLOAD) {
		if (error) {
			writeLog(tr("Error: ") + ftpClient->errorString());
			qDeleteAll(openedDownloadingFiles);
			openedDownloadingFiles.clear();
			currentCommand = CMD_NONE;
			return ;
		} else {
            QString dirPath = currentDownloadBaseDir + currentDownloadRelativeDir;
			if (openedDownloadingFiles.count() == 1) {
                QFileInfo fileInfo(*openedDownloadingFiles.first());
				writeLog(tr("�ļ����� %1 �� %2 ���").arg(
					dirPath + tr("/") + fileInfo.fileName()).arg(
                    currentDownloadLocalDir + tr("/") + fileInfo.fileName()));
			} else {
				writeLog(tr("�ļ������� %1 �� %2 ���").arg(
					dirPath).arg(currentDownloadLocalDir));
			}

			qDeleteAll(openedDownloadingFiles);
			openedDownloadingFiles.clear();

			processDirectory();
		}
	} else if (currentCommand == CMD_UPLOAD) {
		if (error) {
			writeLog(tr("Error: ") + ftpClient->errorString());
			qDeleteAll(openedUploadingFiles);
			openedUploadingFiles.clear();
			currentCommand = CMD_NONE;
			return ;
		} else {
            QString dirPath = currentUploadBaseDir + currentUploadRelativeDir;
			if (openedUploadingFiles.count() == 1) {
				QFileInfo fileInfo(*openedUploadingFiles.first());
				writeLog(tr("�ļ��ϴ� %1 �� %2 ���").arg(
					currentUploadLocalDir + tr("/") + fileInfo.fileName()).arg(
					dirPath + tr("/") + fileInfo.fileName())
					);
			} else {
				writeLog(tr("�ļ����ϴ� %1 �� %2 ���").arg(
					currentUploadLocalDir).arg(dirPath));
			}

			qDeleteAll(openedUploadingFiles);
			openedUploadingFiles.clear();

			processDirectory();
		}
	} else if (currentCommand == CMD_MKDIR) {
		if (error) {
			writeLog(tr("Error: ") + ftpClient->errorString());
			currentCommand = CMD_NONE;
			return ;
		} else {
			refresh();
			currentCommand = CMD_NONE;
		}
	} else if (currentCommand == CMD_DEL) {
        if (error) {
            writeLog(tr("Error: ") + ftpClient->errorString());
            currentCommand = CMD_NONE;
            return ;
        } else {
            if (!hasDir && !pendingDelRelativeDirs.isEmpty()) {
                QString dirPath = currentDelBaseDir + pendingDelRelativeDirs.pop();
                QString delLocalDir = cacheFilePath() + dirPath;
                delDir(delLocalDir);
                ftpClient->rmdir(encoded(dirPath));
                writeLog(tr("�ļ��� %1 ɾ�����").arg(dirPath));
            }
            processDirectory();
        }
	} else if (currentCommand == CMD_RENAME) {
        if (error) {
            writeLog(tr("Error: ") + ftpClient->errorString());
            currentCommand = CMD_NONE;
            return ;
        } else {
            writeLog(tr("�ѽ��ļ� %1 ������Ϊ %2").arg(
                currentOldFileName).arg(currentNewFileName));
            currentCommand = CMD_NONE;
        }
	}
}

void RemoteDirWidget::setRootIndex(const QModelIndex &index)
{
	if (!index.isValid()) {
		return ;
	}
    if (!isConnected()) {
        reconnect();
        return ;
    }
	Node *node = static_cast<Node*>(index.internalPointer());
	if (node->fileName == tr("..")) {
		if (currentDirPathUrl() == tr("/")) {
            return ;
		} 
		dotdot();
	} else if (node->isDir) {
        listDirectoryFiles(node->fileName);
	}
}

void RemoteDirWidget::showContextMenu(const QModelIndex &index)
{
	if (QApplication::mouseButtons() == Qt::RightButton) {
		//*******************************
		// Ĭ�� ʹ�� ���в˵���
		QList<QAction*> actions = contextMenu->actions();
		foreach (QAction* action, actions)
			action->setEnabled(true);

        //*******************************
        // ��ǰδ�����ϻ���������δִ���꣬���������ѡ��
        if (/*!isConnected() || */ftpClient->hasPendingCommands()) {
            foreach (QAction* action, actions)
                action->setEnabled(false);
            goto menuexec;
        }

		//*******************************
		// ���� ���͵� �˵�


		//*******************************
		// ���� ���������Ĳ˵��� ʹ�� ״̬
        {
            Node *node = static_cast<Node*>(index.internalPointer());
            QFileInfo fileInfo(node->filePath);
            if (fileInfo.isDir() && node->fileName == tr("..")) {
                foreach (QAction* action, actions)
                    action->setEnabled(false);
            } else if (true/*fileInfo.isFile()*/) {
                if (!fileInfo.isWritable()) {
                    editAction->setEnabled(false);
                    /*delAction->setEnabled(false);*/
                    renameAction->setEnabled(false);
                }
                if (!fileInfo.isReadable()) {
                    readAction->setEnabled(false);
                    downloadAction->setEnabled(false);
                    queueAction->setEnabled(false);
                }
                if (currentDirPathUrl() == tr("/")) {
                    dotdotAction->setEnabled(false);
                } else {
                    Node *dotdotNode = remoteDirTreeView->item(0, 0);
                    dotdotAction->setEnabled(dotdotNode->fileName == tr(".."));
                }
            }
        }
menuexec:
		contextMenu->exec(QCursor::pos());
	}
}

void RemoteDirWidget::dotdot()
{
    if (!isConnected()) {
        reconnect();
        return ;
    }
    listDirectoryFiles(tr(".."));
	
	if (currentDirPathUrl() == tr("/"))
		dotdotDirToolButton->setEnabled(false);
	else {
		Node *dotdotNode = remoteDirTreeView->item(0, 0);
		dotdotDirToolButton->setEnabled(dotdotNode->fileName == tr(".."));
	}
}

void RemoteDirWidget::queue()
{
    if (!isConnected()) {
        reconnect();
        return ;
    }
}

void RemoteDirWidget::refresh()
{
    if (!isConnected()) {
        reconnect();
        return ;
    }
	listDirectoryFiles();
}

void RemoteDirWidget::edit()
{
    if (!isConnected()) {
        reconnect();
        return ;
    }
}

void RemoteDirWidget::read()
{
    if (!isConnected()) {
        reconnect();
        return ;
    }
}

void RemoteDirWidget::changePermission()
{
    if (!isConnected()) {
        reconnect();
        return ;
    }
}

void RemoteDirWidget::del()
{
    if (!isConnected()) {
        reconnect();
        return ;
    }
    currentCommand = CMD_DEL;
    QString filePath = currentFilePath();
    QFileInfo fileInfo(filePath);
    currentDelBaseDir = currentDirPathUrl();
    currentDelRelativeDir = tr("/") + fileInfo.fileName();
    if (fileInfo.isDir()) {
        pendingDelRelativeDirs.push(currentDelRelativeDir);
        processDirectory();
    } else {
        //*******************************
        // ɾ������cache�ļ�
        QFile file(filePath);
        if (file.exists()) {
            file.remove();
        }

        //*******************************
        // ɾ��Զ���ļ�
        ftpClient->cd(encoded(currentDelBaseDir));
        ftpClient->remove(encoded(fileInfo.fileName()));
    }
}

void RemoteDirWidget::rename()
{
    if (!isConnected()) {
        reconnect();
        return ;
    }
}

void RemoteDirWidget::newDir()
{
    if (!isConnected()) {
        reconnect();
        return ;
    }
	QString dirName = tr("�½��ļ���");
	QDir dir(currentDirPath());
	if (!dir.exists(dirName)) {
		goto succeed;
		return ;
	}

	int i = 2;
	while (dir.exists(dirName)) {
		dirName = tr("�½��ļ���(%1)").arg(i);
		i++;
	}
succeed:
	currentCommand = CMD_MKDIR;
	dir.mkdir(dirName);
	ftpClient->cd(encoded(currentDirPathUrl()));
	ftpClient->mkdir(encoded(dirName));
	return ;
}

void RemoteDirWidget::property()
{
    if (!isConnected()) {
        reconnect();
        return ;
    }
}

void RemoteDirWidget::ftpCommandStarted(int)
{
	QFtp::Command command = ftpClient->currentCommand();
    if (command == QFtp::ConnectToHost) {
        writeLog(tr("���ڽ�����ַ %1, �˿� %2").arg(
			urlAddress.host()).arg(urlAddress.port()));
    } else if (command == QFtp::Login) {
		if (urlAddress.userName().isEmpty()) {
			writeLog(tr("�������ӵ�������...(%1)").arg(tr("����")));
		} else {
			writeLog(tr("�������ӵ�������...(USER %1, PASS (%2))").arg(
			urlAddress.userName()).arg(tr("����")));
		}
    } else if (command == QFtp::Close) {
    } else if (command == QFtp::List) {
    } else if (command == QFtp::Cd) {
    } else if (command == QFtp::Get) {
    } else if (command == QFtp::Put) {
    } else if (command == QFtp::Remove) {
    } else if (command == QFtp::Mkdir) {
    } else if (command == QFtp::Rmdir) {
    } else if (command == QFtp::Rename) {
    }
}

void RemoteDirWidget::ftpStateChanged(int state)
{
	QFtp::Command cmd = ftpClient->currentCommand();
	//*******************************
	// ��ʱ�ѱ��Զ��Ͽ�����
	if (cmd == QFtp::None) {
		writeLog(tr("�Ѵӷ������Ͽ�����"));
		disconnect();
        delDir(cacheDir);
        dotdotDirToolButton->setEnabled(false);
        refreshDirToolButton->setEnabled(false);
        remoteDirComboBox->setEnabled(false);
        connectButton->setText(tr("����"));
        connectButton->setEnabled(true);
	}
}

void RemoteDirWidget::ftpCommandFinished(int,bool error)
{
	QFtp::Command command = ftpClient->currentCommand();
    if (command == QFtp::ConnectToHost) {
        if (!error) {
            writeLog(tr("�����ӵ�������, ���ڵȴ���Ӧ..."));
			ftpClient->login(urlAddress.userName(), urlAddress.password());
        } else {
            writeLog(tr("�������ӵ�������(����δ�ҵ�: %1)").arg(urlAddress.host()));
			if (isConnected()) {
                disconnect();
			}
        }
    } else if (command == QFtp::Login) {
        if (!error) {
            writeLog(tr("���ӳɹ�(��¼�ɹ�)"));
			QString path = urlAddress.path();
            cacheDir = tr("cache") + tr("/") + urlAddress.host();
			QDir().mkdir(tr("cache"));
#if 1
			SetFileAttributesA("cache", FILE_ATTRIBUTE_HIDDEN);
#endif
			delete remoteDirTreeView->model();
			remoteDirTreeView->setModel(new DirTreeModel(this));
			listDirectoryFiles(path);

			connectButton->setText(tr("�Ͽ�"));
			connectButton->setEnabled(true);
			dotdotDirToolButton->setEnabled(currentDirPathUrl() != tr("/"));
			refreshDirToolButton->setEnabled(true);
			remoteDirComboBox->setEnabled(true);
        } else {
            writeLog(tr("����ʧ��(��¼ʧ��), �û������������!(%1)").arg(ftpClient->errorString()));
			if (isConnected()) {
                disconnect();
			}
        }
    } else if (command == QFtp::Close) {
        if (!error) {
            writeLog(tr("�Ѵӷ������Ͽ�����"));
            delDir(cacheDir);
            dotdotDirToolButton->setEnabled(false);
            refreshDirToolButton->setEnabled(false);
            remoteDirComboBox->setEnabled(false);
            connectButton->setText(tr("����"));
            connectButton->setEnabled(true);
        } else {
            writeLog(tr("�޷��Ͽ�����(%1)").arg(ftpClient->errorString()));
        }
    } else if (command == QFtp::List) {
    } else if (command == QFtp::Cd) {
    } else if (command == QFtp::Get) {
    } else if (command == QFtp::Put) {
    } else if (command == QFtp::Remove) {
    } else if (command == QFtp::Mkdir) {
    } else if (command == QFtp::Rmdir) {
    } else if (command == QFtp::Rename) {
    }

	emit ftpCommandDone(command, error);
}

QString RemoteDirWidget::url(const QString &str) const
{
	QString s = str.mid(str.indexOf(urlAddress.host()) + urlAddress.host().length());
	return s == "" ? tr("/") : s;
}

QString RemoteDirWidget::cacheFilePath() const
{
    return QDir::fromNativeSeparators(QDir().absolutePath() + tr("/") + cacheDir);
}

void RemoteDirWidget::connectOrDisconnect()
{
    if (connectButton->text() == tr("����")) {
        connectButton->setText(tr("�Ͽ�"));
        reconnect();
    } else {
        connectButton->setText(tr("����"));
        disconnect();
    }
}

void RemoteDirWidget::disconnect()
{
    if (isConnected()) {
        ftpClient->close();
    }
}

void RemoteDirWidget::editingFinished(const QModelIndex &index)
{
    Node *n = static_cast<Node*>(index.internalPointer());
    QString oldName = QFileInfo(n->filePath).fileName();
    currentOldFileName = "";
    currentNewFileName = "";

    if (!isConnected()) {
        n->fileName = oldName;
        reconnect();
        return ;
    }

    if (oldName != n->fileName) {
        currentCommand = CMD_RENAME;
        currentOldFileName = currentDirPathUrl() + tr("/") + oldName;
        currentNewFileName = currentDirPathUrl() + tr("/") + n->fileName;
        bool is = QFile::rename(n->filePath, n->dirPath + tr("/") + n->fileName);
        n->filePath = n->dirPath + tr("/") + n->fileName;
        ftpClient->cd(encoded(currentDirPathUrl()));
        ftpClient->rename(encoded(oldName), encoded(n->fileName));
        remoteDirTreeView->resizeColumnsToContents();
    }
}
