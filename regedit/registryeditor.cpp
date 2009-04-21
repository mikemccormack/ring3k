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
#include "registryeditor.h"
#include "registrytreeview.h"
#include "ntreg.h"

void RegistryTreeView::currentChanged ( const QModelIndex & /*current*/, const QModelIndex & /*previous*/ )
{
	fprintf(stderr, "currentChanged\n");
	onSelectionChanged();
}

RegistryEditor::RegistryEditor( struct hive* h ) :
	hive( h )
{
	QString kn( "\\" );
	RegistryItem *rootItem = new RegistryItem( hive, NULL, kn );

	RegistryItemModel *keyModel = new RegistryItemModel( rootItem, hive );

	RegistryTreeView *keylist = new RegistryTreeView;
	keylist->setModel( keyModel );
	keylist->setWindowTitle(QObject::tr("Registry editor"));

	RegistryValueModel *valueModel = new RegistryValueModel;

	QListView *valuelist = new QListView;
	valuelist->setModel( valueModel );

	QHBoxLayout *layout = new QHBoxLayout;

	bool r = connect( keylist, SIGNAL(onSelectionChanged()), this, SLOT(key_changed()));
	if (!r)
		fprintf(stderr, "connect failed\n");

	layout->addWidget( keylist );
	layout->addWidget( valuelist );

	setLayout( layout );
}

void RegistryEditor::key_changed()
{
	fprintf(stderr,"key_changed\n");
}
