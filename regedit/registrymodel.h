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

#ifndef __REGEDIT_REGISTRYMODEL_H__
#define __REGEDIT_REGISTRYMODEL_H__

#include <QAbstractItemModel>
#include <qstring.h>
#include "registryitem.h"
#include "ntreg.h"

class RegistryItemModel : public QAbstractItemModel
{
	Q_OBJECT
public:
	RegistryItemModel(RegistryItem* root, struct hive *h);
	~RegistryItemModel() {}
	virtual Qt::ItemFlags flags(const QModelIndex &index) const;
	virtual QModelIndex index(int,int,const QModelIndex&) const;
	virtual int rowCount(const QModelIndex& index) const;
	virtual int columnCount(const QModelIndex& index) const;
	virtual QVariant data(const QModelIndex&, int) const;
	virtual QModelIndex parent(const QModelIndex & index) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
private:
	struct hive *hive;
	RegistryItem *root;
};

#endif // __REGEDIT_REGISTRYMODEL_H__
