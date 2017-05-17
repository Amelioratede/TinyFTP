#include "queuewidget.h"

QueueWidget::QueueWidget(const QString &title, QWidget * parent/* = 0*/)
	: QDockWidget(title, parent)
{
	tabWidget = new QTabWidget(this);
	queueTreeWidget = new QTreeWidget(this);
	queueTreeWidget->header()->setStretchLastSection(true);
	queueTreeWidget->setAlternatingRowColors(true);
	queueTreeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
	queueTreeWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
	queueTreeWidget->setSortingEnabled(true);
	queueTreeWidget->setRootIsDecorated(false);
    queueTreeWidget->setHeaderLabels(QStringList() << tr("����") << tr("��С") << tr("������") 
        << tr("Դ·��") << tr("Ŀ��·��") << tr("״̬") << tr("����") << tr("�ٶ�") << tr("ʣ��ʱ��"));

	setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
	setFeatures(QDockWidget::DockWidgetVerticalTitleBar);
	tabWidget->addTab(queueTreeWidget, tr("����"));
	tabWidget->setTabPosition(QTabWidget::South);
	setWidget(tabWidget);
}

QueueWidget::~QueueWidget()
{

}
