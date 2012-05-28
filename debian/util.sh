#!/bin/bash
##### -*- mode:shell-script; indent-tabs-mode:nil; sh-basic-offset:2 -*-
##### Author: Travis Cross <tc@traviscross.com>

set -e

ddir="."
[ -n "${0%/*}" ] && ddir="${0%/*}"
cd $ddir/../

#### lib

err () {
  echo "$0 error: $1" >&2
  exit 1
}

announce () {
  cat >&2 <<EOF

########################################################################
## $1
########################################################################

EOF
}

xread () {
  local xIFS="$IFS"
  IFS=''
  read $@
  local ret=$?
  IFS="$xIFS"
  return $ret
}

mk_dver () { echo "$1" | sed -e 's/-/~/g'; }
mk_uver () { echo "$1" | sed -e 's/-.*$//' -e 's/~/-/'; }
dsc_source () { dpkg-parsechangelog | grep '^Source:' | awk '{print $2}'; }
dsc_ver () { dpkg-parsechangelog | grep '^Version:' | awk '{print $2}'; }
up_ver () { mk_uver "$(dsc_ver)"; }
dsc_base () { echo "$(dsc_source)_$(dsc_ver)"; }
up_base () { echo "$(dsc_source)-$(up_ver)"; }

find_distro () {
  case "$1" in
    experimental) echo "sid";;
    unstable) echo "sid";;
    testing) echo "wheezy";;
    stable) echo "squeeze";;
    *) echo "$1";;
  esac
}

find_suite () {
  case "$1" in
    sid) echo "unstable";;
    wheezy) echo "testing";;
    squeeze) echo "stable";;
    *) echo "$1";;
  esac
}

#### debian/rules helpers

create_dbg_pkgs () {
  for x in $ddir/*; do
    test ! -d $x && continue
    test "$x" = "tmp" -o "$x" = "source" && continue
    test ! "$x" = "${x%-dbg}" && continue
    test ! -d $x/usr/lib/debug && continue
    mkdir -p $x-dbg/usr/lib
    mv $x/usr/lib/debug $x-dbg/usr/lib/
  done
}

cwget () {
  local url="$1" f="${1##*/}"
  echo "fetching: $url to $f" >&2
  if [ -n "$FS_FILES_DIR" ]; then
    if ! [ -s "$FS_FILES_DIR/$f" ]; then
      (cd $FS_FILES_DIR && wget -N "$url")
    fi
    cp -a $FS_FILES_DIR/$f .
  else
    wget -N "$url"
  fi
}

getlib () {
  local url="$1" f="${1##*/}"
  cwget "$url"
  tar -xv --no-same-owner --no-same-permissions -f "$f"
  rm -f "$f" && mkdir -p $f && touch $f/.download-stamp
}

getlibs () {
  # get pinned libraries
  getlib http://downloads.mongodb.org/cxx-driver/mongodb-linux-x86_64-v1.8-latest.tgz
  getlib http://files.freeswitch.org/downloads/libs/json-c-0.9.tar.gz
  getlib http://files.freeswitch.org/downloads/libs/libmemcached-0.32.tar.gz
  getlib http://files.freeswitch.org/downloads/libs/soundtouch-1.6.0.tar.gz
  getlib http://files.freeswitch.org/downloads/libs/flite-1.5.4-current.tar.bz2
  getlib http://files.freeswitch.org/downloads/libs/sphinxbase-0.7.tar.gz
  getlib http://files.freeswitch.org/downloads/libs/pocketsphinx-0.7.tar.gz
  getlib http://files.freeswitch.org/downloads/libs/communicator_semi_6000_20080321.tar.gz
  getlib http://files.freeswitch.org/downloads/libs/celt-0.10.0.tar.gz
  getlib http://files.freeswitch.org/downloads/libs/opus-0.9.0.tar.gz
  getlib http://files.freeswitch.org/downloads/libs/openldap-2.4.19.tar.gz
  getlib http://download.zeromq.org/zeromq-2.1.9.tar.gz \
    || getlib http://download.zeromq.org/historic/zeromq-2.1.9.tar.gz
  getlib http://files.freeswitch.org/downloads/libs/freeradius-client-1.1.6.tar.gz
  getlib http://files.freeswitch.org/downloads/libs/lame-3.98.4.tar.gz
  getlib http://files.freeswitch.org/downloads/libs/libshout-2.2.2.tar.gz
  getlib http://files.freeswitch.org/downloads/libs/mpg123-1.13.2.tar.gz
  # cleanup mongo
  (
    cd mongo-cxx-driver-v1.8
    rm -rf config.log .sconf_temp *Test *Example
    find . -name "*.o" -exec rm -f {} \;
  )
}

check_repo_clean () {
  git diff-index --quiet --cached HEAD \
    || err "uncommitted changes present"
  git diff-files --quiet \
    || err "unclean working tree"
  git diff-index --quiet HEAD \
    || err "unclean repository"
  ! git ls-files --other --error-unmatch . >/dev/null 2>&1 \
    || err "untracked files or build products present"
}

create_orig () {
  local OPTIND OPTARG
  local uver="" bundle_deps=false zl=9e
  while getopts 'bnv:z:' o "$@"; do
    case "$o" in
      b) bundle_deps=true;;
      n) uver="nightly";;
      v) uver="$OPTARG";;
      z) zl="$OPTARG";;
    esac
  done
  shift $(($OPTIND-1))
  [ -z "$uver" ] || [ "$uver" = "nightly" ] \
    && uver="$(cat build/next-release.txt)-n$(date +%Y%m%dT%H%M%SZ)"
  local treeish="$1" dver="$(mk_dver "$uver")"
  local orig="../freeswitch_$dver.orig.tar.xz"
  [ -n "$treeish" ] || treeish="HEAD"
  check_repo_clean
  git reset --hard "$treeish"
  mv .gitattributes .gitattributes.orig
  grep .gitattributes.orig \
    -e '\bdebian-ignore\b' \
    -e '\bdfsg-nonfree\b' \
    | while xread l; do
    echo "$l export-ignore" >> .gitattributes
  done
  if $bundle_deps; then
    (cd libs && getlibs)
    git add -f libs
  fi
  ./build/set-fs-version.sh "$uver" && git add configure.in
  git commit --allow-empty -m "nightly v$uver"
  git archive -v \
    --worktree-attributes \
    --format=tar \
    --prefix=freeswitch-$uver/ \
    HEAD \
    | xz -c -${zl}v > $orig
  mv .gitattributes.orig .gitattributes
  git reset --hard HEAD^ && git clean -fdx
  echo $orig
}

create_dsc () {
  local distro="$(find_distro $1)" orig="$2"
  local suite="$(find_suite $distro)"
  local orig_ver="$(echo "$orig" | sed -e 's/^.*_//' -e 's/\.orig\.tar.*$//')"
  local dver="${orig_ver}-1~${distro}+1"
  [ -x "$(which dch)" ] \
    || err "package devscripts isn't installed"
  (cd debian && ./bootstrap.sh -c $distro)
  dch -b -m -v "$dver" --force-distribution -D "$suite" "Nightly build."
  git add debian/changelog && git commit -m "nightly v$orig_ver"
  dpkg-source -i.* -Zxz -z9 -b .
  dpkg-genchanges -S > ../$(dsc_base)_source.changes
  local dsc="../$(dsc_base).dsc"
  git reset --hard HEAD^ && git clean -fdx
  echo $dsc
}

build_debs () {
  local distro="$(find_distro $1)" dsc="$2" arch="$3"
  if [ -z "$distro" ] || [ "$distro" = "auto" ]; then
    if ! (echo "$dsc" | grep -e '-[0-9]*~[a-z]*+[0-9]*'); then
      err "no distro specified or found"
    fi
    local x="$(echo $dsc | sed -e 's/^[^-]*-[0-9]*~//' -e 's/+[^+]*$//')"
    distro="$(find_distro $x)"
  fi
  [ -n "$arch" ] || arch="$(dpkg-architecture | grep '^DEB_BUILD_ARCH=' | cut -d'=' -f2)"
  [ -x "$(which cowbuilder)" ] \
    || err "package cowbuilder isn't installed"
  local cow_img=/var/cache/pbuilder/base-$distro-$arch.cow
  cow () {
    cowbuilder "$@" \
      --distribution $distro \
      --architecture $arch \
      --basepath $cow_img
  }
  if ! [ -d $cow_img ]; then
    announce "Creating base $distro-$arch image..."
    cow --create
  fi
  announce "Updating base $distro-$arch image..."
  cow --update
  announce "Building $distro-$arch DEBs from $dsc..."
  cow --build $dsc
  echo ${dsc}_${arch}.changes
}

while getopts 'd' o "$@"; do
  case "$o" in
    d) set -vx;;
  esac
done
shift $(($OPTIND-1))

cmd="$1"
shift
case "$cmd" in
  archive-orig) archive_orig "$@" ;;
  build-debs) build_debs "$@" ;;
  create-dbg-pkgs) create_dbg_pkgs ;;
  create-dsc) create_dsc "$@" ;;
  create-orig) create_orig "$@" ;;
esac

