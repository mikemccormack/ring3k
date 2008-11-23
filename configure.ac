
AC_INIT(ring3k,0.0.12)

AC_CONFIG_HEADERS([include/config.h])
AC_CHECK_HEADERS([asm/ptrace.h])
AC_CHECK_HEADERS([valgrind/valgrind.h])

AC_MSG_CHECKING([asm symbol prefix])
AC_TRY_LINK([], [__asm__("\n\tjmp _main\n":::"memory");],
	[AC_DEFINE([ASM_NAME_PREFIX],["_"],[Symbol prefix])
	AC_MSG_RESULT(underscore)],
	[AC_DEFINE([ASM_NAME_PREFIX],[""],[Symbol prefix])
	AC_MSG_RESULT(none)])

CPPFLAGS="$CPPFLAGS -I/usr/include/libxml2"
AC_CHECK_HEADERS(libxml/parser.h,
	[AC_CHECK_LIB(xml2, xmlParseMemory,
		[AC_DEFINE(HAVE_LIBXML2, 1, [Define if you have the libxml2 library])])])

AC_CHECK_HEADERS(SDL/SDL.h,
	[AC_CHECK_LIB(SDL, SDL_Init,
		[AC_DEFINE(HAVE_SDL, 1, [Define if you have the SDL library])])])

AC_CHECK_PROGS(MINGW32CC,i686-pc-mingw32-gcc i586-mingw32msvc-cc no)
AC_CHECK_PROGS(MINGW32STRIP,i686-pc-mingw32-strip i586-mingw32msvc-strip no)
AC_CHECK_PROGS(MINGW32AS,i686-pc-mingw32-as i586-mingw32msvc-as no)
AC_CHECK_PROGS(MINGW32AR,i686-pc-mingw32-ar i586-mingw32msvc-ar no)
AC_CHECK_PROGS(MINGW32RANLIB,i686-pc-mingw32-ranlib i586-mingw32msvc-ranlib no)
AC_CHECK_PROGS(MINGW32WINDRES,i686-pc-mingw32-windres i586-mingw32msvc-windres no)

AC_CHECK_PROGS(CABEXTRACT,cabextract no)

AC_CHECK_PROGS(ISOINFO,isoinfo no)

AC_CHECK_FUNCS(clone)
AC_CHECK_FUNCS(sigqueue)

AC_CONFIG_FILES(
	Makefile
	loader/Makefile
	tests/Makefile
	tools/Makefile
	libudis86/Makefile
)

AC_OUTPUT

dnl Warning about missing dependencies
if test "$ac_cv_lib_xml2_xmlParseMemory" != "yes" -o \
	"$ac_cv_header_libxml_parser_h" != "yes"
then
	AC_MSG_WARN([libxml2 development libraries not present (Debian package is libxml2-dev)])
fi

if test "$ac_cv_lib_SDL_SDL_Init" != "yes" -o \
	"$ac_cv_header_SDL_SDL_h" != "yes"
then
	AC_MSG_WARN([libSDL development libraries not present (Debian package is libsdl1.2-dev)])
fi

if test x"$MINGW32CC" = "x"
then
	AC_MSG_WARN([mingw32 not present. (Debian package is mingw32)])
fi

if test x"$CABEXTRACT" = "x"
then
	AC_MSG_WARN([cabextract not present. (Debian package is cabextract)])
fi

if test x"$ISOINFO" = "x"
then
	AC_MSG_WARN([isoinfo not present. (Debian package is cdrecord)])
fi
