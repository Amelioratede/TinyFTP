#ifndef FTPCLIENT_H
#define FTPCLIENT_H

#include <QFtp>
#include <QtGui>
#include "queuewidget.h"

class Task;
class TinyFTP;
class FTPClient : public QFtp
{
	Q_OBJECT
public:
	FTPClient(QObject *parent = 0);
	~FTPClient();
	bool idle();
	void sendMsg(const QString &msg);
	void download(const QString &remoteDirUrl, const QString &localDir,
		const QString &fileName, const bool &isDir);
	void upload(const QString &remoteDirPathUrl, const QString &filePath);
signals:
	void ftpMsg(const QString &msg);
	private slots:
		void ftpListInfo(const QUrlInfo &urlInfo);
		void ftpDone(bool error);
private:
	void processDirectory();
	QString currentDownloadBaseDirPathUrl;		// "\xxx" ��ͷ
	QString currentDownloadRelativeDirPathUrl;	// "\xxx" ��ͷ
	// ftp ·��:currentDownloadBaseDir + currentDownloadRelativeDir
	QString currentDownloadLocalDirPath;
	QStringList pendingDownloadRelativeDirPathUrls;

	QString currentUploadBaseDirPathUrl;
	QString currentUploadRelativeDirPathUrl;
	QString currentUploadLocalDirPath;
	QStringList pendingUploadRelativeDirPathUrls;

// 	QString currentDelBaseDir;
// 	QString currentDelRelativeDir;
// 	//QString currentDelLocalDir;
// 	QStack<QString> pendingDelRelativeDirs;
// 	bool hasDir;

	QUrl urlAddress;
	FtpCommand currentCommand;
	QList<QFile *> openedDownloadingFiles;
	QList<QFile *> openedUploadingFiles;
	Task task;
	TinyFTP *parentTinyFtp;
};

#endif // FTPCLIENT_H
