dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.59)

AC_INIT([uim-chewing], [0.0.2])
AM_INIT_AUTOMAKE(1.8.3)
AC_CONFIG_HEADER([config.h])

AC_PROG_CC
AC_PROG_LIBTOOL

PKG_CHECK_MODULES(CHEWING, [chewing >= 0.3.091])
CHEWING_DATADIR=`$PKG_CONFIG chewing --variable=datadir`
AC_DEFINE_UNQUOTED(CHEWING_DATADIR, "$CHEWING_DATADIR", [Chewing data dir])

PKG_CHECK_MODULES(UIM, [uim >= 1.2.0])
UIM_LIBDIR=`$PKG_CONFIG uim --variable=libdir`
UIM_SCMDIR=`$PKG_CONFIG uim --variable=uim_scmdir`

AC_SUBST(CHEWING_DATADIR)
AC_SUBST(UIM_LIBDIR)
AC_SUBST(UIM_SCMDIR)


AC_CONFIG_FILES([Makefile
                 scm/Makefile
                 src/Makefile
		 ])
AC_OUTPUT
