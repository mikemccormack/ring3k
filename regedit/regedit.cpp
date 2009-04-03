#include <QApplication>
#include <QTreeView>
#include <QAbstractItemModel>
#include <assert.h>
#include "ntreg.h"

class QRegistryItemModel : public QAbstractListModel
{
	struct hive *hive;
	const char* path;

public:
	QRegistryItemModel(const char* path, struct hive *h);
	virtual QModelIndex index(int,int,const QModelIndex&) const;
	virtual int rowCount(const QModelIndex& index) const;
	virtual QVariant data(const QModelIndex&, int) const;
public:
	int enum_func( struct nk_key *key );
};

QRegistryItemModel::QRegistryItemModel(const char* p, struct hive *h) :
	hive( h ),
	path( p )
{
	fprintf(stderr, "QRegistryItemModel::QRegistryItemModel\n");
}

QModelIndex QRegistryItemModel::index(int row, int column, const QModelIndex& parent) const
{
	fprintf(stderr, "QRegistryItemModel::index "
		"%d, %d\n", row, column);
	if (row >= 0 && column == 0)
		return createIndex( row, column );
	return QModelIndex();
}

int QRegistryItemModel::rowCount(const QModelIndex& index) const
{
	if (index.column()>0)
		return 0;
	fprintf(stderr, "QRegistryItemModel::rowCount "
		"%d, %d\n", index.row(), index.column());
	if (!index.isValid())
	{
		// the root item
		int count = nk_enumerate_subkeys(hive, path, 0, 0, 0);
		fprintf(stderr, "count = %d\n", count);
		assert( count >= 0 );
		return count;
	}
	return 0;
}

QVariant QRegistryItemModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid())
		return QVariant();
	if (role != Qt::DisplayRole)
		return QVariant();
	fprintf(stderr, "QRegistryItemModel::data "
		"%d, %d\n", index.row(), index.column());

	struct ex_data key;
	int r = nk_get_subkey(hive, path, 0, index.row(), &key);
	if (r >= 0)
		//return QVariant( key.nk->keyname );
		return QVariant( key.name );

	return QVariant();
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

	nk_ls( hive, "\\", 0, 0);

	QRegistryItemModel model("\\", hive);
	QTreeView keylist;
	keylist.resize(320, 200);
	keylist.setHeader( 0 );
	keylist.setModel( &model );
	keylist.show();

	return app.exec();
}
