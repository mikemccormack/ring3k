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


#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"

#include "debug.h"
#include "object.h"
#include "object.inl"
#include "ntcall.h"
#include "token.h"

// http://blogs.msdn.com/david_leblanc/archive/2007/07/26/process-tokens-and-default-dacls.aspx
// typedef struct _ACCESS_MASK {
// WORD SpecificRights;
// BYTE StandardRights;
// BYTE AccessSystemAcl : 1;
// BYTE Reserved : 3;
// BYTE GenericAll : 1;
// BYTE GenericExecute : 1;
// BYTE GenericWrite : 1;
// BYTE GenericRead : 1;
// } ACCESS_MASK;
// typedef ACCESS_MASK *PACCESS_MASK;

class luid_and_privileges_t;
class token_privileges_t;

typedef list_anchor<luid_and_privileges_t,0> luid_and_priv_list_t;
typedef list_element<luid_and_privileges_t> luid_and_priv_entry_t;
typedef list_iter<luid_and_privileges_t,0> luid_and_priv_iter_t;

class luid_and_privileges_t : public LUID_AND_ATTRIBUTES
{
	friend class list_anchor<luid_and_privileges_t,0>;
	friend class list_iter<luid_and_privileges_t,0>;
protected:
	luid_and_priv_entry_t entry[1];
public:
	void dump();
};

void luid_and_privileges_t::dump()
{
	dprintf("%08lx %08lx %08lx\n", Luid.LowPart, Luid.HighPart, Attributes );
}

class token_privileges_t
{
	ULONG priv_count;
	luid_and_priv_list_t priv_list;
public:
	token_privileges_t();
	~token_privileges_t();
	void dump();
	ULONG get_length();
	NTSTATUS copy_from_user( PTOKEN_PRIVILEGES tp );
	NTSTATUS copy_to_user( PTOKEN_PRIVILEGES tp );
	NTSTATUS add( LUID_AND_ATTRIBUTES& la );
};

token_privileges_t::token_privileges_t() :
	priv_count(0)
{
}

token_privileges_t::~token_privileges_t()
{
	luid_and_privileges_t *priv;
	while ((priv = priv_list.head()))
	{
		priv_list.unlink( priv );
		delete priv;
	}
	priv_count = 0;
}

void token_privileges_t::dump()
{
	luid_and_priv_iter_t i(priv_list);
	while (i)
	{
		luid_and_privileges_t *priv = i;
		priv->dump();
		i.next();
	}
}

NTSTATUS token_privileges_t::add( LUID_AND_ATTRIBUTES& la )
{
	luid_and_privileges_t *priv = new luid_and_privileges_t;
	if (!priv)
		return STATUS_NO_MEMORY;
	priv->Luid = la.Luid;
	priv->Attributes = la.Attributes;
	priv_list.append( priv );
	priv_count++;
	return STATUS_SUCCESS;
}

NTSTATUS token_privileges_t::copy_from_user( PTOKEN_PRIVILEGES tp )
{
	NTSTATUS r;
	ULONG count = 0;

	r = ::copy_from_user( &count, &tp->PrivilegeCount, sizeof count );
	if (r != STATUS_SUCCESS)
		return r;

	for (ULONG i=0; i<count; i++)
	{
		LUID_AND_ATTRIBUTES la;
		r = ::copy_from_user( &la, &tp->Privileges[i], sizeof la );
		if (r != STATUS_SUCCESS)
			return r;
		r = add( la );
		if (r != STATUS_SUCCESS)
			return r;
	}

	return r;
}

ULONG token_privileges_t::get_length()
{
	return sizeof priv_count + priv_count * sizeof (LUID_AND_ATTRIBUTES);
}

NTSTATUS token_privileges_t::copy_to_user( PTOKEN_PRIVILEGES tp )
{
	NTSTATUS r;

	r = ::copy_to_user( &tp->PrivilegeCount, &priv_count, sizeof priv_count );
	if (r != STATUS_SUCCESS)
		return r;

	luid_and_priv_iter_t i(priv_list);
	ULONG n = 0;
	while (i)
	{
		luid_and_privileges_t *priv = i;
		LUID_AND_ATTRIBUTES* la = priv;
		r = ::copy_to_user( &tp->Privileges[n], la, sizeof *la );
		if (r != STATUS_SUCCESS)
			break;
		i.next();
		n++;
	}

	return r;
}

class user_copy_t
{
public:
	virtual ULONG get_length() = 0;
	virtual NTSTATUS copy_to_user(PVOID) = 0;
	virtual ~user_copy_t() = 0;
};

user_copy_t::~user_copy_t()
{
}

class sid_t : public user_copy_t
{
public:
	BYTE Revision;
	SID_IDENTIFIER_AUTHORITY IdentifierAuthority;
protected:
	BYTE SubAuthorityCount;
	ULONG *subauthority;
	inline ULONG sid_len() { return FIELD_OFFSET( SID, SubAuthority ); }
public:
	sid_t();
	NTSTATUS copy_from_user( PSID sid );
	virtual ULONG get_length();
	virtual NTSTATUS copy_to_user( PVOID sid );
	void set_subauth_count( BYTE count );
	void set_subauth( ULONG n, ULONG subauth );
	ULONG get_subauth_count() {return SubAuthorityCount;}
	void dump();
};

sid_t::sid_t() :
	Revision(1),
	SubAuthorityCount(0),
	subauthority(0)
{
}

void sid_t::dump()
{
	BYTE* b = IdentifierAuthority.Value;
	dprintf("sid: %02x %02x %02x-%02x-%02x-%02x-%02x-%02x\n",
		Revision, SubAuthorityCount, b[0], b[1], b[2], b[3], b[4], b[6]);
}

void sid_t::set_subauth_count( BYTE count )
{
	if (subauthority)
		delete subauthority;
	SubAuthorityCount = count;
	subauthority = new ULONG[count];
}

void sid_t::set_subauth( ULONG n, ULONG subauth )
{
	assert(n < SubAuthorityCount);
	subauthority[n] = subauth;
}

ULONG sid_t::get_length()
{
	return sid_len() + SubAuthorityCount * sizeof (ULONG);
}

NTSTATUS sid_t::copy_from_user( PSID psid )
{
	NTSTATUS r;
	SID sid;

	r = ::copy_from_user( &sid, psid, sid_len() );
	if (r != STATUS_SUCCESS)
		return r;

	Revision = sid.Revision;
	IdentifierAuthority = sid.IdentifierAuthority;
	set_subauth_count( sid.SubAuthorityCount );

	PISID pisid = (PISID) psid;
	r = ::copy_from_user( subauthority, &pisid->SubAuthority, SubAuthorityCount * sizeof (ULONG));

	return r;
}

NTSTATUS sid_t::copy_to_user( PSID psid )
{
	NTSTATUS r;
	SID sid;

	sid.Revision = Revision;
	sid.IdentifierAuthority = IdentifierAuthority;
	sid.SubAuthorityCount = SubAuthorityCount;

	r = ::copy_to_user( psid, &sid, sid_len() );
	if (r != STATUS_SUCCESS)
		return r;

	PISID pisid = (PISID) psid;
	r = ::copy_to_user( &pisid->SubAuthority, subauthority, SubAuthorityCount * sizeof (ULONG));

	return r;
}

// wrapper for SID_AND_ATTRIBUTES
class sid_and_attributes_t
{
	sid_t sid;
	ULONG Attributes;
public:
	sid_and_attributes_t();
	ULONG get_length();
	NTSTATUS copy_hdr_to_user( SID_AND_ATTRIBUTES* psida, ULONG ofs );
	NTSTATUS copy_to_user( SID_AND_ATTRIBUTES* sida );
	sid_t &get_sid();
};

sid_and_attributes_t::sid_and_attributes_t():
	Attributes( 0 )
{
}

sid_t &sid_and_attributes_t::get_sid()
{
	return sid;
}

ULONG sid_and_attributes_t::get_length()
{
	return sizeof (SID_AND_ATTRIBUTES) + sid.get_length();
}

NTSTATUS sid_and_attributes_t::copy_hdr_to_user( SID_AND_ATTRIBUTES* psida, ULONG ofs )
{
	SID_AND_ATTRIBUTES sida;
	sida.Attributes = Attributes;
	sida.Sid = (PSID) ((BYTE*)psida + ofs);
	return ::copy_to_user( psida, &sida, sizeof sida );
}

NTSTATUS sid_and_attributes_t::copy_to_user( SID_AND_ATTRIBUTES* psida )
{
	NTSTATUS r;
	r = copy_hdr_to_user( psida, sizeof *psida );
	if (r != STATUS_SUCCESS)
		return r;

	return sid.copy_to_user( (PSID)(psida + 1) );
}

class token_groups_t
{
	ULONG count;
	sid_and_attributes_t *sa;
protected:
	void reset();
public:
	token_groups_t();
	~token_groups_t();
	ULONG get_length();
	NTSTATUS copy_to_user( TOKEN_GROUPS *tg );
	void set_count( ULONG n );
	sid_and_attributes_t& get_sa( ULONG n );
};

token_groups_t::token_groups_t() :
	count(0),
	sa(0)
{
}

token_groups_t::~token_groups_t()
{
	reset();
}

void token_groups_t::reset()
{
	delete sa;
	sa = 0;
	count = 0;
}

// assume this is only done once
void token_groups_t::set_count( ULONG n )
{
	reset();
	sa = new sid_and_attributes_t[n];
	count = n;
}

sid_and_attributes_t& token_groups_t::get_sa( ULONG n )
{
	if (n >= count)
		throw;
	return sa[n];
}

ULONG token_groups_t::get_length()
{
	ULONG len = sizeof (ULONG);

	for (ULONG i=0; i<count; i++)
		len += sa[i].get_length();

	return len;
}

NTSTATUS token_groups_t::copy_to_user( TOKEN_GROUPS *tg )
{
	NTSTATUS r;
	r = ::copy_to_user( &tg->GroupCount, &count, sizeof count );
	if (r != STATUS_SUCCESS)
		return r;

	// Copying multiple SID_AND_ATTRIBUTES structs is a bit complex.
	// The SID_AND_ATTRIBUTES and the SID it points to are separated.
	// The order should be:
	//    ULONG GroupCount;
	//    SID_AND_ATTRIBUTES Groups[GroupCount];
	//    1st SID
	//    2nd SID
	//    ...
	ULONG ofs = sizeof (ULONG) + count * sizeof (SID_AND_ATTRIBUTES);
	for (ULONG i=0; i<count; i++)
	{
		r = sa[i].copy_hdr_to_user( &tg->Groups[i], ofs );
		if (r != STATUS_SUCCESS)
			return r;
		r = sa[i].get_sid().copy_to_user( (PSID) ((BYTE*) tg + ofs) );
		if (r != STATUS_SUCCESS)
			return r;
		ofs += sa[i].get_sid().get_length();
	}

	return r;
}

// access control entry
class ace_t;

typedef list_anchor<ace_t,0> ace_list_t;
typedef list_element<ace_t> ace_entry_t;
typedef list_iter<ace_t,0> ace_iter_t;

class ace_t : public user_copy_t
{
	struct ace_common_t {
		ACE_HEADER header;
		ULONG mask;
		ULONG sid_start;
	};
	friend class list_anchor<ace_t,0>;
	friend class list_iter<ace_t,0>;
protected:
	ace_entry_t entry[1];
	BYTE type;
	BYTE flags;
	ULONG mask;
	sid_t sid;
public:
	ace_t(BYTE type);
	virtual ULONG get_length();
	virtual NTSTATUS copy_to_user( PVOID pace );
	virtual ~ace_t();
	sid_t& get_sid();
};

ace_t::ace_t(BYTE _type) :
	type( _type ),
	flags( 0 )
{
}

ace_t::~ace_t()
{
}

sid_t& ace_t::get_sid()
{
	return sid;
}

ULONG ace_t::get_length()
{
	return sizeof (ace_common_t) + sid.get_length();
}

NTSTATUS ace_t::copy_to_user( PVOID pace )
{
	BYTE *p = (BYTE*) pace;

	ace_common_t ace;

	ace.header.AceType = type;
	ace.header.AceFlags = flags;
	ace.header.AceSize = sizeof ace + sid.get_length();
	ace.mask = mask;
	ace.sid_start = sizeof ace;

	NTSTATUS r = ::copy_to_user( pace, &ace, sizeof ace );
	if (r != STATUS_SUCCESS)
		return r;

	return sid.copy_to_user( (PVOID) ((PBYTE) p + sizeof ace) );
}

class access_allowed_ace_t : public ace_t
{
public:
	access_allowed_ace_t();
};

access_allowed_ace_t::access_allowed_ace_t() :
	ace_t( ACCESS_ALLOWED_ACE_TYPE )
{
}

class access_denied_ace_t : public ace_t
{
public:
	access_denied_ace_t();
};

access_denied_ace_t::access_denied_ace_t() :
	ace_t( ACCESS_DENIED_ACE_TYPE )
{
}

class system_audit_ace_t : public ace_t
{
public:
	system_audit_ace_t();
};

system_audit_ace_t::system_audit_ace_t() :
	ace_t( SYSTEM_AUDIT_ACE_TYPE )
{
}

class system_alarm_ace_t : public ace_t
{
public:
	system_alarm_ace_t();
};

system_alarm_ace_t::system_alarm_ace_t() :
	ace_t( SYSTEM_ALARM_ACE_TYPE )
{
}

// access control list
class acl_t : public user_copy_t, protected ACL
{
	ace_list_t ace_list;
public:
	virtual ULONG get_length();
	virtual NTSTATUS copy_to_user( PVOID pacl );
	virtual ~acl_t();
	NTSTATUS copy_from_user( PACL pacl );
	void add( ace_t *ace );
};

acl_t::~acl_t()
{
	ace_t *ace;
	while ((ace = ace_list.head()))
	{
		ace_list.unlink( ace );
		delete ace;
	}
}

ULONG acl_t::get_length()
{
	ULONG len = sizeof (ACL);
	ace_iter_t i(ace_list);
	while (i)
	{
		ace_t *ace = i;
		len += ace->get_length();
		i.next();
	}
	return len;
}

NTSTATUS acl_t::copy_to_user( PVOID pacl )
{
	PACL acl = this;
	ULONG ofs = sizeof *acl;
	NTSTATUS r = ::copy_to_user( pacl, acl, ofs );

	ace_iter_t i(ace_list);
	while (i && r == STATUS_SUCCESS)
	{
		ace_t *ace = i;
		r = ace->copy_to_user( (PVOID) ((BYTE*) pacl + ofs) );
		ofs += ace->get_length();
		i.next();
	}
	return r;
}

void acl_t::add( ace_t *ace )
{
	ace_list.append( ace );
}

class privilege_set_t
{
	ULONG count;
	ULONG control;
	LUID_AND_ATTRIBUTES *privileges;
protected:
	void reset();
	void set_count( ULONG count );
public:
	privilege_set_t();
	~privilege_set_t();
	NTSTATUS copy_from_user( PPRIVILEGE_SET ps );
};

privilege_set_t::privilege_set_t() :
	count(0),
	control(0),
	privileges(0)
{
}

privilege_set_t::~privilege_set_t()
{
	reset();
}

void privilege_set_t::reset()
{
	if (count)
	{
		delete privileges;
		count = 0;
	}
}

void privilege_set_t::set_count( ULONG n )
{
	reset();
	privileges = new LUID_AND_ATTRIBUTES[n];
	count = n;
}

NTSTATUS privilege_set_t::copy_from_user( PPRIVILEGE_SET ps )
{
	struct {
		ULONG count;
		ULONG control;
	} x;

	NTSTATUS r = ::copy_from_user( &x, ps, sizeof x );
	if (r != STATUS_SUCCESS)
		return r;

	set_count( x.count );
	control = x.control;

	r = ::copy_from_user( privileges, ps->Privilege, count * sizeof privileges[0] );

	return STATUS_SUCCESS;
}

token_t::~token_t()
{
}

class token_impl_t : public token_t {
	token_privileges_t privs;
	sid_t owner;
	sid_t primary_group;
	acl_t default_dacl;
	sid_and_attributes_t user;
	token_groups_t groups;
public:
	token_impl_t();
	virtual ~token_impl_t();
	virtual token_privileges_t& get_privs();
	virtual sid_t& get_owner();
	virtual sid_and_attributes_t& get_user();
	virtual sid_t& get_primary_group();
	virtual token_groups_t& get_groups();
	virtual acl_t& get_default_dacl();
	virtual NTSTATUS adjust(token_privileges_t& privs);
	NTSTATUS add( LUID_AND_ATTRIBUTES& la );
};

token_impl_t::token_impl_t()
{
	// FIXME: make this a default local computer account with privileges disabled
	LUID_AND_ATTRIBUTES la;

	//la.Luid.LowPart = SECURITY_LOCAL_SYSTEM_RID;
	la.Luid.LowPart = SECURITY_LOCAL_SERVICE_RID;
	la.Luid.HighPart = 0;
	la.Attributes = 0;

	privs.add( la );

	static const SID_IDENTIFIER_AUTHORITY auth = {SECURITY_NT_AUTHORITY};
	memcpy( &owner.IdentifierAuthority, &auth, sizeof auth );
	owner.set_subauth_count( 1 );
	owner.set_subauth( 0, SECURITY_LOCAL_SYSTEM_RID );

	memcpy( &primary_group.IdentifierAuthority, &auth, sizeof auth );
	primary_group.set_subauth_count( 1 );
	primary_group.set_subauth( 0, DOMAIN_GROUP_RID_COMPUTERS );

	sid_t &user_sid = user.get_sid();
	memcpy( &user_sid.IdentifierAuthority, &auth, sizeof auth );
	user_sid.set_subauth_count( 1 );
	user_sid.set_subauth( 0, SECURITY_LOCAL_SYSTEM_RID );
}

token_impl_t::~token_impl_t()
{
}

sid_t& token_impl_t::get_owner()
{
	return owner;
}

sid_t& token_impl_t::get_primary_group()
{
	return primary_group;
}

token_groups_t& token_impl_t::get_groups()
{
	return groups;
}

sid_and_attributes_t& token_impl_t::get_user()
{
	return user;
}

token_privileges_t& token_impl_t::get_privs()
{
	return privs;
}

acl_t& token_impl_t::get_default_dacl()
{
	return default_dacl;
}

NTSTATUS token_impl_t::adjust(token_privileges_t& privs)
{
	return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtOpenProcessToken(
	HANDLE Process,
	ACCESS_MASK DesiredAccess,
	PHANDLE Token )
{
	NTSTATUS r;

	dprintf("%p %08lx %p\n", Process, DesiredAccess, Token);

	r = verify_for_write( Token, sizeof *Token );
	if (r != STATUS_SUCCESS)
		return r;

	process_t *p = 0;
	r = object_from_handle( p, Process, 0 );
	if (r != STATUS_SUCCESS)
		return r;

	token_t *token = new token_impl_t;
	if (!token)
		return STATUS_NO_MEMORY;

	r = alloc_user_handle( token, DesiredAccess, Token );
	release( token );

	return r;
}

NTSTATUS NTAPI NtOpenThreadToken(
	HANDLE Thread,
	ACCESS_MASK DesiredAccess,
	BOOLEAN OpenAsSelf,
	PHANDLE TokenHandle )
{
	NTSTATUS r;

	dprintf("%p %08lx %u %p\n", Thread, DesiredAccess, OpenAsSelf, TokenHandle);

	r = verify_for_write( TokenHandle, sizeof *TokenHandle );
	if (r != STATUS_SUCCESS)
		return r;

	thread_t *t = 0;
	r = object_from_handle( t, Thread, DesiredAccess );
	if (r != STATUS_SUCCESS)
		return r;

	token_t *tok = t->get_token();
	if (tok == 0)
		return STATUS_NO_TOKEN;

	r = alloc_user_handle( tok, DesiredAccess, TokenHandle );

	return r;
}

NTSTATUS NTAPI NtAdjustPrivilegesToken(
	HANDLE TokenHandle,
	BOOLEAN DisableAllPrivileges,
	PTOKEN_PRIVILEGES NewState,
	ULONG BufferLength,
	PTOKEN_PRIVILEGES PreviousState,
	PULONG ReturnLength )
{
	NTSTATUS r;

	dprintf("%p %u %p %lu %p %p\n", TokenHandle, DisableAllPrivileges,
			NewState, BufferLength, PreviousState, ReturnLength );

	if (ReturnLength)
	{
		r = verify_for_write( ReturnLength, sizeof *ReturnLength );
		if (r != STATUS_SUCCESS)
			return r;
	}

	if (!NewState)
		return STATUS_INVALID_PARAMETER;

	// getting the old state requires query rights
	ACCESS_MASK mask = TOKEN_ADJUST_PRIVILEGES;
	if (PreviousState)
		mask |= TOKEN_QUERY;

	token_t *token = 0;
	r = object_from_handle( token, TokenHandle, mask );
	if (r != STATUS_SUCCESS)
		return r;

	token_privileges_t privs;
	r = privs.copy_from_user( NewState );
	if (r != STATUS_SUCCESS)
		return r;

	// return previous state information if required
	if (PreviousState)
	{
		token_privileges_t& prev_state = token->get_privs();

		ULONG len = prev_state.get_length();
		dprintf("old privs %ld bytes\n", len);
		prev_state.dump();
		if (len > BufferLength)
			return STATUS_BUFFER_TOO_SMALL;

		r = prev_state.copy_to_user( PreviousState );
		if (r != STATUS_SUCCESS)
			return r;

		r = copy_to_user( ReturnLength, &len, sizeof len );
		assert( r == STATUS_SUCCESS );
	}

	r = token->adjust( privs );

	dprintf("new privs\n");
	privs.dump();

	return r;
}

static NTSTATUS copy_ptr_to_user( user_copy_t& item, PVOID info, ULONG infolen, ULONG& retlen )
{
	// really screwy - have to write back a pointer to the sid, then the sid
	retlen = item.get_length() + sizeof (PVOID);
	if (retlen > infolen)
		return STATUS_BUFFER_TOO_SMALL;

	// pointer followed by the data blob
	PVOID ptr = (PVOID) ((PVOID*) info + 1);
	NTSTATUS r = copy_to_user( info, &ptr, sizeof ptr );
	if (r != STATUS_SUCCESS)
		return r;
	return item.copy_to_user( ptr );
}

NTSTATUS NTAPI NtQueryInformationToken(
	HANDLE TokenHandle,
	TOKEN_INFORMATION_CLASS TokenInformationClass,
	PVOID TokenInformation,
	ULONG TokenInformationLength,
	PULONG ReturnLength )
{
	token_t *token;
	ULONG len;
	NTSTATUS r;
	TOKEN_STATISTICS stats;

	dprintf("%p %u %p %lu %p\n", TokenHandle, TokenInformationClass,
			TokenInformation, TokenInformationLength, ReturnLength );

	r = object_from_handle( token, TokenHandle, TOKEN_QUERY );
	if (r != STATUS_SUCCESS)
		return r;

	switch( TokenInformationClass )
	{
	case TokenOwner:
		dprintf("TokenOwner\n");
		r = copy_ptr_to_user( token->get_owner(), TokenInformation, TokenInformationLength, len );
		break;

	case TokenPrimaryGroup:
		dprintf("TokenPrimaryGroup\n");
		r = copy_ptr_to_user( token->get_primary_group(), TokenInformation, TokenInformationLength, len );
		break;

	case TokenDefaultDacl:
		dprintf("TokenDefaultDacl\n");
		r = copy_ptr_to_user( token->get_default_dacl(), TokenInformation, TokenInformationLength, len );
		break;

	case TokenUser:
		dprintf("TokenUser\n");
		len = token->get_user().get_length();
		if (len > TokenInformationLength)
		{
			r = STATUS_BUFFER_TOO_SMALL;
			break;
		}

		r = token->get_user().copy_to_user( (SID_AND_ATTRIBUTES*) TokenInformation );
		break;

	case TokenImpersonationLevel:
		dprintf("TokenImpersonationLevel\n");
		return STATUS_INVALID_INFO_CLASS;

	case TokenStatistics:
		dprintf("TokenStatistics\n");
		len = sizeof stats;
		if (len != TokenInformationLength)
			return STATUS_INFO_LENGTH_MISMATCH;

		memset( &stats, 0, sizeof stats );
		r = copy_to_user( TokenInformation, &stats, sizeof stats );
		if (r != STATUS_SUCCESS)
			return r;

		break;

	case TokenGroups:
		dprintf("TokenGroups\n");
		len = token->get_groups().get_length();
		if (len > TokenInformationLength)
		{
			r = STATUS_BUFFER_TOO_SMALL;
			break;
		}

		r = token->get_groups().copy_to_user( (TOKEN_GROUPS*) TokenInformation );
		break;

	default:
		dprintf("info class %d\n", TokenInformationClass);
		return STATUS_INVALID_INFO_CLASS;
	}

	if (ReturnLength)
		copy_to_user( ReturnLength, &len, sizeof len );

	return r;
}

NTSTATUS NTAPI NtSetSecurityObject(
	HANDLE Handle,
	SECURITY_INFORMATION SecurityInformation,
	PSECURITY_DESCRIPTOR SecurityDescriptor )
{
	dprintf("%p %08lx %p\n", Handle, SecurityInformation, SecurityDescriptor );
	return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtDuplicateToken(
	HANDLE ExistingToken,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	BOOLEAN EffectiveOnly,
	TOKEN_TYPE TokenType,
	PHANDLE TokenHandle)
{
	token_t *existing = 0;

	NTSTATUS r = object_from_handle( existing, ExistingToken, TOKEN_QUERY );
	if (r != STATUS_SUCCESS)
		return r;

	token_t *token = new token_impl_t;
	if (!token)
		return STATUS_NO_MEMORY;

	r = alloc_user_handle( token, DesiredAccess, TokenHandle );
	release( token );

	return r;
}

NTSTATUS NTAPI NtFilterToken(
	HANDLE ExistingTokenHandle,
	ULONG Flags,
	PTOKEN_GROUPS SidsToDisable,
	PTOKEN_PRIVILEGES PrivelegesToDelete,
	PTOKEN_GROUPS SidsToRestrict,
	PHANDLE NewTokenHandle)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI NtAccessCheck(
	PSECURITY_DESCRIPTOR SecurityDescriptor,
	HANDLE TokenHandle,
	ACCESS_MASK DesiredAccess,
	PGENERIC_MAPPING GenericMapping,
	PPRIVILEGE_SET PrivilegeSet,
	PULONG PrivilegeSetLength,
	PACCESS_MASK GrantedAccess,
	PBOOLEAN AccessStatus)
{
	return STATUS_NOT_IMPLEMENTED;
}

//NtPrivilegeCheck(00000154,0006fdc0,0006fe34) ret=77fa7082
NTSTATUS NtPrivilegeCheck(
	HANDLE TokenHandle,
	PPRIVILEGE_SET RequiredPrivileges,
	PBOOLEAN Result)
{
	token_t *token = 0;

	NTSTATUS r = object_from_handle( token, TokenHandle, TOKEN_QUERY );
	if (r != STATUS_SUCCESS)
		return r;

	privilege_set_t ps;
	r = ps.copy_from_user( RequiredPrivileges );
	if (r != STATUS_SUCCESS)
		return r;

	BOOLEAN ok = TRUE;
	r = copy_to_user( Result, &ok, sizeof ok );

	return r;
}
