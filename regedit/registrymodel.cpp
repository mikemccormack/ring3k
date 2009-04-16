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

#include <QTreeView>
#include <QAbstractItemModel>
#include <QList>
#include <qstring.h>
#include "registryitem.h"
#include "registrymodel.h"
#include "ntreg.h"

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

QVariant RegistryItemModel::headerData(int /*section*/, Qt::Orientation orientation, int role) const
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
