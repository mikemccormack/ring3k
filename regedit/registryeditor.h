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

#ifndef __REGEDIT_REGISTRYEDITOR_H__
#define __REGEDIT_REGISTRYEDITOR_H__

#include <QApplication>
#include <QWidget>
#include <QList>
#include <QListView>
#include <QHBoxLayout>
#include "registryitem.h"
#include "registrymodel.h"
#include "registryvalue.h"
#include "registrytreeview.h"
#include "ntreg.h"

class RegistryEditor : public QWidget
{
	Q_OBJECT;
private:
	struct hive *hive;
	RegistryItem *rootItem;
	RegistryItemModel *keyModel;
	RegistryTreeView *keylist;
	RegistryValueModel *valueModel;
	QListView *valuelist;
	QHBoxLayout *layout;
public:
	RegistryEditor( struct hive * h );
protected slots:
	void key_changed(const QModelIndex& current, const QModelIndex& prev);
};

#endif // __REGEDIT_REGISTRYEDITOR_H__
