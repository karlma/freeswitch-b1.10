dnl -------------------------------------------------------- -*- autoconf -*-
dnl Copyright 2002-2005 The Apache Software Foundation or its licensors, as
dnl applicable.
dnl
dnl Licensed under the Apache License, Version 2.0 (the "License");
dnl you may not use this file except in compliance with the License.
dnl You may obtain a copy of the License at
dnl
dnl     http://www.apache.org/licenses/LICENSE-2.0
dnl
dnl Unless required by applicable law or agreed to in writing, software
dnl distributed under the License is distributed on an "AS IS" BASIS,
dnl WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
dnl See the License for the specific language governing permissions and
dnl limitations under the License.


dnl
dnl DBM module
dnl

dnl   APU_LIB_BERKELEY_DB(major, minor, patch, places, headers, libnames)
dnl
dnl   Search for a useable version of Berkeley DB in a number of
dnl   common places.  The installed DB must be no older than the
dnl   version given by MAJOR, MINOR, and PATCH.  All of these
dnl   arguments are allowed to be '-1', indicating we don't care.
dnl   PLACES is a list of places to search for a Berkeley DB
dnl   installation.  HEADERS is a list of headers to try.  LIBNAMES
dnl   is a list of names of the library to attempt to link against,
dnl   typically 'db' and 'db4'.
dnl
dnl   If we find a useable version, set CPPFLAGS and LIBS as
dnl   appropriate, and set the shell variable `apu_have_db' to
dnl   `1', and apu_db_lib to the matching lib name, and apu_db_header
dnl   to the header to use.  Otherwise, set `apu_have_db' to `0'.
dnl
dnl   This macro also checks for the `--with-berkeley-db=PATH' flag;
dnl   if given, the macro will use the PATH specified, and the
dnl   configuration script will die if it can't find the library.  If
dnl   the user gives the `--without-berkeley-db' flag, the entire
dnl   search is skipped.
dnl
dnl   We cache the results of individual searches under particular
dnl   prefixes, not the overall result of whether we found Berkeley
dnl   DB.  That way, the user can re-run the configure script with
dnl   different --with-berkeley-db switch values, without interference
dnl   from the cache.


AC_DEFUN([APU_CHECK_BERKELEY_DB], [
  bdb_version=$1
  if test "$2" != "-1"; then
    bdb_version="$bdb_version.$2"
    if test "$3" != "-1"; then
      bdb_version="$bdb_version.$3"
    fi
  fi
  bdb_places=$4
  bdb_default_search_headers=$5
  bdb_default_search_lib_names=$6

  apu_have_db=0

  # Save the original values of the flags we tweak.
  apu_check_lib_save_libs="$LIBS"
  apu_check_lib_save_ldflags="$LDFLAGS"
  apu_check_lib_save_cppflags="$CPPFLAGS"

  # The variable `found' is the prefix under which we've found
  # Berkeley DB, or `not' if we haven't found it anywhere yet.
  found=not
  for bdb_place in $bdb_places; do

    LDFLAGS="$apu_check_lib_save_ldflags"
    CPPFLAGS="$apu_check_lib_save_cppflags"
    case "$bdb_place" in
      "std" )
        description="the standard places"
      ;;
      *":"* )
        header="`echo $bdb_place | sed -e 's/:.*$//'`"
        lib="`echo $bdb_place | sed -e 's/^.*://'`"
        CPPFLAGS="$CPPFLAGS -I$header"
        LDFLAGS="$LDFLAGS -L$lib"
        description="$header and $lib"
      ;;
      * )
        if test -d $bdb_place; then
          LDFLAGS="$LDFLAGS -L$bdb_place/lib"
          CPPFLAGS="$CPPFLAGS -I$bdb_place/include"
        else
          AC_MSG_CHECKING([for Berkeley DB $bdb_version in $bdb_place])
          AC_MSG_RESULT([directory not found])
          continue
        fi
        description="$bdb_place"
      ;;
    esac

    # Since there is no AC_MSG_NOTICE in autoconf 2.13, we use this
    # trick to display a message instead.
    AC_MSG_CHECKING([for Berkeley DB $bdb_version in $description])
    AC_MSG_RESULT()

    for bdb_libname in $bdb_default_search_lib_names; do
      for bdb_header in $bdb_default_search_headers; do
        # Clear the header cache variable for each location
        changequote(,)
        cache_id="`echo ac_cv_header_${bdb_header} \
                 | sed -e 's/[^a-zA-Z0-9_]/_/g'`"
        changequote([,])
        unset $cache_id
        AC_CHECK_HEADER([$bdb_header], [
          if test "$1" = "3" -o "$1" = "4"; then
            # We generate a separate cache variable for each prefix and libname
            # we search under.  That way, we avoid caching information that
            # changes if the user runs `configure' with a different set of
            # switches.
            changequote(,)
            cache_id="`echo apu_cv_check_berkeley_db_$1_$2_$3_${bdb_header}_${bdb_libname}_in_${bdb_place} \
                     | sed -e 's/[^a-zA-Z0-9_]/_/g'`"
            changequote([,])

            AC_MSG_CHECKING([for -l$bdb_libname])
            dnl We can't use AC_CACHE_CHECK here, because that won't print out
            dnl the value of the computed cache variable properly.
            AC_CACHE_VAL($cache_id,
              [
                APU_TRY_BERKELEY_DB($1, $2, $3, $bdb_header, $bdb_libname)
                eval "$cache_id=$apu_try_berkeley_db"
              ])
            result="`eval echo '$'$cache_id`"
            AC_MSG_RESULT($result)
          elif test "$1" = "1"; then
            AC_CHECK_LIB($bdb_libname,
              dbopen,
              [result=yes],
              [result=no]
            )
          elif test "$1" = "2"; then
            AC_CHECK_LIB($bdb_libname,
              db_open,
              [result=yes],
              [result=no]
            )
          fi
        ], [result="no"])
        
        # If we found it, no need to search any more.
        if test "$result" = "yes"; then
          found="$bdb_place"
          break
        fi
      done
      test "$found" != "not" && break
    done
    test "$found" != "not" && break
  done

  # Restore the original values of the flags we tweak.
  LDFLAGS="$apu_check_lib_save_ldflags"
  CPPFLAGS="$apu_check_lib_save_cppflags"

  case "$found" in
  "not")
    apu_have_db=0
    ;;
  "std")
    apu_db_header=$bdb_header
    apu_db_lib=$bdb_libname
    apu_have_db=1
    ;;
  *":"*)
    header="`echo $found | sed -e 's/:.*$//'`"
    lib="`echo $found | sed -e 's/^.*://'`"

    APR_ADDTO(APRUTIL_INCLUDES, [-I$header])
    APR_ADDTO(APRUTIL_LDFLAGS, [-L$lib])
    apu_db_header=$bdb_header
    apu_db_lib=$bdb_libname
    apu_have_db=1
    ;;
  *)
    APR_ADDTO(APRUTIL_INCLUDES, [-I$found/include])
    APR_ADDTO(APRUTIL_LDFLAGS, [-L$found/lib])
    apu_db_header=$bdb_header
    apu_db_lib=$bdb_libname
    apu_have_db=1
    ;;
  esac
])


dnl   APU_TRY_BERKELEY_DB(major, minor, patch, header, libname)
dnl
dnl   A subroutine of APU_CHECK_BERKELEY_DB.
dnl
dnl   Check that a new-enough version of Berkeley DB is installed.
dnl   "New enough" means no older than the version given by MAJOR,
dnl   MINOR, and PATCH.  The result of the test is not cached; no
dnl   messages are printed.  Use HEADER as the header file to include.
dnl   Use LIBNAME as the library to link against.
dnl   (e.g. LIBNAME should usually be "db" or "db4".)
dnl
dnl   Set the shell variable `apu_try_berkeley_db' to `yes' if we found
dnl   an appropriate version installed, or `no' otherwise.
dnl
dnl   This macro uses the Berkeley DB library function `db_version' to
dnl   find the version.  If the library installed doesn't have this
dnl   function, then this macro assumes it is too old.

dnl   NOTE: This is pretty messed up.  It seems that the FreeBSD port of
dnl   Berkeley DB 4 puts the header file in /usr/local/include/db4, but the
dnl   database library in /usr/local/lib, as libdb4.[a|so].  There is no
dnl   /usr/local/include/db.h.  So if you check for /usr/local first, you'll
dnl   get the old header file from /usr/include, and the new library from
dnl   /usr/local/lib.  Disaster.  Thus this test compares the version constants
dnl   in the db.h header with the ones returned by db_version().


AC_DEFUN([APU_TRY_BERKELEY_DB],
  [
    apu_try_berkeley_db_save_libs="$LIBS"

    apu_check_berkeley_db_major=$1
    apu_check_berkeley_db_minor=$2
    apu_check_berkeley_db_patch=$3
    apu_try_berkeley_db_header=$4
    apu_try_berkeley_db_libname=$5

    LIBS="$LIBS -l$apu_try_berkeley_db_libname"
    AC_TRY_RUN(
      [
#include <stdio.h>
#include <$apu_try_berkeley_db_header>
main ()
{
  int major, minor, patch;

  db_version(&major, &minor, &patch);

  /* Sanity check: ensure that db.h constants actually match the db library */
  if (major != DB_VERSION_MAJOR
      || minor != DB_VERSION_MINOR
      || patch != DB_VERSION_PATCH)
    exit (1);

  /* Run-time check:  ensure the library claims to be the correct version. */

  if ($apu_check_berkeley_db_major != -1) {
    if (major < $apu_check_berkeley_db_major)
      exit (1);
    if (major > $apu_check_berkeley_db_major)
      exit (0);
  }

  if ($apu_check_berkeley_db_minor != -1) {
    if (minor < $apu_check_berkeley_db_minor)
      exit (1);
    if (minor > $apu_check_berkeley_db_minor)
      exit (0);
  }

  if ($apu_check_berkeley_db_patch == -1
      || patch >= $apu_check_berkeley_db_patch)
    exit (0);
  else
    exit (1);
}
      ],
      [apu_try_berkeley_db=yes],
      [apu_try_berkeley_db=no],
      [apu_try_berkeley_db=yes]
    )

    LIBS="$apu_try_berkeley_db_save_libs"
  ]
)


dnl
dnl APU_CHECK_DB1: is DB1 present?
dnl
dnl if present: sets apu_db_header, apu_db_lib, and apu_db_version
dnl
AC_DEFUN([APU_CHECK_DB1], [
  places=$1
  if test -z "$places"; then
    places="std"
  fi
  APU_CHECK_BERKELEY_DB(1, 0, 0,
    "$places",
    "db1/db.h db.h",
    "db1"
  )
  if test "$apu_have_db" = "1"; then
    apu_db_version=1
  fi
])


dnl
dnl APU_CHECK_DB185: is DB1.85 present?
dnl
dnl if present: sets apu_db_header, apu_db_lib, and apu_db_version
dnl
dnl NB: BerkelyDB v2 and above can be compiled in 1.85 mode
dnl which has a libdb not libdb1 or libdb185
AC_DEFUN([APU_CHECK_DB185], [
  places=$1
  if test -z "$places"; then
    places="std"
  fi
  APU_CHECK_BERKELEY_DB(1, -1, -1,
    "$places",
    "db_185.h",
    "db"
  )
  if test "$apu_have_db" = "1"; then
    apu_db_version=185
  fi
])


dnl
dnl APU_CHECK_DB2: is DB2 present?
dnl
dnl if present: sets apu_db_header, apu_db_lib, and apu_db_version
dnl
AC_DEFUN([APU_CHECK_DB2], [
  places=$1
  if test -z "$places"; then
    places="std"
  fi
  APU_CHECK_BERKELEY_DB(2, -1, -1,
    "$places",
    "db2/db.h db.h",
    "db2 db"
  )
  if test "$apu_have_db" = "1"; then
    apu_db_version=2
  fi
])


dnl
dnl APU_CHECK_DB3: is DB3 present?
dnl
dnl if present: sets apu_db_header, apu_db_lib, and apu_db_version
dnl
AC_DEFUN([APU_CHECK_DB3], [
  places=$1
  if test -z "$places"; then
    places="std"
  fi
  APU_CHECK_BERKELEY_DB(3, -1, -1,
    "$places",
    "db3/db.h db.h",
    "db3 db"
  )
  if test "$apu_have_db" = "1"; then
    apu_db_version=3
  fi
])


dnl
dnl APU_CHECK_DB4: is DB4 present?
dnl
dnl if present: sets apu_db_header, apu_db_lib, and apu_db_version
dnl
AC_DEFUN([APU_CHECK_DB4], [
  places=$1
  if test -z "$places"; then
    places="std /usr/local /usr/local/BerkeleyDB.4.0 /boot/home/config"
  fi
  APU_CHECK_BERKELEY_DB("4", "0", "-1",
    "$places",
    "db4/db.h db.h",
    "db-4.0 db4 db"
  )
  if test "$apu_have_db" = "1"; then
    apu_db_version=4
  fi
])


dnl
dnl APU_CHECK_DB41: is DB4.1 present?
dnl
dnl if present: sets apu_db_header, apu_db_lib, and apu_db_version
dnl
AC_DEFUN([APU_CHECK_DB41], [
  places=$1
  if test -z "$places"; then
    places="std /usr/local /usr/local/BerkeleyDB.4.1 /boot/home/config"
  fi
  APU_CHECK_BERKELEY_DB("4", "1", "-1",
    "$places",
    "db41/db.h db4/db.h db.h",
    "db-4.1 db41 db4 db"
  )
  if test "$apu_have_db" = "1"; then
    apu_db_version=4
  fi
])


dnl
dnl APU_CHECK_DB42: is DB4.2 present?
dnl
dnl if present: sets apu_db_header, apu_db_lib, and apu_db_version
dnl
AC_DEFUN([APU_CHECK_DB42], [
  places=$1
  if test -z "$places"; then
    places="std /usr/local /usr/local/BerkeleyDB.4.2 /boot/home/config"
  fi
  APU_CHECK_BERKELEY_DB("4", "2", "-1",
    "$places",
    "db42/db.h db4/db.h db.h",
    "db-4.2 db42 db4 db"
  )
  if test "$apu_have_db" = "1"; then
    apu_db_version=4
  fi
])
dnl
dnl APU_CHECK_DB43: is DB4.3 present?
dnl
dnl if present: sets apu_db_header, apu_db_lib, and apu_db_version
dnl
AC_DEFUN([APU_CHECK_DB43], [
  places=$1
  if test -z "$places"; then
    places="std /usr/local/BerkeleyDB.4.3 /boot/home/config"
  fi
  APU_CHECK_BERKELEY_DB("4", "3", "-1",
    "$places",
    "db43/db.h db4/db.h db.h",
    "db-4.3 db4-4.3 db43 db4 db"
  )
  if test "$apu_have_db" = "1"; then
    apu_db_version=4
  fi
])
dnl
dnl APU_CHECK_DB44: is DB4.4 present?
dnl
dnl if present: sets apu_db_header, apu_db_lib, and apu_db_version
dnl
AC_DEFUN([APU_CHECK_DB44], [
  places=$1
  if test -z "$places"; then
    places="std /usr/local/BerkeleyDB.4.4 /boot/home/config"
  fi
  APU_CHECK_BERKELEY_DB("4", "4", "-1",
    "$places",
    "db44/db.h db4/db.h db.h",
    "db-4.4 db4-4.4 db44 db4 db"
  )
  if test "$apu_have_db" = "1"; then
    apu_db_version=4
  fi
])
dnl
dnl APU_CHECK_DB45: is DB4.5 present?
dnl
dnl if present: sets apu_db_header, apu_db_lib, and apu_db_version
dnl
AC_DEFUN([APU_CHECK_DB45], [
  places=$1
  if test -z "$places"; then
    places="std /usr/local/BerkeleyDB.4.5 /boot/home/config"
  fi
  APU_CHECK_BERKELEY_DB("4", "5", "-1",
    "$places",
    "db45/db.h db4/db.h db.h",
    "db-4.5 db4-4.5 db45 db4 db"
  )
  if test "$apu_have_db" = "1"; then
    apu_db_version=4
  fi
])

AC_DEFUN([APU_CHECK_DB], [
  requested=$1
  check_places=$2

  case "$requested" in
  db)
    APU_CHECK_DB_ALL("$check_places")
    if test "$apu_have_db" = "0"; then
      AC_MSG_ERROR(Berkeley db requested, but not found)
    fi
    ;;
  db1)
    APU_CHECK_DB1("$check_places")
    if test "$apu_db_version" != "1"; then
      AC_MSG_ERROR(Berkeley db1 not found)
    fi
    ;;
  db185)
    APU_CHECK_DB185("$check_places")
    if test "$apu_db_version" != "185"; then
      AC_MSG_ERROR(Berkeley db185 not found)
    fi
    ;;
  db2)
    APU_CHECK_DB2("$check_places")
    if test "$apu_db_version" != "2"; then
      AC_MSG_ERROR(Berkeley db2 not found)
    fi
    ;;
  db3)
    APU_CHECK_DB3("$check_places")
    if test "$apu_db_version" != "3"; then
      AC_MSG_ERROR(Berkeley db3 not found)
    fi
    ;;
  db4)
    APU_CHECK_DB4("$check_places")
    if test "$apu_db_version" != "4"; then
      AC_MSG_ERROR(Berkeley db4 not found)
    fi
    ;;
  db41)
    APU_CHECK_DB41("$check_places")
    if test "$apu_db_version" != "4"; then
      AC_MSG_ERROR(Berkeley db4 not found)
    fi
    ;;
  db42)
    APU_CHECK_DB42("$check_places")
    if test "$apu_db_version" != "4"; then
      AC_MSG_ERROR(Berkeley db4 not found)
    fi
    ;;
  db43)
    APU_CHECK_DB43("$check_places")
    if test "$apu_db_version" != "4"; then
      AC_MSG_ERROR(Berkeley db4 not found)
    fi
    ;;
  db44)
    APU_CHECK_DB44("$check_places")
    if test "$apu_db_version" != "4"; then
      AC_MSG_ERROR(Berkeley db4 not found)
    fi
    ;;
  db45)
    APU_CHECK_DB44("$check_places")
    if test "$apu_db_version" != "4"; then
      AC_MSG_ERROR(Berkeley db4 not found)
    fi
    ;;
  default)
    APU_CHECK_DB_ALL("$check_places")
    ;;
  esac
])

dnl
dnl APU_CHECK_DB_ALL: Try all Berkeley DB versions, from 4.3 to 1.
dnl
AC_DEFUN([APU_CHECK_DB_ALL], [
  all_places=$1
 
  APU_CHECK_DB45("$all_places")
  if test "$apu_db_version" != "4"; then
    APU_CHECK_DB44("$all_places")
    if test "$apu_db_version" != "4"; then
      APU_CHECK_DB43("$all_places")
      if test "$apu_db_version" != "4"; then
        APU_CHECK_DB42("$all_places")
        if test "$apu_db_version" != "4"; then
          APU_CHECK_DB41("$all_places")
          if test "$apu_db_version" != "4"; then
            APU_CHECK_DB4("$all_places")
            if test "$apu_db_version" != "4"; then
              APU_CHECK_DB3("$all_places")
              if test "$apu_db_version" != "3"; then
                APU_CHECK_DB2("$all_places")
                if test "$apu_db_version" != "2"; then
                  APU_CHECK_DB1("$all_places")
                  if test "$apu_db_version" != "1"; then
                    APU_CHECK_DB185("$all_places")
                  fi
                fi
              fi
            fi
          fi
        fi
      fi
    fi
  fi
  AC_MSG_CHECKING(for Berkeley DB)
  if test "$apu_have_db" = "1"; then
    AC_MSG_RESULT(found db$apu_db_version)
  else
    AC_MSG_RESULT(not found)
  fi
])


dnl
dnl APU_CHECK_DBM: see what kind of DBM backend to use for apr_dbm.
dnl
AC_DEFUN([APU_CHECK_DBM], [
  apu_use_sdbm=0
  apu_use_ndbm=0
  apu_use_gdbm=0
  apu_use_db=0
  dnl it's in our codebase
  apu_have_sdbm=1
  apu_have_gdbm=0
  apu_have_ndbm=0
  apu_have_db=0

  apu_db_header=db.h                # default so apu_select_dbm.h is syntactically correct
  apu_db_version=0

  AC_ARG_WITH(dbm, [
    --with-dbm=DBM          choose the DBM type to use.
      DBM={sdbm,gdbm,ndbm,db,db1,db185,db2,db3,db4,db41,db42,db43,db44,db45}
  ], [
    if test "$withval" = "yes"; then
      AC_MSG_ERROR([--with-dbm needs to specify a DBM type to use.
        One of: sdbm, gdbm, ndbm, db, db1, db185, db2, db3, db4, db41, db42, db43, db44, db45])
    fi
    requested="$withval"
  ], [
    requested=default
  ])

  dnl We don't pull in GDBM unless the user asks for it, since it's GPL
  AC_ARG_WITH([gdbm], [
    --with-gdbm=DIR          specify GDBM location
  ], [
    apu_have_gdbm=0
    if test "$withval" = "yes"; then
      AC_CHECK_HEADER(gdbm.h, AC_CHECK_LIB(gdbm, gdbm_open, [apu_have_gdbm=1]))
    elif test "$withval" = "no"; then
      apu_have_gdbm=0
    else
      CPPFLAGS="-I$withval/include"
      LIBS="-L$withval/lib "

      AC_MSG_CHECKING(checking for gdbm in $withval)
      AC_CHECK_HEADER(gdbm.h, AC_CHECK_LIB(gdbm, gdbm_open, [apu_have_gdbm=1]))
      if test "$apu_have_gdbm" != "0"; then
        APR_ADDTO(APRUTIL_LDFLAGS, [-L$withval/lib])
        APR_ADDTO(APRUTIL_INCLUDES, [-I$withval/include])
      fi
    fi
  ])

  AC_ARG_WITH([ndbm], [
    --with-ndbm=PATH 
      Find the NDBM header and library in \`PATH/include' and 
      \`PATH/lib'.  If PATH is of the form \`HEADER:LIB', then search 
      for header files in HEADER, and the library in LIB.  If you omit
      the \`=PATH' part completely, the configure script will search
      for NDBM in a number of standard places.
  ], [
    apu_have_ndbm=0
    if test "$withval" = "yes"; then
      AC_MSG_CHECKING(checking for ndbm in the usual places)
      apu_want_ndbm=1
      NDBM_INC=""
      NDBM_LDFLAGS=""
    elif test "$withval" = "no"; then
      apu_want_ndbm=0
    else
      apu_want_ndbm=1
      case "$withval" in
        *":"*)
          NDBM_INC="-I`echo $withval |sed -e 's/:.*$//'`"
          NDBM_LDFLAGS="-L`echo $withval |sed -e 's/^.*://'`"
          AC_MSG_CHECKING(checking for ndbm includes with $NDBM_INC libs with $NDBM_LDFLAGS )
        ;;
        *)
          NDBM_INC="-I$withval/include"
          NDBM_LDFLAGS="-L$withval/lib"
          AC_MSG_CHECKING(checking for ndbm includes in $withval)
        ;;
      esac
    fi

    save_cppflags="$CPPFLAGS"
    save_ldflags="$LDFLAGS"
    CPPFLAGS="$CPPFLAGS $NDBM_INC"
    LDFLAGS="$LDFLAGS $NDBM_LDFLAGS"
    dnl db_ndbm_open is what sleepcat's compatibility library actually has in it's lib
    if test "$apu_want_ndbm" != "0"; then
      AC_CHECK_HEADER(ndbm.h, 
        AC_CHECK_LIB(c, dbm_open, [apu_have_ndbm=1;apu_ndbm_lib=c],
          AC_CHECK_LIB(dbm, dbm_open, [apu_have_ndbm=1;apu_ndbm_lib=dbm],
            AC_CHECK_LIB(db, dbm_open, [apu_have_ndbm=1;apu_ndbm_lib=db],
              AC_CHECK_LIB(db, __db_ndbm_open, [apu_have_ndbm=1;apu_ndbm_lib=db])
            )
          )
        )
      )
      if test "$apu_have_ndbm" != "0";  then
        if test "$withval" != "yes"; then
          APR_ADDTO(APRUTIL_INCLUDES, [$NDBM_INC])
          APR_ADDTO(APRUTIL_LDFLAGS, [$NDBM_LDFLAGS])
        fi
      elif test "$withval" != "yes"; then
        AC_ERROR( NDBM not found in the specified directory)
      fi
    fi
    CPPFLAGS="$save_cppflags"
    LDFLAGS="$save_ldflags"
  ], [
    dnl don't check it no one has asked us for it
    apu_have_ndbm=0
  ])


  if test -n "$apu_db_xtra_libs"; then
    saveddbxtralibs="$LIBS"
    LIBS="$apu_db_xtra_libs $LIBS"
  fi

  dnl We're going to try to find the highest version of Berkeley DB supported.
  dnl
  dnl Note that we only do this if the user requested it, since the Sleepycat
  dnl license is viral and requires distribution of source along with programs
  dnl that use it.
  AC_ARG_WITH([berkeley-db], [
    --with-berkeley-db=PATH
      Find the Berkeley DB header and library in \`PATH/include' and
      \`PATH/lib'.  If PATH is of the form \`HEADER:LIB', then search
      for header files in HEADER, and the library in LIB.  If you omit
      the \`=PATH' part completely, the configure script will search
      for Berkeley DB in a number of standard places.
  ], [
    if test "$withval" = "yes"; then
      apu_want_db=1
      user_places=""
    elif test "$withval" = "no"; then
      apu_want_db=0
    else
      apu_want_db=1
      user_places="$withval"
    fi

    if test "$apu_want_db" != "0"; then
      APU_CHECK_DB($requested, $user_places)
      if test "$apu_have_db" = "0"; then
        AC_ERROR(Berkeley DB not found.)
      fi
    fi 
  ])

  if test -n "$apu_db_xtra_libs"; then
    LIBS="$saveddbxtralibs"
  fi

  case "$requested" in
    sdbm)
      apu_use_sdbm=1
      apu_default_dbm=sdbm
      ;;
    gdbm)
      apu_use_gdbm=1
      apu_default_dbm=gdbm
      ;;
    ndbm)
      apu_use_ndbm=1
      apu_default_dbm=ndbm
      ;;
    db)
      apu_use_db=1
      apu_default_dbm=db
      ;;
    db1)
      apu_use_db=1
      apu_default_dbm=db1
      ;;
    db185)
      apu_use_db=1
      apu_default_dbm=db185
      ;;
    db2)
      apu_use_db=1
      apu_default_dbm=db2
      ;;
    db3)
      apu_use_db=1
      apu_default_dbm=db3
      ;;
    db4)
      apu_use_db=1
      apu_default_dbm=db4
      ;;
    db41)
      apu_use_db=1
      apu_default_dbm=db4
      ;;
    db42)
      apu_use_db=1
      apu_default_dbm=db4
      ;;
    db43)
      apu_use_db=1
      apu_default_dbm=db4
      ;;
    db44)
      apu_use_db=1
      apu_default_dbm=db4
      ;;
    db45)
      apu_use_db=1
      apu_default_dbm=db4
      ;;
    default)
      dnl ### use more sophisticated DBMs for the default?
      apu_default_dbm="sdbm (default)"
      apu_use_sdbm=1
      ;;
    *)
      AC_MSG_ERROR([--with-dbm=$look_for is an unknown DBM type.
        Use one of: sdbm, gdbm, ndbm, db, db1, db185, db2, db3, db4, db41, db42, db43, db44 db45])
      ;;
  esac

  dnl Yes, it'd be nice if we could collate the output in an order
  dnl so that the AC_MSG_CHECKING would be output before the actual
  dnl checks, but it isn't happening now.
  AC_MSG_CHECKING(for default DBM)
  AC_MSG_RESULT($apu_default_dbm)

  AC_SUBST(apu_use_sdbm)
  AC_SUBST(apu_use_gdbm)
  AC_SUBST(apu_use_ndbm)
  AC_SUBST(apu_use_db)

  AC_SUBST(apu_have_sdbm)
  AC_SUBST(apu_have_gdbm)
  AC_SUBST(apu_have_ndbm)
  AC_SUBST(apu_have_db)
  AC_SUBST(apu_db_header)
  AC_SUBST(apu_db_version)

  dnl Since we have already done the AC_CHECK_LIB tests, if we have it, 
  dnl we know the library is there.
  if test "$apu_have_gdbm" = "1"; then
    APR_ADDTO(APRUTIL_EXPORT_LIBS,[-lgdbm])
    APR_ADDTO(APRUTIL_LIBS,[-lgdbm])
  fi

  if test "$apu_have_ndbm" = "1"; then
    APR_ADDTO(APRUTIL_EXPORT_LIBS,[-l$apu_ndbm_lib])
    APR_ADDTO(APRUTIL_LIBS,[-l$apu_ndbm_lib])
  fi

  if test "$apu_have_db" = "1"; then
    APR_ADDTO(APRUTIL_EXPORT_LIBS,[-l$apu_db_lib])
    APR_ADDTO(APRUTIL_LIBS,[-l$apu_db_lib])
    if test -n "apu_db_xtra_libs"; then
      APR_ADDTO(APRUTIL_EXPORT_LIBS,[$apu_db_xtra_libs])
      APR_ADDTO(APRUTIL_LIBS,[$apu_db_xtra_libs])
    fi
  fi
])

