#include "common.h"
#include "ftpclient.h"
#include "tinyftp.h"
#include "localdirwidget.h"
#include "remotedirwidget.h"

FTPClient::FTPClient(QObject *parent)
	: QFtp(parent)
{
	connect(this, SIGNAL(done(bool)), this, SLOT(ftpDone(bool)));
	connect(this, SIGNAL(commandFinished(int,bool)), this, SLOT(ftpCommandFinished(int,bool)));
	connect(this, SIGNAL(listInfo(const QUrlInfo &)), this, SLOT(ftpListInfo(const QUrlInfo &)));
	curTask = 0;
}

FTPClient::~FTPClient()
{

}

bool FTPClient::idle()
{
	return state() == QFtp::Unconnected;
}

void FTPClient::sendMsg(const QString &msg)
{
	emit ftpMsg(msg);
}

void FTPClient::download(const QString remoteDirPathUrl, const QString localDirPath,
	const QString fileName, const bool isDir
	)
{
	{
		TaskData taskData = curTask->taskData();
		sendMsg(tr("��ʼ���� %1 �� %2").arg(taskData.downloadRemoteDirPathUrl + 
			tr("/") + taskData.fileName).arg(
			taskData.downloadLocalDirPath + tr("/") + taskData.fileName));
	}

	curCommand = CMD_DOWNLOAD;

	currentDownloadBaseDirPathUrl = /*path.left(lstIdx);*//*currentDirPathUrl()*/remoteDirPathUrl;
	currentDownloadRelativeDirPathUrl = /*path.mid(lstIdx);*/tr("/") + /*node->fileName*/fileName;
	currentDownloadLocalBaseDirPath = localDirPath;
	// 1. �ļ�ֱ������
	// 2. Ŀ¼�ݹ�����
	if (/*node->isDir*/isDir) {
		pendingDownloadRelativeDirPathUrls.append(currentDownloadRelativeDirPathUrl);
		processDirectory();
	} else {
		/*LocalDirWidget *l = parentTinyFtp->localCurrentWidget();*/
		currentDownloadLocalDirPath = /*l->currentDirPath()*/currentDownloadLocalBaseDirPath;

		QFile *file = new QFile(currentDownloadLocalDirPath + tr("/")
			+ /*node->fileName*/fileName);		

		if (!file->open(QIODevice::WriteOnly)) {
			sendMsg(tr("Warning: Cannot write file %1: %2").arg(
				/*l->currentDirPath()*/currentDownloadLocalBaseDirPath + tr("/") + 
				file->fileName()).arg(file->errorString()));
		} else {
			this->get(encoded(/*path*/remoteDirPathUrl + tr("/") + fileName), file);
		}
		openedDownloadingFiles.append(file);
	}
}

void FTPClient::upload(const QString remoteDirPathUrl, const QString filePath)
{
	{
		TaskData taskData = curTask->taskData();
		sendMsg(tr("��ʼ�ϴ� %1 �� %2").arg(taskData.uploadRemoteDirPathUrl + 
			tr("/") + taskData.fileName).arg(
			taskData.uploadLocalDirPath + tr("/") + taskData.fileName));
	}

	curCommand = CMD_UPLOAD;
	QFileInfo fileInfo(filePath);
	currentUploadBaseDirPathUrl = /*currentDirPathUrl()*/remoteDirPathUrl;
	currentUploadRelativeDirPathUrl = tr("/") + fileInfo.fileName();
	currentUploadLocalBaseDirPath = fileInfo.absolutePath();
	if (fileInfo.isDir()) {
		pendingUploadRelativeDirPathUrls.append(currentUploadRelativeDirPathUrl);
		processDirectory();
	} else {
		QFile *file = new QFile(filePath);		
		if (!file->open(QIODevice::ReadOnly)) {
			sendMsg(tr("Warning: Cannot read file %1: %2").arg(
				filePath).arg(file->errorString()));
		} else {
			this->cd(encoded(currentUploadBaseDirPathUrl));
			this->put(file, encoded(fileInfo.fileName()));
		}
		openedUploadingFiles.append(file);
	}
}

void FTPClient::setCurrentTask(Task *task)
{
	if (!curTask) {
		curTask = task;
		curTask->setTaskStatus(taskStatus_Doing);
		sendMsg(tr(">>>>>>>>>>>��ʼ����[%1]<<<<<<<<<<<").arg(curTask->taskName()));
	} else {
		sendMsg(tr("Error : ��ǰ���������ڽ���[%1]").arg(curTask->taskName()));
	}
}

Task * FTPClient::currentTask()
{
	return curTask;
}

void FTPClient::ftpListInfo(const QUrlInfo &urlInfo)
{
	if (curCommand == CMD_DOWNLOAD) {
		if (urlInfo.isFile()) {
			QFile *file = new QFile(currentDownloadLocalDirPath + tr("/")
				+ decoded(urlInfo.name()));

			if (!file->open(QIODevice::WriteOnly)) {
				sendMsg(tr("Warning: Cannot write file %1: %2").arg(
					QDir::fromNativeSeparators(
					file->fileName())).arg(file->errorString()));
			} else {
				this->get(urlInfo.name(), file);
			}
			openedDownloadingFiles.append(file);
		} else if (urlInfo.isDir() && !urlInfo.isSymLink()) {
			pendingDownloadRelativeDirPathUrls.append(currentDownloadRelativeDirPathUrl + tr("/") + decoded(urlInfo.name()));
		}
	} /*else if (currentCommand == CMD_DEL) {
		if (urlInfo.isFile()) {
			QString dirPath = currentDelBaseDir + currentDelRelativeDir + tr("/") + urlInfo.name();
			QString path = cacheFilePath() + dirPath;
			QFile file(decoded(path));
			if (file.exists()) {
				file.remove();
			}
			this->remove(urlInfo.name());
		} else if (urlInfo.isDir() && !urlInfo.isSymLink()) {
			hasDir = true;
			pendingDelRelativeDirs.push(currentDelRelativeDir + tr("/") + decoded(urlInfo.name()));
		}
	}*/
}

void FTPClient::ftpDone(bool error)
{
	if (curCommand == CMD_DOWNLOAD) {
		if (error) {
			sendMsg(tr("Error: ") + this->errorString());
			qDeleteAll(openedDownloadingFiles);
			openedDownloadingFiles.clear();
			curCommand = CMD_NONE;
			return ;
		} else {
			QString dirPath = currentDownloadBaseDirPathUrl + currentDownloadRelativeDirPathUrl;
			if (openedDownloadingFiles.count() == 1) {
				QFileInfo fileInfo(*openedDownloadingFiles.first());
				sendMsg(tr("�ļ����� %1 �� %2 ���").arg(
					dirPath + tr("/") + fileInfo.fileName()).arg(
					currentDownloadLocalDirPath + tr("/") + fileInfo.fileName()));
			} else {
				sendMsg(tr("�ļ������� %1 �� %2 ���").arg(
					dirPath).arg(currentDownloadLocalDirPath));
			}

			qDeleteAll(openedDownloadingFiles);
			openedDownloadingFiles.clear();

			processDirectory();
		}
	} else if (curCommand == CMD_UPLOAD) {
		if (error) {
			sendMsg(tr("Error: ") + this->errorString());
			qDeleteAll(openedUploadingFiles);
			openedUploadingFiles.clear();
			curCommand = CMD_NONE;
			return ;
		} else {
			QString dirPath = currentUploadBaseDirPathUrl + currentUploadRelativeDirPathUrl;
			if (openedUploadingFiles.count() == 1) {
				QFileInfo fileInfo(*openedUploadingFiles.first());
				sendMsg(tr("�ļ��ϴ� %1 �� %2 ���").arg(
					currentUploadLocalDirPath + tr("/") + fileInfo.fileName()).arg(
					dirPath + tr("/") + fileInfo.fileName())
					);
			} else {
				sendMsg(tr("�ļ����ϴ� %1 �� %2 ���").arg(
					currentUploadLocalDirPath).arg(dirPath));
			}

			qDeleteAll(openedUploadingFiles);
			openedUploadingFiles.clear();

			processDirectory();
		}
	} /*else if (currentCommand == CMD_DEL) {
		if (error) {
			writeLog(tr("Error: ") + this->errorString());
			currentCommand = CMD_NONE;
			return ;
		} else {
			if (!hasDir && !pendingDelRelativeDirs.isEmpty()) {
				QString dirPath = currentDelBaseDir + pendingDelRelativeDirs.pop();
				QString delLocalDir = cacheFilePath() + dirPath;
				delDir(delLocalDir);
				this->rmdir(encoded(dirPath));
				writeLog(tr("�ļ��� %1 ɾ�����").arg(dirPath));
			}
			processDirectory();
		}
	}*/
}

void FTPClient::ftpCommandFinished(int,bool error)
{
	QFtp::Command command = this->currentCommand();
	QString errorStr = "";
	if (error) {
		errorStr = this->errorString();
	}
	if (command == QFtp::ConnectToHost) {
	} else if (command == QFtp::Login) {
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

void FTPClient::processDirectory()
{
	if (curCommand == CMD_DOWNLOAD) {
		if (pendingDownloadRelativeDirPathUrls.isEmpty()) {
			//*******************************
			// �������
			sendMsg(tr("�����ļ����������"));
			sendMsg(tr(">>>>>>>>>>>����[%1]���<<<<<<<<<<<").arg(curTask->taskName()));
			curTask->setTaskStatus(taskStatus_Done);
			curTask = 0;

			//*******************************
			// �ñ��ش��ڽ������ã���ʾ�ļ����غ��Ŀ¼��
//  			LocalDirWidget *l = parentTinyFtp->localCurrentWidget();
//  			l->reset();
			emit refreshLocalDirWidget();
			curCommand = CMD_NONE;
			this->close();	// �Ͽ�����
			return ;
		}
		//LocalDirWidget *l = parentTinyFtp->localCurrentWidget();
		currentDownloadRelativeDirPathUrl = pendingDownloadRelativeDirPathUrls.takeFirst();
		currentDownloadLocalDirPath = /*l->currentDirPath()*/currentDownloadLocalBaseDirPath + currentDownloadRelativeDirPathUrl;
		if (QFileInfo(currentDownloadLocalDirPath).exists()) {
			//*******************************
			// �Ƿ���и����ļ�
		}
		QDir().mkdir(currentDownloadLocalDirPath);
		this->cd(encoded(currentDownloadBaseDirPathUrl + currentDownloadRelativeDirPathUrl));
		this->list();
	} else if (curCommand == CMD_UPLOAD) {
		if (pendingUploadRelativeDirPathUrls.isEmpty()) {
			//*******************************
			// �������
			sendMsg(tr("�����ļ����ϴ����"));
			sendMsg(tr(">>>>>>>>>>>����[%1]���<<<<<<<<<<<").arg(curTask->taskName()));
			curTask->setTaskStatus(taskStatus_Done);
			curTask = 0;

			//*******************************
			// ��Զ�̴��ڽ������ã���ʾ�ļ����غ��Ŀ¼��
// 			RemoteDirWidget *r = parentTinyFtp->remoteCurrentWidget();
// 			r->reset();
			emit refreshRemoteDirWidget();
			curCommand = CMD_NONE;
			this->close();	// �Ͽ�����
			return ;
		}
		/*LocalDirWidget *l = parentTinyFtp->localCurrentWidget();*/
		currentUploadRelativeDirPathUrl = pendingUploadRelativeDirPathUrls.takeFirst();
		currentUploadLocalDirPath = /*l->currentDirPath()*/currentUploadLocalBaseDirPath + currentUploadRelativeDirPathUrl;
		QString dirPath = currentUploadBaseDirPathUrl + currentUploadRelativeDirPathUrl;
		QString cacheFilePath = QDir().path() + currentUploadRelativeDirPathUrl;
		if (!QDir(cacheFilePath).exists()) {
			QDir().mkdir(cacheFilePath);
		}
		this->mkdir(encoded(dirPath));
		this->cd(encoded(dirPath));
		foreach (QFileInfo fileInfo, QDir(currentUploadLocalDirPath).entryInfoList(
			QDir::AllEntries | QDir::NoSymLinks | QDir::NoDotAndDotDot)) {
				if (fileInfo.isDir()) {
					pendingUploadRelativeDirPathUrls.append(currentUploadRelativeDirPathUrl + tr("/") + fileInfo.fileName());
				} else {
					QFile *file = new QFile(fileInfo.absoluteFilePath());

					if (!file->open(QIODevice::ReadOnly)) {
						sendMsg(tr("Warning: Cannot read file %1: %2").arg(
							QDir::fromNativeSeparators(
							file->fileName())).arg(file->errorString()));
					} else {
						this->put(file, encoded(fileInfo.fileName()));
					}
					openedUploadingFiles.append(file);
				}
		}
	} /*else if (currentCommand == CMD_DEL) {
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
        this->cd(encoded(currentDelBaseDir + currentDelRelativeDir));
        this->list();
	}*/
}
