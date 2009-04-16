#include <QApplication>
#include <QTreeView>
#include <QAbstractItemModel>
#include <QList>
#include <QPushButton>
#include <QHBoxLayout>
#include <qstring.h>
#include <assert.h>
#include "ntreg.h"

class RegistryItem
{
	struct hive *hive;
	RegistryItem* parent;
	QString name;
	bool enumerated;
	QList<RegistryItem*> subkeys;
public:
	RegistryItem( struct hive *h, RegistryItem* p, const QString& n );
	~RegistryItem();
	QString getPath() const;
	RegistryItem* getParent();
	RegistryItem* getChild( int n );
	int getSubkeyCount();
	const QString& getName() const;
	int getNumber() const;
protected:
	void enumerateChildren();
};

