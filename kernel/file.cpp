/*
 * nt loader
 *
 * Copyright 2006-2008 Mike McCormack
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


#include <unistd.h>

#include <stdio.h>
#include <stdarg.h>
#include <fcntl.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <linux/types.h>
#include <linux/unistd.h>
#include <sys/syscall.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "winioctl.h"

#include "debug.h"
#include "object.h"
#include "object.inl"
#include "mem.h"
#include "ntcall.h"
#include "file.h"
#include "symlink.h"

// FIXME: use unicode tables
WCHAR lowercase(const WCHAR ch)
{
	if (ch >= 'A' && ch <='Z')
		return ch | 0x20;
	return ch;
}

io_object_t::io_object_t() :
	completion_port( 0 ),
	completion_key( 0 )
{
}

void io_object_t::set_completion_port( completion_port_t *port, ULONG key )
{
	if (completion_port)
	{
		release( completion_port );
		completion_port = 0;
	}
	completion_port = port;
	completion_key = 0;
}

NTSTATUS io_object_t::set_position( LARGE_INTEGER& ofs )
{
	return STATUS_OBJECT_TYPE_MISMATCH;
}

NTSTATUS io_object_t::fs_control( event_t* event, IO_STATUS_BLOCK iosb, ULONG FsControlCode,
		 PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength )
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS io_object_t::set_pipe_info( FILE_PIPE_INFORMATION& pipe_info )
{
	return STATUS_OBJECT_TYPE_MISMATCH;
}

file_t::~file_t()
{
	close( fd );
}

file_t::file_t( int f ) :
	fd( f )
{
}

class file_create_info_t : public open_info_t
{
public:
	ULONG FileAttributes;
	ULONG CreateOptions;
	ULONG CreateDisposition;
	bool created;
public:
	file_create_info_t( ULONG _Attributes, ULONG _CreateOptions, ULONG _CreateDisposition );
	virtual NTSTATUS on_open( object_dir_t* dir, object_t*& obj, open_info_t& info );
};

file_create_info_t::file_create_info_t( ULONG _Attributes, ULONG _CreateOptions, ULONG _CreateDisposition ) :
	FileAttributes( _Attributes ),
	CreateOptions( _CreateOptions ),
	CreateDisposition( _CreateDisposition ),
	created( false )
{
}

NTSTATUS file_create_info_t::on_open( object_dir_t* dir, object_t*& obj, open_info_t& info )
{
	trace("file_create_info_t::on_open()\n");
	if (!obj)
		return STATUS_OBJECT_NAME_NOT_FOUND;
	return STATUS_SUCCESS;
}

NTSTATUS file_t::read( PVOID Buffer, ULONG Length, ULONG *bytes_read )
{
	NTSTATUS r = STATUS_SUCCESS;
	ULONG ofs = 0;
	while (ofs < Length)
	{
		BYTE *p = (BYTE*)Buffer+ofs;
		size_t len = Length - ofs;

		r = current->process->vm->get_kernel_address( &p, &len );
		if (r < STATUS_SUCCESS)
			break;

		int ret = ::read( fd, p, len );
		if (ret < 0)
		{
			r = STATUS_IO_DEVICE_ERROR;
			break;
		}

		ofs += len;
	}

	*bytes_read = ofs;

	return r;
}

NTSTATUS file_t::write( PVOID Buffer, ULONG Length, ULONG *written )
{
	NTSTATUS r = STATUS_SUCCESS;
	ULONG ofs = 0;
	while (ofs< Length)
	{
		BYTE *p = (BYTE*)Buffer+ofs;
		size_t len = Length - ofs;

		NTSTATUS r = current->process->vm->get_kernel_address( &p, &len );
		if (r < STATUS_SUCCESS)
			break;

		int ret = ::write( fd, p, len );
		if (ret < 0)
		{
			r = STATUS_IO_DEVICE_ERROR;
			break;
		}

		ofs += len;
	}

	*written = ofs;

	return r;
}

NTSTATUS file_t::set_position( LARGE_INTEGER& ofs )
{
	int ret;

	ret = lseek( fd, ofs.QuadPart, SEEK_SET );
	if (ret < 0)
	{
		trace("seek failed\n");
		return STATUS_UNSUCCESSFUL;
	}
	return STATUS_SUCCESS;
}

NTSTATUS file_t::remove()
{
	char name[40];
	char path[255];
	int r;

	// get the file's name
	sprintf( name, "/proc/self/fd/%d", get_fd() );
	r = readlink( name, path, sizeof path - 1 );
	if (r < 0)
		return STATUS_ACCESS_DENIED;
	path[r] = 0;

	// remove it
	if (0 > unlink( path ) &&
		0 > rmdir( path ))
	{
		fprintf(stderr, "Failed to delete %s\n", path);
		// FIXME: check errno
		return STATUS_ACCESS_DENIED;
	}

	return STATUS_SUCCESS;
}

class directory_entry_t;

typedef list_anchor<directory_entry_t,0> dirlist_t;
typedef list_iter<directory_entry_t,0> dirlist_iter_t;
typedef list_element<directory_entry_t> dirlist_element_t;

class directory_entry_t {
public:
	dirlist_element_t entry[1];
	unicode_string_t name;
	struct stat st;
};

class directory_t : public file_t
{
	int count;
	directory_entry_t *ptr;
	dirlist_t entries;
	unicode_string_t mask;
protected:
	void reset();
	void add_entry(const char *name);
	int open_unicode_file( const char *unix_path, int flags, bool& created );
	int open_unicode_dir( const char *unix_path, int flags, bool& created );
public:
	directory_t( int fd );
	~directory_t();
	NTSTATUS query_directory_file();
	NTSTATUS read( PVOID Buffer, ULONG Length, ULONG *bytes_read );
	NTSTATUS write( PVOID Buffer, ULONG Length, ULONG *bytes_read );
	virtual NTSTATUS query_information( FILE_ATTRIBUTE_TAG_INFORMATION& info );
	directory_entry_t* get_next();
	bool match(unicode_string_t &name) const;
	void scandir();
	bool is_firstscan() const;
	NTSTATUS set_mask(unicode_string_t *mask);
	int get_num_entries() const;
	virtual NTSTATUS open( object_t *&out, open_info_t& info );
	NTSTATUS open_file( file_t *&file, UNICODE_STRING& path, ULONG Attributes,
		ULONG Options, ULONG CreateDisposition, bool &created, bool case_insensitive );
};

class directory_factory : public object_factory
{
	int fd;
public:
	directory_factory( int _fd );
	NTSTATUS alloc_object(object_t** obj);
};

directory_factory::directory_factory( int _fd ) :
	fd( _fd )
{
}

NTSTATUS directory_factory::alloc_object(object_t** obj)
{
	*obj = new directory_t( fd );
	if (!*obj)
		return STATUS_NO_MEMORY;
	return STATUS_SUCCESS;
}


directory_t::directory_t( int fd ) :
	file_t(fd),
	count(-1),
	ptr(0)
{
}

directory_t::~directory_t()
{
}

NTSTATUS directory_t::read( PVOID Buffer, ULONG Length, ULONG *bytes_read )
{
	return STATUS_OBJECT_TYPE_MISMATCH;
}

NTSTATUS directory_t::write( PVOID Buffer, ULONG Length, ULONG *bytes_read )
{
	return STATUS_OBJECT_TYPE_MISMATCH;
}

// getdents64 borrowed from wine/dlls/ntdll/directory.c

typedef struct
{
    ULONG64        d_ino;
    LONG64         d_off;
    unsigned short d_reclen;
    unsigned char  d_type;
    char           d_name[256];
} KERNEL_DIRENT64;

int getdents64( int fd, unsigned char *de, unsigned int size )
{
    int ret;
    __asm__( "pushl %%ebx; movl %2,%%ebx; int $0x80; popl %%ebx"
             : "=a" (ret)
             : "0" (220 /*NR_getdents64*/), "r" (fd), "c" (de), "d" (size)
             : "memory" );
    if (ret < 0)
    {
        errno = -ret;
        ret = -1;
    }
    return ret;
}

#ifndef HAVE_FSTATAT
int fstatat64( int dirfd, const char *path, struct stat64 *buf, int flags )
{
	int ret;

	__asm__(
		"pushl %%ebx\n"
		"\tmovl %2,%%ebx\n"
		"\tint $0x80\n"
		"\tpopl %%ebx\n"
		: "=a"(ret)
		: "0"(300 /*NR_fstatat64*/), "r"( dirfd ), "c"( path ), "d"( buf ), "S"(flags)
		: "memory" );
	if (ret < 0)
	{
		errno = -ret;
		ret = -1;
	}
	return ret;
}

int fstatat( int dirfd, const char *path, struct stat *buf, int flags )
{
	struct stat64 st;
	int ret;

	ret = fstatat64( dirfd, path, &st, flags );
	if (ret >= 0)
	{
		buf->st_dev = st.st_dev;
		buf->st_ino = st.st_ino;
		buf->st_mode = st.st_mode;
		buf->st_nlink = st.st_nlink;
		buf->st_uid = st.st_uid;
		buf->st_gid = st.st_gid;
		buf->st_rdev = st.st_rdev;
		if (st.st_size < 0x100000000LL)
			buf->st_size = st.st_size;
		else
			buf->st_size = ~0;
		buf->st_blksize = st.st_blksize;
		buf->st_blocks = st.st_blocks;
		buf->st_atime = st.st_atime;
		buf->st_mtime = st.st_mtime;
		buf->st_ctime = st.st_ctime;
	}
	return ret;
}

#endif

void directory_t::reset()
{
	ptr = 0;
	count = 0;

	while (!entries.empty())
	{
		directory_entry_t *x = entries.head();
		entries.unlink(x);
		delete x;
	}
}

bool directory_t::match(unicode_string_t &name) const
{
	if (mask.Length == 0)
		return true;

	// check for dot pseudo files
	bool pseudo_file = (name.Length == 2 && name.Buffer[0] == '.') ||
		(name.Length == 4 && name.Buffer[0] == '.' && name.Buffer[1] == '.' );

	int i = 0, j = 0;
	while (i < mask.Length/2 && j < name.Length/2)
	{
		// asterisk matches everything
		if (mask.Buffer[i] == '*')
			return true;

		// question mark matches one character
		if (mask.Buffer[i] == '?')
		{
			if (pseudo_file)
				return false;
			i++;
			j++;
			continue;
		}

		// double quote matches separator
		if (mask.Buffer[i] == '"' && j != 0 && name.Buffer[j] == '.')
		{
			i++;
			j++;
			continue;
		}

		// right angle matches anything except a dot
		if (mask.Buffer[i] == '>' && name.Buffer[j] != '.')
		{
			i++;
			j++;
			continue;
		}

		// right angle matches strings without a dot
		if (mask.Buffer[i] == '<')
		{
			while (name.Buffer[j] != '.' && j < name.Length)
				j++;
			i++;
			continue;
		}

		if (pseudo_file)
			return false;

		// match characters
		//trace("%c <> %c\n", mask.Buffer[i], name.Buffer[j]);
		if (lowercase(mask.Buffer[i]) != lowercase(name.Buffer[j]))
			return false;

		i++;
		j++;
	}

	// left over characters are a mismatch
	if (i != mask.Length/2 || j != name.Length/2)
		return false;

	return true;
}

void directory_t::add_entry(const char *name)
{
	trace("adding dir entry: %s\n", name);
	directory_entry_t *ent = new directory_entry_t;
	ent->name.copy(name);
	/* FIXME: Should symlinks be deferenced?
	   AT_SYMLINK_NOFOLLOW */
	if (0 != fstatat(get_fd(), name, &ent->st, 0))
	{
		delete ent;
		return;
	}
	if (!match(ent->name))
	{
		delete ent;
		return;
	}
	trace("matched mask %pus\n", &mask);
	//trace("mode = %o\n", ent->st.st_mode);
	entries.append(ent);
	count++;
}

int directory_t::get_num_entries() const
{
	return count;
}

void directory_t::scandir()
{
	unsigned char buffer[0x1000];
	int r;

	reset();
	r = lseek( get_fd(), 0, SEEK_SET );
	if (r == -1)
	{
		trace("lseek failed (%d)\n", errno);
		return;
	}

	trace("reading entries:\n");
	// . and .. always come first
	add_entry(".");
	add_entry("..");

	do
	{
		r = ::getdents64( get_fd(), buffer, sizeof buffer );
		if (r < 0)
		{
			trace("getdents64 failed (%d)\n", r);
			break;
		}

		int ofs = 0;
		while (ofs<r)
		{
			KERNEL_DIRENT64* de = (KERNEL_DIRENT64*) &buffer[ofs];
			//fprintf(stderr, "%ld %d %s\n", de->d_off, de->d_reclen, de->d_name);
			if (de->d_off <= 0)
				break;
			if (de->d_reclen <=0 )
				break;
			ofs += de->d_reclen;
			if (!strcmp(de->d_name,".") || !strcmp(de->d_name, ".."))
				continue;
			add_entry(de->d_name);
		}
	} while (0);
}

NTSTATUS directory_t::set_mask(unicode_string_t *string)
{
	mask.copy(string);
	return STATUS_SUCCESS;
}

// scan for the first time after construction
bool directory_t::is_firstscan() const
{
	return (count == -1);
}

directory_entry_t* directory_t::get_next()
{
	if (!ptr)
	{
		ptr = entries.head();
		return ptr;
	}

	if (ptr == entries.tail())
		return 0;

	ptr = ptr->entry[0].get_next();

	return ptr;
}

NTSTATUS directory_t::query_information( FILE_ATTRIBUTE_TAG_INFORMATION& info )
{
	info.FileAttributes = FILE_ATTRIBUTE_DIRECTORY;
	info.ReparseTag = 0;
	return STATUS_SUCCESS;
}

int file_t::get_fd()
{
	return fd;
}

NTSTATUS file_t::query_information( FILE_BASIC_INFORMATION& info )
{
	info.FileAttributes = FILE_ATTRIBUTE_ARCHIVE;
	return STATUS_SUCCESS;
}

NTSTATUS file_t::query_information( FILE_STANDARD_INFORMATION& info )
{
	struct stat st;
	if (0<fstat( fd, &st ))
		return STATUS_UNSUCCESSFUL;
	info.EndOfFile.QuadPart = st.st_size;
	info.AllocationSize.QuadPart = (st.st_size+0x1ff)&~0x1ff;
	if (S_ISDIR(st.st_mode))
		info.Directory = TRUE;
	else
		info.Directory = FALSE;
	return STATUS_SUCCESS;
}

NTSTATUS file_t::query_information( FILE_ATTRIBUTE_TAG_INFORMATION& info )
{
	info.FileAttributes = FILE_ATTRIBUTE_ARCHIVE;
	info.ReparseTag = 0;
	return STATUS_SUCCESS;
}

char *build_path( int fd, const UNICODE_STRING *us )
{
	char *str, *p;
	int i;
	int len = us->Length/2 + 1;
	const char fd_prefix[] = "/proc/self/fd/%d/";

	if (fd >= 0)
		len += sizeof fd_prefix + 10;

	str = new char[ len ];
	if (!str)
		return str;

	str[0] = 0;
	if (fd >= 0)
		sprintf( str, fd_prefix, fd );

	p = &str[strlen( str )];
	for (i=0; i<us->Length/2; i++)
		*p++ = us->Buffer[i];
	*p = 0;

	return str;
}

char *get_unix_path( int fd, UNICODE_STRING& str, bool case_insensitive )
{
	char *file;
	int i;

	file = build_path( fd, &str );
	if (!file)
		return NULL;

	for (i=0; file[i]; i++)
	{
		if (file[i] == '\\')
		{
			file[i] = '/';
			continue;
		}

		// make filename lower case if necessary
		if (!case_insensitive)
			continue;
		file[i] = lowercase(file[i]);
	}

	return file;
}

int directory_t::open_unicode_file( const char *unix_path, int flags, bool &created )
{
	int r = -1;

	trace("open file : %s\n", unix_path);
	r = ::open( unix_path, flags&~O_CREAT );
	if (r < 0 && (flags & O_CREAT))
	{
		trace("create file : %s\n", unix_path);
		r = ::open( unix_path, flags, 0666 );
		if (r >= 0)
			created = true;
	}
	return r;
}

int directory_t::open_unicode_dir( const char *unix_path, int flags, bool &created )
{
	int r = -1;

	if (flags & O_CREAT)
	{
		trace("create dir : %s\n", unix_path);
		r = ::mkdir( unix_path, 0777 );
		if (r == 0)
			created = true;
	}
	trace("open name : %s\n", unix_path);
	r = ::open( unix_path, flags & ~O_CREAT );
	trace("r = %d\n", r);
	return r;
}

NTSTATUS directory_t::open_file(
	file_t *&file,
	UNICODE_STRING& path,
	ULONG Attributes,
	ULONG Options,
	ULONG CreateDisposition,
	bool &created,
	bool case_insensitive )
{
	int file_fd;

	trace("name = %pus\n", &path );

	int mode = O_RDONLY;
	switch (CreateDisposition)
	{
	case FILE_OPEN:
		mode = O_RDONLY;
		break;
	case FILE_CREATE:
		mode = O_CREAT;
		break;
	case FILE_OPEN_IF:
		mode = O_CREAT;
		break;
	default:
		trace("CreateDisposition = %ld\n", CreateDisposition);
		return STATUS_NOT_IMPLEMENTED;
	}

	char *unix_path = get_unix_path( get_fd(), path, case_insensitive );
	if (!unix_path)
		return STATUS_OBJECT_PATH_NOT_FOUND;

	if (Options & FILE_DIRECTORY_FILE)
	{
		file_fd = open_unicode_dir( unix_path, mode, created );
		delete[] unix_path;
		if (file_fd == -1)
			return STATUS_OBJECT_PATH_NOT_FOUND;

		trace("file_fd = %d\n", file_fd );
		file = new directory_t( file_fd );
		if (!file)
		{
			::close( file_fd );
			return STATUS_NO_MEMORY;
		}
	}
	else
	{
		file_fd = open_unicode_file( unix_path, mode, created );
		delete[] unix_path;
		if (file_fd == -1)
			return STATUS_OBJECT_PATH_NOT_FOUND;

		file = new file_t( file_fd );
		if (!file)
		{
			::close( file_fd );
			return STATUS_NO_MEMORY;
		}
	}

	return STATUS_SUCCESS;
}

NTSTATUS directory_t::open( object_t *&out, open_info_t& info )
{
	file_t *file = 0;

	trace("directory_t::open %pus\n", &info.path );

	file_create_info_t *file_info = dynamic_cast<file_create_info_t*>( &info );
	if (!file_info)
		return STATUS_OBJECT_TYPE_MISMATCH;

	NTSTATUS r = open_file( file, info.path, file_info->Attributes, file_info->CreateOptions,
		file_info->CreateDisposition, file_info->created, info.case_insensitive() );
	if (r < STATUS_SUCCESS)
		return r;
	out = file;
	return r;
}

NTSTATUS open_file( file_t *&file, UNICODE_STRING& name )
{
	file_create_info_t info( 0, 0, FILE_OPEN );
	info.path.set( name );
	info.Attributes = OBJ_CASE_INSENSITIVE;
	object_t *obj = 0;
	NTSTATUS r = open_root( obj, info );
	if (r < STATUS_SUCCESS)
		return r;
	file = dynamic_cast<file_t*>( obj );
	assert( file != NULL );
	return STATUS_SUCCESS;
}

void init_drives()
{
	int fd = open( "drive", O_RDONLY );
	if (fd < 0)
		die("drive does not exist");
	directory_factory factory( fd );
	unicode_string_t dirname;
	dirname.copy( L"\\Device\\HarddiskVolume1" );
	object_t *obj = 0;
	NTSTATUS r;
	r = factory.create_kernel( obj, dirname );
	if (r < STATUS_SUCCESS)
	{
		trace( "failed to create %pus\n", &dirname);
		die("fatal\n");
	}

	unicode_string_t c_link;
	c_link.set( L"\\??\\c:" );
	r = create_symlink( c_link, dirname );
	if (r < STATUS_SUCCESS)
	{
		trace( "failed to create symlink %pus (%08lx)\n", &c_link, r);
		die("fatal\n");
	}
}

NTSTATUS NTAPI NtCreateFile(
	PHANDLE FileHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	PIO_STATUS_BLOCK IoStatusBlock,
	PLARGE_INTEGER AllocationSize,
	ULONG Attributes,
	ULONG ShareAccess,
	ULONG CreateDisposition,
	ULONG CreateOptions,
	PVOID EaBuffer,
	ULONG EaLength )
{
	object_attributes_t oa;
	IO_STATUS_BLOCK iosb;
	NTSTATUS r;

	r = oa.copy_from_user( ObjectAttributes );
	if (r < STATUS_SUCCESS)
		return r;

	trace("root %p attr %08lx %pus\n",
			oa.RootDirectory, oa.Attributes, oa.ObjectName);

	r = verify_for_write( IoStatusBlock, sizeof *IoStatusBlock );
	if (r < STATUS_SUCCESS)
		return r;

	r = verify_for_write( FileHandle, sizeof *FileHandle );
	if (r < STATUS_SUCCESS)
		return r;

	if (!(CreateOptions & FILE_DIRECTORY_FILE))
		Attributes &= ~FILE_ATTRIBUTE_DIRECTORY;

	if (!oa.ObjectName)
		return STATUS_OBJECT_PATH_NOT_FOUND;

	file_create_info_t info( Attributes, CreateOptions, CreateDisposition );

	info.path.set( *oa.ObjectName );
	info.Attributes = oa.Attributes;

	object_t *obj = 0;
	r = open_root( obj, info );
	if (r >= STATUS_SUCCESS)
	{
		r = alloc_user_handle( obj, DesiredAccess, FileHandle );
		release( obj );
	}

	iosb.Status = r;
	iosb.Information = info.created ? FILE_CREATED : FILE_OPENED;

	copy_to_user( IoStatusBlock, &iosb, sizeof iosb );

	return r;
}

NTSTATUS NTAPI NtOpenFile(
	PHANDLE FileHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	PIO_STATUS_BLOCK IoStatusBlock,
	ULONG ShareAccess,
	ULONG OpenOptions )
{
	return NtCreateFile( FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock,
				0, 0, ShareAccess, FILE_OPEN, OpenOptions, 0, 0 );
}

NTSTATUS NTAPI NtFsControlFile(
	HANDLE FileHandle,
	HANDLE EventHandle,
	PIO_APC_ROUTINE ApcRoutine,
	PVOID ApcContext,
	PIO_STATUS_BLOCK IoStatusBlock,
	ULONG FsControlCode,
	PVOID InputBuffer,
	ULONG InputBufferLength,
	PVOID OutputBuffer,
	ULONG OutputBufferLength )
{
	trace("%p %p %p %p %p %08lx %p %lu %p %lu\n", FileHandle,
			EventHandle, ApcRoutine, ApcContext, IoStatusBlock, FsControlCode,
			InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength );

	IO_STATUS_BLOCK iosb;
	io_object_t *io = 0;
	event_t *event = 0;
	NTSTATUS r;

	r = object_from_handle( io, FileHandle, 0 );
	if (r < STATUS_SUCCESS)
		return r;

	r = verify_for_write( IoStatusBlock, sizeof *IoStatusBlock );
	if (r < STATUS_SUCCESS)
		return r;

#if 0
	if (EventHandle)
	{
		r = object_from_handle( event, EventHandle, SYNCHRONIZE );
		if (r < STATUS_SUCCESS)
			return r;
	}
#endif

	r = io->fs_control( event, iosb, FsControlCode,
		 InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength );

	iosb.Status = r;

	copy_to_user( IoStatusBlock, &iosb, sizeof iosb );

	return r;
}

NTSTATUS NTAPI NtDeviceIoControlFile(
	HANDLE File,
	HANDLE Event,
	PIO_APC_ROUTINE ApcRoutine,
	PVOID ApcContext,
	PIO_STATUS_BLOCK IoStatusBlock,
	ULONG IoControlCode,
	PVOID InputBuffer,
	ULONG InputBufferLength,
	PVOID OutputBuffer,
	ULONG OutputBufferLength )
{
	trace("%p %p %p %p %p %08lx %p %lu %p %lu\n",
			File, Event, ApcRoutine, ApcContext, IoStatusBlock, IoControlCode,
			InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength );
	return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtWriteFile(
	HANDLE FileHandle,
	HANDLE Event,
	PIO_APC_ROUTINE ApcRoutine,
	PVOID ApcContext,
	PIO_STATUS_BLOCK IoStatusBlock,
	PVOID Buffer,
	ULONG Length,
	PLARGE_INTEGER ByteOffset,
	PULONG Key )
{

	trace("%p %p %p %p %p %p %lu %p %p\n", FileHandle, Event, ApcRoutine,
			ApcContext, IoStatusBlock, Buffer, Length, ByteOffset, Key);

	io_object_t *io = 0;
	NTSTATUS r;

	r = object_from_handle( io, FileHandle, 0 );
	if (r < STATUS_SUCCESS)
		return r;

	r = verify_for_write( IoStatusBlock, sizeof *IoStatusBlock );
	if (r < STATUS_SUCCESS)
		return r;

	ULONG ofs = 0;
	r = io->write( Buffer, Length, &ofs );
	if (r < STATUS_SUCCESS)
		return r;

	IO_STATUS_BLOCK iosb;
	iosb.Status = r;
	iosb.Information = ofs;

	copy_to_user( IoStatusBlock, &iosb, sizeof iosb );

	return r;
}

NTSTATUS NTAPI NtQueryAttributesFile(
	POBJECT_ATTRIBUTES ObjectAttributes,
	PFILE_BASIC_INFORMATION FileInformation )
{
	object_attributes_t oa;
	NTSTATUS r;
	FILE_BASIC_INFORMATION info;

	trace("%p %p\n", ObjectAttributes, FileInformation);

	r = oa.copy_from_user( ObjectAttributes );
	if (r)
		return r;

	trace("root %p attr %08lx %pus\n",
			oa.RootDirectory, oa.Attributes, oa.ObjectName);

	if (!oa.ObjectName || !oa.ObjectName->Buffer)
		return STATUS_INVALID_PARAMETER;

	// FIXME: use oa.RootDirectory
	object_t *obj = 0;
	file_create_info_t open_info( 0, 0, FILE_OPEN );
	open_info.path.set( *oa.ObjectName );
	open_info.Attributes = oa.Attributes;
	r = open_root( obj, open_info );
	if (r < STATUS_SUCCESS)
		return r;

	file_t *file = dynamic_cast<file_t*>(obj );
	if (file)
	{

		memset( &info, 0, sizeof info );
		r = file->query_information( info );
	}
	else
		r = STATUS_OBJECT_TYPE_MISMATCH;
	release( obj );

	return r;
}

NTSTATUS NTAPI NtQueryVolumeInformationFile(
	HANDLE FileHandle,
	PIO_STATUS_BLOCK IoStatusBlock,
	PVOID VolumeInformation,
	ULONG VolumeInformationLength,
	FS_INFORMATION_CLASS VolumeInformationClass )
{
	trace("%p %p %p %lu %u\n", FileHandle, IoStatusBlock, VolumeInformation,
			VolumeInformationLength, VolumeInformationClass );
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI NtReadFile(
	HANDLE FileHandle,
	HANDLE EventHandle,
	PIO_APC_ROUTINE ApcRoutine,
	PVOID ApcContext,
	PIO_STATUS_BLOCK IoStatusBlock,
	PVOID Buffer,
	ULONG Length,
	PLARGE_INTEGER ByteOffset,
	PULONG Key)
{
	trace("%p %p %p %p %p %p %lu %p %p\n", FileHandle, EventHandle,
			ApcRoutine, ApcContext, IoStatusBlock,
			Buffer, Length, ByteOffset, Key);

	NTSTATUS r;
	io_object_t *io = 0;

	r = object_from_handle( io, FileHandle, GENERIC_READ );
	if (r < STATUS_SUCCESS)
		return r;

	r = verify_for_write( IoStatusBlock, sizeof *IoStatusBlock );
	if (r < STATUS_SUCCESS)
		return r;

	ULONG ofs = 0;
	r = io->read( Buffer, Length, &ofs );
	if (r < STATUS_SUCCESS)
		return r;

	IO_STATUS_BLOCK iosb;
	iosb.Status = r;
	iosb.Information = ofs;

	r = copy_to_user( IoStatusBlock, &iosb, sizeof iosb );

	return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtDeleteFile(
	POBJECT_ATTRIBUTES ObjectAttributes)
{
	object_attributes_t oa;
	NTSTATUS r;

	trace("%p\n", ObjectAttributes);

	r = oa.copy_from_user( ObjectAttributes );
	if (r < STATUS_SUCCESS)
		return r;

	trace("root %p attr %08lx %pus\n",
			oa.RootDirectory, oa.Attributes, oa.ObjectName);

	if (!oa.ObjectName || !oa.ObjectName->Buffer)
		return STATUS_INVALID_PARAMETER;

	// FIXME: use oa.RootDirectory
	object_t *obj = 0;
	file_create_info_t open_info( 0, 0, FILE_OPEN );
	open_info.path.set( *oa.ObjectName );
	open_info.Attributes = oa.Attributes;
	r = open_root( obj, open_info );
	if (r < STATUS_SUCCESS)
		return r;

	file_t *file = dynamic_cast<file_t*>(obj );
	if (file)
		r = file->remove();
	else
		r = STATUS_OBJECT_TYPE_MISMATCH;
	release( obj );
	return r;
}

NTSTATUS NTAPI NtFlushBuffersFile(
	HANDLE FileHandle,
	PIO_STATUS_BLOCK IoStatusBlock)
{
	trace("%p %p\n", FileHandle, IoStatusBlock);
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI NtCancelIoFile(
	HANDLE FileHandle,
	PIO_STATUS_BLOCK IoStatusBlock)
{
	trace("%p %p\n", FileHandle, IoStatusBlock);
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI NtSetInformationFile(
	HANDLE FileHandle,
	PIO_STATUS_BLOCK IoStatusBlock,
	PVOID FileInformation,
	ULONG FileInformationLength,
	FILE_INFORMATION_CLASS FileInformationClass)
{
	io_object_t *file = 0;
	NTSTATUS r;
	ULONG len = 0;
	union {
		FILE_DISPOSITION_INFORMATION dispos;
		FILE_COMPLETION_INFORMATION completion;
		FILE_POSITION_INFORMATION position;
		FILE_PIPE_INFORMATION pipe;
	} info;

	r = object_from_handle( file, FileHandle, 0 );
	if (r < STATUS_SUCCESS)
		return r;

	switch (FileInformationClass)
	{
	case FileDispositionInformation:
		len = sizeof info.dispos;
		break;
	case FileCompletionInformation:
		len = sizeof info.completion;
		break;
	case FilePositionInformation:
		len = sizeof info.position;
		break;
	case FilePipeInformation:
		len = sizeof info.pipe;
		break;
	default:
		trace("Unknown information class %d\n", FileInformationClass );
		return STATUS_INVALID_PARAMETER;
	}

	r = copy_from_user( &info, FileInformation, len );
	if (r < STATUS_SUCCESS)
		return r;

	completion_port_t *completion_port = 0;

	switch (FileInformationClass)
	{
	case FileDispositionInformation:
		trace("delete = %d\n", info.dispos.DeleteFile);
		break;
	case FileCompletionInformation:
		r = object_from_handle( completion_port, info.completion.CompletionPort, IO_COMPLETION_MODIFY_STATE );
		if (r < STATUS_SUCCESS)
			return r;
		file->set_completion_port( completion_port, info.completion.CompletionKey );
		break;
	case FilePositionInformation:
		r = file->set_position( info.position.CurrentByteOffset );
		break;
	case FilePipeInformation:
		r = file->set_pipe_info( info.pipe );
		break;
	default:
		break;
	}

	return r;
}

NTSTATUS NTAPI NtQueryInformationFile(
	HANDLE FileHandle,
	PIO_STATUS_BLOCK IoStatusBlock,
	PVOID FileInformation,
	ULONG FileInformationLength,
	FILE_INFORMATION_CLASS FileInformationClass)
{
	trace("%p %p %p %lu %u\n", FileHandle, IoStatusBlock,
			FileInformation, FileInformationLength, FileInformationClass);

	file_t *file = 0;
	NTSTATUS r;

	r = object_from_handle( file, FileHandle, 0 );
	if (r < STATUS_SUCCESS)
		return r;

	union {
		FILE_BASIC_INFORMATION basic_info;
		FILE_STANDARD_INFORMATION std_info;
		FILE_ATTRIBUTE_TAG_INFORMATION attrib_info;
	} info;
	ULONG len;
	memset( &info, 0, sizeof info );

	switch (FileInformationClass)
	{
	case FileBasicInformation:
		len = sizeof info.basic_info;
		r = file->query_information( info.basic_info );
		break;
	case FileStandardInformation:
		len = sizeof info.std_info;
		r = file->query_information( info.std_info );
		break;
	case FileAttributeTagInformation:
		len = sizeof info.attrib_info;
		r = file->query_information( info.attrib_info );
		break;
	default:
		trace("Unknown information class %d\n", FileInformationClass );
		r = STATUS_INVALID_PARAMETER;
	}

	if (r < STATUS_SUCCESS)
		return r;

	if (len > FileInformationLength)
		 len = FileInformationLength;

	return copy_to_user( FileInformation, &info, len );
}

NTSTATUS NTAPI NtSetQuotaInformationFile(
	HANDLE FileHandle,
	PIO_STATUS_BLOCK IoStatusBlock,
	PFILE_USER_QUOTA_INFORMATION FileInformation,
	ULONG FileInformationLength)
{
	trace("%p %p %p %lu\n", FileHandle, IoStatusBlock,
			FileInformation, FileInformationLength);

	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI NtQueryQuotaInformationFile(HANDLE,PIO_STATUS_BLOCK,PFILE_USER_QUOTA_INFORMATION,ULONG,BOOLEAN,PFILE_QUOTA_LIST_INFORMATION,ULONG,PSID,BOOLEAN)
{
	trace("\n");
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI NtLockFile(
	HANDLE FileHandle,
	HANDLE EventHandle,
	PIO_APC_ROUTINE ApcRoutine,
	PVOID ApcContext,
	PIO_STATUS_BLOCK IoStatusBlock,
	PULARGE_INTEGER LockOffset,
	PULARGE_INTEGER LockLength,
	ULONG Key,
	BOOLEAN FailImmediately,
	BOOLEAN ExclusiveLock)
{
	trace("just returns success...\n");
	return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtUnlockFile(
	HANDLE FileHandle,
	PIO_STATUS_BLOCK IoStatusBlock,
	PULARGE_INTEGER LockOffset,
	PULARGE_INTEGER LockLength,
	ULONG Key)
{
	trace("just returns success...\n");
	return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtQueryDirectoryFile(
	HANDLE DirectoryHandle,
	HANDLE Event,
	PIO_APC_ROUTINE ApcRoutine,
	PVOID ApcContext,
	PIO_STATUS_BLOCK IoStatusBlock,
	PVOID FileInformation,
	ULONG FileInformationLength,
	FILE_INFORMATION_CLASS FileInformationClass,
	BOOLEAN ReturnSingleEntry,
	PUNICODE_STRING FileName,
	BOOLEAN RestartScan)
{
	NTSTATUS r;

	directory_t *dir = 0;
	r = object_from_handle( dir, DirectoryHandle, 0 );
	if (r < STATUS_SUCCESS)
		return r;

	// default (empty) mask matches all...
	unicode_string_t mask;
	if (FileName)
	{
		r = mask.copy_from_user( FileName );
		if (r < STATUS_SUCCESS)
			return r;

		trace("Filename = %pus (len=%d)\n", &mask, mask.Length);
	}

	if (FileInformationClass != FileBothDirectoryInformation)
	{
		trace("unimplemented FileInformationClass %d\n", FileInformationClass);
		return STATUS_NOT_IMPLEMENTED;
	}

	if (dir->is_firstscan())
	{
		r = dir->set_mask(&mask);
		if (r < STATUS_SUCCESS)
			return r;
	}

	if (dir->is_firstscan() || RestartScan)
		dir->scandir();

	if (dir->get_num_entries() == 0)
		return STATUS_NO_SUCH_FILE;

	directory_entry_t *de = dir->get_next();
	if (!de)
		return STATUS_NO_MORE_FILES;

	FILE_BOTH_DIRECTORY_INFORMATION info;
	memset( &info, 0, sizeof info );

	if (S_ISDIR(de->st.st_mode))
		info.FileAttributes = FILE_ATTRIBUTE_DIRECTORY;
	else
		info.FileAttributes = FILE_ATTRIBUTE_ARCHIVE;
	info.FileNameLength = de->name.Length;
	info.EndOfFile.QuadPart = de->st.st_size;
	info.AllocationSize.QuadPart = de->st.st_blocks * 512;

	r = copy_to_user( FileInformation, &info, sizeof info );
	if (r < STATUS_SUCCESS)
		return r;

	const ULONG ofs = FIELD_OFFSET(FILE_BOTH_DIRECTORY_INFORMATION, FileName);
	PWSTR p = (PWSTR)((PBYTE)FileInformation + ofs);
	r = copy_to_user( p, de->name.Buffer, de->name.Length );
	if (r < STATUS_SUCCESS)
		return r;

	IO_STATUS_BLOCK iosb;
	iosb.Status = r;
	iosb.Information = ofs + de->name.Length;

	copy_to_user( IoStatusBlock, &iosb, sizeof iosb );

	return r;
}

NTSTATUS NTAPI NtQueryFullAttributesFile(
	POBJECT_ATTRIBUTES ObjectAttributes,
	PFILE_NETWORK_OPEN_INFORMATION FileInformation)
{
	object_attributes_t oa;
	NTSTATUS r;

	r = oa.copy_from_user( ObjectAttributes );
	if (r < STATUS_SUCCESS)
		return r;

	trace("name = %pus\n", oa.ObjectName);

	return STATUS_NOT_IMPLEMENTED;
}
