#include <QApplication>
#include <QTreeView>
#include <QAbstractItemModel>
#include <QList>
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
	RegistryItem( struct hive *h, RegistryItem* p, const char *n );
	QString* getPath();
	RegistryItem* getParent();
	RegistryItem* getChild( int n );
	int getSubkeyCount();
	const QString& getName() const;
protected:
	void enumerateChildren();
};

RegistryItem::RegistryItem( struct hive *h, RegistryItem* p, const char *n ) :
	hive( h ),
	parent( p ),
	name( n ),
	enumerated( false )
{
}

QString* RegistryItem::getPath()
{
	QString* path;
	if (parent)
	{
		path = parent->getPath();
		path->append( name );
		path->append( "\\" );
	}
	else
		path = new QString( name );
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
	char *path = getPath()->toUtf8().data();
	int i = 0;
	while (1)
	{
		int r = nk_get_subkey( hive, path, 0, i, &key );
		if (r < 0)
			break;
		fprintf(stderr, "enumerateChildren: %s\n", key.name);
		RegistryItem* item = new RegistryItem( hive, this, key.name );
		subkeys.append( item );
		i++;
	}
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

class RegistryItemModel : public QAbstractListModel
{
	struct hive *hive;
	RegistryItem *root;

public:
	RegistryItemModel(RegistryItem* root, struct hive *h);
	virtual QModelIndex index(int,int,const QModelIndex&) const;
	virtual int rowCount(const QModelIndex& index) const;
	virtual QVariant data(const QModelIndex&, int) const;
public:
	int enum_func( struct nk_key *key );
};

RegistryItemModel::RegistryItemModel(RegistryItem* r, struct hive *h) :
	hive( h ),
	root( r )
{
	fprintf(stderr, "RegistryItemModel::RegistryItemModel\n");
}

QModelIndex RegistryItemModel::index(int row, int column, const QModelIndex& parent) const
{
	fprintf(stderr, "RegistryItemModel::index "
		"%d, %d\n", row, column);

	RegistryItem* parentItem;
	if (!parent.isValid())
		parentItem = root;
	else
		parentItem = static_cast<RegistryItem*>( parent.internalPointer() );

	RegistryItem *child = parentItem->getChild( row );
	if (!child)
		return QModelIndex();

	return createIndex( row, column, child );
}

int RegistryItemModel::rowCount(const QModelIndex& parent) const
{
	if (parent.column()>0)
		return 0;
	fprintf(stderr, "RegistryItemModel::rowCount "
		"%d, %d\n", parent.row(), parent.column());

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
	fprintf(stderr, "RegistryItemModel::data "
		"%d, %d\n", index.row(), index.column());

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

	const char *filename = argv[1];

	struct hive *hive;
	hive = open_hive(filename, HMODE_RO);
	if (!hive)
	{
		fprintf(stderr, "Failed to open hive\n");
		return 1;
	}

	//nk_ls( hive, "\\", 0, 0);
	RegistryItem* rootItem = new RegistryItem( hive, NULL, "" );

	RegistryItemModel model(rootItem, hive);
	QTreeView keylist;
	keylist.resize(320, 200);
	keylist.setHeader( 0 );
	keylist.setModel( &model );
	keylist.show();

	return app.exec();
}
