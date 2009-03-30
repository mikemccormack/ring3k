dnl
dnl ring3k configure script
dnl
dnl Process this file with autoconf to produce a configure script
dnl

AC_INIT(ring3k,0.0.23)
AC_CONFIG_SRCDIR(kernel/thread.cpp)

AC_PROG_CC
AC_PROG_CXX

AC_SUBST(HOSTCC,${CC})
AC_SUBST(HOSTCXX,${CXX})
AC_SUBST(CC,"${CC} -m32")
AC_SUBST(CXX,"${CXX} -m32")
AC_PROG_INSTALL

AC_CONFIG_HEADERS([include/config.h])
AC_CHECK_HEADERS([asm/ptrace.h])
AC_CHECK_HEADERS([valgrind/valgrind.h])

dnl Check how to jmp to a C function from asm
AC_MSG_CHECKING([asm symbol prefix])
AC_TRY_LINK([], [__asm__("\n\tjmp _main\n":::"memory");],
	[AC_DEFINE([ASM_NAME_PREFIX],["_"],[Symbol prefix])
	AC_MSG_RESULT(underscore)],
	[AC_DEFINE([ASM_NAME_PREFIX],[""],[Symbol prefix])
	AC_MSG_RESULT(none)])

dnl Check for libxml2
ac_save_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS -I/usr/include/libxml2"
AC_CHECK_HEADERS(libxml/parser.h,
	[AC_CHECK_LIB(xml2, xmlParseMemory,
		[AC_DEFINE(HAVE_LIBXML2, 1, [Define if you have the libxml2 library])])])
AC_SUBST(XML2LIBS,"-lxml2")
AC_SUBST(XML2INCL,"-I/usr/include/libxml2")

dnl Check for SDL
AC_CHECK_HEADERS(SDL/SDL.h,
	[AC_CHECK_LIB(SDL, SDL_Init,
		[AC_DEFINE(HAVE_SDL, 1, [Define if you have the SDL library])]
		[AC_SUBST(LIBSDL,"-lSDL")])]
	[AC_CHECK_LIB(SDL-1.2, SDL_Init,
		[AC_DEFINE(HAVE_SDL, 1, [Define if you have the SDL-1.2 library])]
		[AC_SUBST(LIBSDL,"-lSDL-1.2")])])
CPPFLAGS="$ac_save_CPPFLAGS"

dnl Check for Freetype (mostly borrowed from Wine's configure.ac)
AC_CHECK_PROGS(ft_devel,[freetype-config freetype2-config],no)
if test "$ft_devel" != "no"
then
	ac_freetype_incl=`$ft_devel --cflags`
	ac_freetype_libs=`$ft_devel --libs`
fi
ac_save_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS $ac_freetype_incl"
AC_CHECK_HEADERS(ft2build.h \
	freetype/freetype.h,,,[
	#ifdef HAVE_FT2BUILD_H
	# include <ft2build.h>
	#endif
	])
AC_CHECK_LIB(freetype,FT_Init_FreeType,[ft_lib=yes],[ft_lib=no],[$ac_freetype_libs])
CPPFLAGS="$ac_save_CPPFLAGS"
if test "$ac_cv_header_freetype_freetype_h" = "yes" -a "$ft_lib" = "yes"
then
	AC_DEFINE(HAVE_FREETYPE, 1, [Define if you have the Freetype 2 library])
	AC_SUBST(FREETYPELIBS,"$ac_freetype_libs")
	AC_SUBST(FREETYPEINCL,"$ac_freetype_incl")
fi

dnl Check for Cairo
AC_CHECK_PROGS(PKG_CONFIG, pkg-config, false)
if test "$PKG_CONFIG" != "false"
then
	ac_cairo_libs="`$PKG_CONFIG --libs cairo 2>/dev/null`"
	ac_cairo_incl="`$PKG_CONFIG --cflags cairo 2>/dev/null`"
fi
ac_save_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS $ac_cairo_incl"
AC_CHECK_HEADERS(cairo.h \
	cairo-xlib.h,
	[AC_CHECK_LIB(cairo, cairo_xlib_surface_create,
		[cairo_lib=yes],[cairo_lib=no],[$ac_cairo_libs])])
if test "$cairo_lib" != "no"
then
	AC_DEFINE(HAVE_CAIRO, 1, [Define if you have the cairo library])
	AC_SUBST(CAIROLIBS,"$ac_cairo_libs")
	AC_SUBST(CAIROINCL,"$ac_cairo_incl")
fi

dnl Check for mingw32 cross compilation environment
AC_CHECK_PROGS(MINGW32CC,i686-pc-mingw32-gcc i586-mingw32msvc-cc no)
AC_CHECK_PROGS(MINGW32STRIP,i686-pc-mingw32-strip i586-mingw32msvc-strip no)
AC_CHECK_PROGS(MINGW32AS,i686-pc-mingw32-as i586-mingw32msvc-as no)
AC_CHECK_PROGS(MINGW32AR,i686-pc-mingw32-ar i586-mingw32msvc-ar no)
AC_CHECK_PROGS(MINGW32RANLIB,i686-pc-mingw32-ranlib i586-mingw32msvc-ranlib no)
AC_CHECK_PROGS(MINGW32WINDRES,i686-pc-mingw32-windres i586-mingw32msvc-windres no)

AC_CHECK_PROGS(ISOINFO,isoinfo no)

AC_CHECK_FUNCS(clone)
AC_CHECK_FUNCS(sigqueue)
AC_CHECK_FUNCS(fstatat)

AC_SUBST_FILE([MAKE_RULES])
MAKE_RULES=Make.rules

AC_CONFIG_FILES(
	Make.rules
	Makefile
	kernel/Makefile
	tests/Makefile
	tools/Makefile
	libudis86/Makefile
	libmspack/Makefile
	libntreg/Makefile
	programs/Makefile
	programs/minitris/Makefile
	programs/minshell/Makefile
	programs/pixels/Makefile
	programs/winemine/Makefile
	programs/winlogon/Makefile
	unpack/Makefile
)

AC_OUTPUT

dnl Warning about missing dependencies
if test "$ac_cv_lib_xml2_xmlParseMemory" != "yes" -o \
	"$ac_cv_header_libxml_parser_h" != "yes"
then
	AC_MSG_WARN([libxml2 development libraries not present (Debian package is libxml2-dev)])
fi

if test "$ac_cv_lib_SDL_1_2_SDL_Init" != "yes" -a \
	"$ac_cv_lib_SDL_SDL_Init" != "yes"
then
	AC_MSG_WARN([libSDL development libraries not present (Debian package is libsdl1.2-dev)])
fi

if test "$ac_cv_header_freetype_freetype_h" != "yes" -o \
	"$ft_lib" != "yes"
then
	AC_MSG_WARN([freetype2 development libraries not present (Debian package is libfreetype6-dev)])
fi

if test x"$MINGW32CC" = "x"
then
	AC_MSG_WARN([mingw32 not present. (Debian package is mingw32)])
fi

if test x"$ISOINFO" = "x"
then
	AC_MSG_WARN([isoinfo not present. (Debian package is genisoimage)])
fi
