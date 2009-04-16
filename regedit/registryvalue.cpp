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

RegistryValueModel::RegistryValueModel()
{
}

Qt::ItemFlags RegistryValueModel::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return 0;

	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant RegistryValueModel::headerData(int /*section*/, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
		return "values";
	return QVariant();
}

QModelIndex RegistryValueModel::index(int row, int column, const QModelIndex& parent) const
{
	if (!parent.isValid() && row == 0 && column == 0)
		return createIndex( row, column, NULL );
	return QModelIndex();
}

QModelIndex RegistryValueModel::parent(const QModelIndex &/*index*/) const
{
	return QModelIndex();
}

QVariant RegistryValueModel::data(const QModelIndex& index, int role) const
{
	if (index.column() != 0)
		return QVariant();
	if (role != Qt::DisplayRole)
		return QVariant();
	if (index.row() == 0)
		return "@";
	return QVariant();
}

int RegistryValueModel::columnCount(const QModelIndex& parent) const
{
	if (!parent.isValid())
		return 1;
	else
		return 0;
}

int RegistryValueModel::rowCount(const QModelIndex& parent) const
{
	if (!parent.isValid())
		return 1;
	else
		return 0;
}

