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
#include <QListView>
#include <QAbstractItemModel>
#include <QList>
#include <QHBoxLayout>
#include <qstring.h>
#include <assert.h>
#include "registryitem.h"
#include "registrymodel.h"
#include "registryvalue.h"
#include "ntreg.h"

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

	RegistryItemModel *keyModel = new RegistryItemModel( rootItem, hive );

	QTreeView *keylist = new QTreeView;
	keylist->setModel( keyModel );
	keylist->setWindowTitle(QObject::tr("Registry editor"));

	RegistryValueModel *valueModel = new RegistryValueModel;

	QListView *valuelist = new QListView;
	valuelist->setModel( valueModel );

	QWidget *window = new QWidget;
	QHBoxLayout *layout = new QHBoxLayout;

	layout->addWidget( keylist );
	layout->addWidget( valuelist );

	window->setLayout( layout );
	window->show();

	return app.exec();
}
