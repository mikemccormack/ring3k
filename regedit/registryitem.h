
#ifndef __REGEDIT_REGISTRYITEM_H__
#define __REGEDIT_REGISTRYITEM_H__

#include <QList>
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

#endif // __REGEDIT_REGISTRYITEM_H__
