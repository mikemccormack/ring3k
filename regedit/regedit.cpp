/*
 * Registry editor
 *
 * Copyright 2009 Mike McCormack
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

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

RegistryItem::RegistryItem( struct hive *h, RegistryItem* p, const QString& n ) :
	hive( h ),
	parent( p ),
	name( n ),
	enumerated( false )
{
}

RegistryItem::~RegistryItem()
{
}

QString RegistryItem::getPath() const
{
	QString path;
	if (parent)
	{
		path = parent->getPath();
		path.append( name );
		path.append( "\\" );
	}
	else
	{
		path = name;
	}
	return path;
}

RegistryItem* RegistryItem::getParent()
{
	return parent;
}

void RegistryItem::enumerateChildren()
{
	if (enumerated)
		return;

	struct ex_data key;
	memset( &key, 0, sizeof key );
	QString path = getPath();
	char *utf8_path = strdup(path.toUtf8().data());
	int i = 0;
	while (1)
	{
		int r = nk_get_subkey( hive, utf8_path, 0, i, &key );
		if (r < 0)
			break;
		QString kn( key.name );
		RegistryItem* item = new RegistryItem( hive, this, kn );
		//free( key.name );
		subkeys.append( item );
		i++;
	}
	free( utf8_path );
	enumerated = true;
}


RegistryItem* RegistryItem::getChild( int n )
{
	enumerateChildren();
	return subkeys.at( n );
}

int RegistryItem::getSubkeyCount()
{
	enumerateChildren();
	return subkeys.count();
}

const QString& RegistryItem::getName() const
{
	return name;
}

int RegistryItem::getNumber() const
{
	if (!parent)
		return 0;
	return parent->subkeys.lastIndexOf( const_cast<RegistryItem*>(this) );
}

class RegistryItemModel : public QAbstractItemModel
{
	struct hive *hive;
	RegistryItem *root;

public:
	RegistryItemModel(RegistryItem* root, struct hive *h);
	virtual Qt::ItemFlags flags(const QModelIndex &index) const;
	virtual QModelIndex index(int,int,const QModelIndex&) const;
	virtual int rowCount(const QModelIndex& index) const;
	virtual int columnCount(const QModelIndex& index) const;
	virtual QVariant data(const QModelIndex&, int) const;
	virtual QModelIndex parent(const QModelIndex & index) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
};

RegistryItemModel::RegistryItemModel(RegistryItem* r, struct hive *h) :
	hive( h ),
	root( r )
{
}

Qt::ItemFlags RegistryItemModel::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return 0;

	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant RegistryItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
		return "registry";
	return QVariant();
}

QModelIndex RegistryItemModel::index(int row, int column, const QModelIndex& parent) const
{
	if (!hasIndex(row, column, parent))
		return QModelIndex();

	RegistryItem *parentItem, *item;
	if (!parent.isValid())
		parentItem = root;
	else
		parentItem = static_cast<RegistryItem*>( parent.internalPointer() );

	item = parentItem->getChild( row );

	if (!item) return QModelIndex();

	assert( row == item->getNumber());

	return createIndex( row, column, item );
}

QModelIndex RegistryItemModel::parent(const QModelIndex &index) const
{
	if (!index.isValid())
		return QModelIndex();
	RegistryItem* item = static_cast<RegistryItem*>( index.internalPointer() );
	RegistryItem* parent = item->getParent();
	if (parent == root)
		return QModelIndex();

	return createIndex( parent->getNumber(), 0, parent );
}

int RegistryItemModel::columnCount(const QModelIndex& parent) const
{
	if (!parent.isValid())
		return 1;

	RegistryItem* parentItem = static_cast<RegistryItem*>( parent.internalPointer() );
	if (parentItem->getSubkeyCount())
		return 1;
	else
		return 0;
}

int RegistryItemModel::rowCount(const QModelIndex& parent) const
{
	if (parent.column()>0)
		return 0;

	RegistryItem* parentItem;
	if (!parent.isValid())
		parentItem = root;
	else
		parentItem = static_cast<RegistryItem*>( parent.internalPointer() );

	return parentItem->getSubkeyCount();
}

QVariant RegistryItemModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid())
		return QVariant();
	if (role != Qt::DisplayRole)
		return QVariant();

	RegistryItem* item = static_cast<RegistryItem*>( index.internalPointer() );
	return item->getName();
}

int main( int argc, char **argv )
{
	QApplication app(argc, argv);

	if (argc < 2)
	{
		fprintf(stderr, "Need the name of registry hive\n");
		return 1;
	}

	struct hive *hive = 0;
	const char *filename = argv[1];

	hive = open_hive(filename, HMODE_RO);
	if (!hive)
	{
		fprintf(stderr, "Failed to open hive\n");
		return 1;
	}

	QString kn( "\\" );
	RegistryItem *rootItem = new RegistryItem( hive, NULL, kn );

	RegistryItemModel model(rootItem, hive);

	QTreeView *keylist = new QTreeView;
	keylist->setModel( &model );
	keylist->setWindowTitle(QObject::tr("Registry editor"));

	QTreeView *valuelist = new QTreeView;

	QWidget *window = new QWidget;
	QHBoxLayout *layout = new QHBoxLayout;

	layout->addWidget( keylist );
	layout->addWidget( valuelist );

	window->setLayout( layout );
	window->show();

	return app.exec();
}
