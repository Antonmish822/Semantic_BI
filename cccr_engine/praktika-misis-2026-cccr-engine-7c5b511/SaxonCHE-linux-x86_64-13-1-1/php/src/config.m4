dnl config.m4 for extension saxon

dnl Set edition-specific vars

SAXON_EDITION="he"
SAXON_VERSION="13.1.1"

PHP_ARG_WITH([saxonc],
  [for saxon support],
  [AS_HELP_STRING([--with-saxonc],
    [Include saxon support])])

PHP_ARG_WITH([saxonc-core],
  [for saxon support],
  [AS_HELP_STRING([--with-saxonc-core],
    [directory containing libsaxonc-core])])

check_shared_lib() {
  LIBNAME="$1"
  SAXON_DIR="$2"
  if test "$SHLIB_SUFFIX_NAME" == "dylib" ; then
    SAXON_SHARED_LIB_SUFFIX="$SAXON_EDITION.$SAXON_VERSION.dylib"
  else
    SAXON_SHARED_LIB_SUFFIX="$SAXON_EDITION.so.$SAXON_VERSION"
  fi

  FULL_LIBNAME="$LIBNAME-$SAXON_SHARED_LIB_SUFFIX"

  AC_MSG_CHECKING([for Saxon shared lib $FULL_LIBNAME files in $SAXON_DIR/lib])

  if test ! -f "$SAXON_DIR/lib/$FULL_LIBNAME" ; then
    AC_MSG_RESULT([$SAXON_DIR/lib/$FULL_LIBNAME not found])
    AC_MSG_ERROR([Please make sure you have installed the SaxonC $SAXON_VERSION distribution])
  fi
}

if test "$PHP_SAXONC" != "no"; then
  dnl Write more examples of tests here...

  dnl --with-saxonc -> check with-path
  SEARCH_PATH="/opt/saxonica /usr/local /usr" # If SaxonC has been installed
  SEARCH_FOR="include/saxonc/SaxonProcessor.h"
  if test "$PHP_SAXONC" != "yes"; then
    AC_MSG_CHECKING([for libsaxonc files in $PHP_SAXON])
    if test -r "$PHP_SAXONC/$SEARCH_FOR"; then # path given as parameter
      SAXONC_DIR="$PHP_SAXONC"
      SAXONC_CORE_DIR="$PHP_SAXONC"
      AC_MSG_RESULT(found in $PHP_SAXON)
    fi
  else # search default path list
    AC_MSG_CHECKING([for SaxonC files in default path])
    echo "SEARCH PATH: $SEARCH_PATH"
    for i in $SEARCH_PATH ; do
      echo "\$i: $i"
      echo "$i/$SEARCH_FOR"
      if test -r "$i/$SEARCH_FOR"; then
        SAXONC_DIR="$i"
        SAXONC_CORE_DIR="$i"
        AC_MSG_RESULT(found in $i)
      fi
    done
  fi

  if test -z "$SAXONC_DIR"; then
    AC_MSG_RESULT([$SAXONC_DIR/$SEARCH_FOR not found])
    AC_MSG_ERROR([Please reinstall the SaxonC distribution])
  fi
fi

if test "$PHP_SAXONC_CORE" != "no"; then
  dnl --with-saxonc-core -> check with-path
  SEARCH_FOR="include/saxonc-core-$SAXON_EDITION.h"

  if test "$PHP_SAXONC_CORE" != "yes"; then
    AC_MSG_CHECKING([for libsaxonc-core files in $PHP_SAXONC_CORE])
    SAXONC_CORE_DIR=""
    if test -r "$PHP_SAXONC_CORE/$SEARCH_FOR"; then # path given as parameter
      SAXONC_CORE_DIR="$PHP_SAXONC_CORE"
      AC_MSG_RESULT(found in $PHP_SAXONC_CORE)
    fi

    if test -z "$SAXONC_CORE_DIR"; then
      AC_MSG_RESULT([$PHP_SAXONC_CORE/$SEARCH_FOR not found])
      AC_MSG_ERROR([Please reinstall the SaxonC distribution])
    fi
  else
    if ! test -r "$SAXONC_CORE_DIR/$SEARCH_FOR"; then # use previously set default
      AC_MSG_RESULT([$SEARCH_FOR not found])
      AC_MSG_ERROR([Please reinstall the SaxonC distribution])
    fi
  fi
fi


check_shared_lib "libsaxonc-core" "$SAXONC_CORE_DIR"
check_shared_lib "libsaxonc" "$SAXONC_DIR"

dnl Main PHP settings now we know where to look
if test "$PHP_SAXONC" != "no"; then
  CXXFLAGS="-std=c++14"
  PHP_REQUIRE_CXX()
  PHP_ADD_INCLUDE(["$SAXONC_DIR/include"])
  if test "$SAXONC_CORE_DIR" != "$SAXONC_DIR"; then
    PHP_ADD_INCLUDE(["$SAXONC_CORE_DIR/include"])
    LDFLAGS="-L$SAXONC_CORE_DIR/lib -L$SAXONC_DIR/lib"
  else
    LDFLAGS="-L$SAXONC_DIR/lib"
  fi
  PHP_SUBST(SAXON_SHARED_LIBADD)
  PHP_SUBST(PHP_SAXON_API)

  dnl In case of no dependencies
  AC_DEFINE(HAVE_SAXON, 1, [ Have saxon support ])
  PHP_ADD_LIBRARY(stdc++, 1, SAXON_SHARED_LIBADD)
  PHP_ADD_LIBRARY(dl, 1, SAXON_SHARED_LIBADD)
  PHP_ADD_LIBRARY(saxonc-$SAXON_EDITION, 1, SAXON_SHARED_LIBADD)
  PHP_ADD_LIBRARY(saxonc-core-$SAXON_EDITION, 1, SAXON_SHARED_LIBADD)
  PHP_NEW_EXTENSION(saxon, php8_saxon.cpp, $ext_shared)
fi
