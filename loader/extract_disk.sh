# nt loader
#
# Copyright 2006-2008 Mike McCormack
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
# 
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
#

sys32files="
 ADVAPI32.DL_
 AUTOCHK.EXE
 BASESRV.DL_
 C_1252.NL_
 C_850.NL_
 C_437.NL_
 CGA80WOA.FO_
 CGA40WOA.FO_
 CSRSRV.DL_
 CSRSS.EX_
 CMD.EX_
 COMCTL32.DL_
 CRYPT32.DL_
 CRYPTDLL.DL_
 CTYPE.NL_
 DIGEST.DL_
 DNSAPI.DL_
 DOSAPP.FO_
 EGA80WOA.FO_
 EGA40WOA.FO_
 GDI32.DL_
 IMM32.DL_
 KBDUS.DLL
 KERNEL32.DL_
 KERBEROS.DL_
 L_INTL.NL_
 LOCALE.NL_
 LSASRV.DL_
 LSASS.EX_
 MSASN1.DL_
 MSAPSSPC.DL_
 MSGINA.DL_
 MSNSSPC.DL_
 MSPRIVS.DL_
 MSV1_0.DL_
 MSVCRT.DLL
 MSVCIRT.DL_
 MSVCRT40.DL_
 NDDEAPI.DL_
 NETAPI32.DL_
 NETRAP.DL_
 NTDLL.DLL
 NTDSAPI.DL_
 PROFMAP.DL_
 RPCRT4.DL_
 SAMLIB.DL_
 SAMSRV.DL_
 SAVEDUMP.EX_
 SCESRV.DL_
 SCHANNEL.DLL
 SECUR32.DL_
 SERVICES.EX_
 SFC.DL_
 SFCFILES.DL_
 SHELL32.DL_
 SHLWAPI.DL_
 SMSS.EX_
 SORTKEY.NL_
 SORTTBLS.NL_
 UMPNPMGR.DL_
 USER32.DL_
 USERENV.DL_
 UNICODE.NL_
 UNIPROC/WINSRV.DL_
 WINLOGON.EX_
 WINSTA.DL_
 WLDAP32.DL_
 WS2_32.DL_
 WS2HELP.DL_
 WSOCK32.DL_
"

tmp="/tmp/win2k-temp.cab"
root="drive"
target="$root/winnt/system32"
iso="win2k.iso"

if test \! -f "$iso"
then
	echo "Missing a Windows 2000 ISO image to extract files from"
	exit 1
fi

mkdir -p "$target"
mkdir -p "$root/winnt/temp"
mkdir -p "$root/winnt/security"
mkdir -p "$root/winnt/security/logs"
mkdir -p "$root/program files"
mkdir -p "$root/program files/common files"
mkdir -p "$root/tests"
ln -s "$root" c:

# add dummy security database
dd if=/dev/null of=drive/winnt/security/res1.log bs=1024 count=1024
dd if=/dev/null of=drive/winnt/security/res2.log bs=1024 count=1024
dd if=/dev/null of=drive/winnt/security/edb.chk bs=8192 count=1
dd if=/dev/null of=drive/winnt/security/edb.log bs=1024 count=1024
dd if=/dev/null of=drive/winnt/security/edb007ec.log bs=1024 count=1024

for file in $sys32files
do
	# copy a file from the ISO
	isoinfo -x "/I386/$file" -i "$iso" > "$tmp"
	if test \! -s "$tmp"
	then
		echo "Failed to extract $file"
		continue
	fi

	# extract the file
	compressed=`echo $file | sed -e 's/.$/_/'`
	if test "x$compressed" = "x$file"
	then
		echo "Extracting $file"
		cabextract -L -d "$target" "$tmp"
	else
		lower=`echo "$file" | tr A-Z a-z`
		echo "Copying    $lower"
		cp "$tmp" "$target/$lower"
	fi
done

cat > "$root/winnt/system.ini" <<EOF
[drivers]
wave=mmdrv.dll
timer=timer.drv

[mci]
[driver32]
[386enh]
woafont=dosapp.FON
EGA80WOA.FON=EGA80WOA.FON
EGA40WOA.FON=EGA40WOA.FON
CGA80WOA.FON=CGA80WOA.FON
CGA40WOA.FON=CGA40WOA.FON

EOF

cat > "$root/winnt/win.ini" <<EOF
; empty

EOF

for testexe in atom event file mutant port reg runnt section seh sema thread token virtual
do
	cp ../tests/$testexe.exe $root/tests
done
