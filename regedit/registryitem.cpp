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
#include "registryitem.h"
#include "ntreg.h"

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

