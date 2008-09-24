AC_INIT(ntnative,0.1)

AC_CONFIG_HEADERS([include/config.h])
AC_CHECK_HEADERS([asm/ptrace.h])

dnl AC_DEFINE([ASM_NAME_PREFIX],,[Symbol prefix])
AC_MSG_CHECKING([asm symbol prefix])
AC_TRY_LINK([], [__asm__("\n\tjmp _main\n":::"memory");],
	[AC_DEFINE([ASM_NAME_PREFIX],["_"],[Symbol prefix])
	AC_MSG_RESULT(underscore)],
	[AC_DEFINE([ASM_NAME_PREFIX],[""],[Symbol prefix])
	AC_MSG_RESULT(none)])

AC_CHECK_PROGS(MINGW32CC,i686-pc-mingw32-gcc i586-mingw32msvc-cc)
AC_CHECK_PROGS(MINGW32STRIP,i686-pc-mingw32-strip i586-mingw32msvc-strip)
AC_CHECK_PROGS(MINGW32AS,i686-pc-mingw32-as i586-mingw32msvc-as)
AC_CHECK_PROGS(MINGW32AR,i686-pc-mingw32-ar i586-mingw32msvc-ar)
AC_CHECK_PROGS(MINGW32RANLIB,i686-pc-mingw32-ranlib i586-mingw32msvc-ranlib)
AC_CHECK_PROGS(MINGW32WINDRES,i686-pc-mingw32-windres i586-mingw32msvc-windres)
AC_CHECK_PROGS(CABEXTRACT,cabextract)

AC_CHECK_FUNCS(clone)
AC_CHECK_FUNCS(sigqueue)

AC_CONFIG_FILES(
	Makefile
	loader/Makefile
	tests/Makefile
	libudis86/Makefile
)

AC_OUTPUT
