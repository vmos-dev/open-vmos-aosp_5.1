#!/bin/bash
# See usage() below for the description.

function usage() {
  cat <<EOF
# This script copies the .jar files that each plugin depends on into the plugins libs folder.
# By default, on Mac & Linux, this script creates symlinks from the libs folder to the jar file.
# Since Windows does not support symlinks, the jar files are copied.
#
# Options:
# -f : to copy files rather than creating symlinks on the Mac/Linux platforms.
# -d : print make dependencies instead of running make; doesn't copy files.
# -c : copy files expected after make dependencies (reported by -d) have been built.
#
# The purpose of -d/-c is to include the workflow in a make file:
# - the make rule should depend on \$(shell create_all_symlinks -d)
# - the rule body should perform   \$(shell create_all_symlinks -c [-f])
EOF
}

# CD to the top android directory
PROG_DIR=`dirname "$0"`
cd "${PROG_DIR}/../../../"

HOST=`uname`
USE_COPY=""        # force copy dependent jar files rather than creating symlinks
ONLY_SHOW_DEPS=""  # only report make dependencies but don't build them nor copy.
ONLY_COPY_DEPS=""  # only copy dependencies built by make; uses -f as needed.

function die() {
  echo "Error: $*" >/dev/stderr
  exit 1
}

function warn() {
  # Only print something if not in show-deps mode
  if [[ -z $ONLY_SHOW_DEPS ]]; then
    echo "$*"
  fi
}

function printGradleJarPath() {
  # Prints to stdout the absolute path of the JAR assembled for a given gradle project.
  # $1 = source dir, e.g. tools/base or tools/swt
  # $2 = the gradle project name e.g. common or lint-api
  echo "## Quering Gradle properties for '$2' in '$1'." > /dev/stderr
  ( cd $1 && \
    ./gradlew :$2:properties | awk '
        BEGIN { B=""; N=""; V="" }
        /^archivesBaseName:/ { N=$2 }
        /^buildDir:/         { D=$2 }
        /^version:/          { V=$2 }
        END { print D "/libs/" N "-" V ".jar" }' )
}

## parse arguments
while [ $# -gt 0 ]; do
  case "$1" in
    "-f" )
      USE_COPY="1"
      ;;
    "-d" )
      ONLY_SHOW_DEPS="1"
      ;;
    "-c" )
      ONLY_COPY_DEPS="1"
      ;;
    * )
      usage
      exit 2
  esac
  shift
done

warn "## Running $0"

if [[ "${HOST:0:6}" == "CYGWIN" || "$USE_MINGW" == "1" ]]; then
  # This is either Cygwin or Linux/Mingw cross-compiling to Windows.
  PLATFORM="windows-x86"
  if [[ "${HOST:0:6}" == "CYGWIN" ]]; then
    # We can't use symlinks under Cygwin
    USE_COPY="1"
  fi
elif [[ "$HOST" == "Linux" ]]; then
  PLATFORM="linux-x86"
elif [[ "$HOST" == "Darwin" ]]; then
  PLATFORM="darwin-x86"
else
  die "Unsupported platform ($HOST). Aborting."
fi

if [[ "$USE_COPY" == "1" ]]; then
  function cpfile { # $1=source $2=dest $3=optional dest filename
    cp -fv $1 $2/$3
  }
else
  # computes the "reverse" path, e.g. "a/b/c" => "../../.."
  function back() {
    echo $1 | sed 's@[^/]*@..@g'
  }

  function cpfile { # $1=source $2=dest $3=optional dest filename
    local src=$1
    if [[ "${src:0:1}" != "/" ]]; then
      # Not an absolute path. We assume a relative path to be
      # relative to the android root and we want to make it
      # relative to the destination dir.
      src=$(back $2)/$1
    fi
    ln -svf $src $2/$3
  }
fi

DEST="sdk/eclipse/scripts"

set -e # fail early
LIBS=""
CP_FILES=""


### Configure which libs to build.
#
# Each entry for LIBS needs to be prefixed with the way we want to build it:
# make: - a library built using its traditional Android.mk
# base: - a gradle library located in tools/base
# swt:  - a gradle library located in toosl/swt
#
# LIBS entries without or with an unknown ":" prefix will generate an error.

### BASE ###

BASE_PLUGIN_DEST="sdk/eclipse/plugins/com.android.ide.eclipse.base/libs"
BASE_PLUGIN_LIBS="base:common swt:sdkstats base:sdklib base:dvlib base:layoutlib-api base:sdk-common"
BASE_PLUGIN_PREBUILTS="\
    prebuilts/tools/common/m2/repository/net/sf/kxml/kxml2/2.3.0/kxml2-2.3.0.jar \
    prebuilts/tools/common/m2/repository/org/apache/commons/commons-compress/1.0/commons-compress-1.0.jar \
    prebuilts/tools/common/m2/repository/com/google/guava/guava/15.0/guava-15.0.jar \
    prebuilts/tools/common/m2/repository/commons-logging/commons-logging/1.1.1/commons-logging-1.1.1.jar \
    prebuilts/tools/common/m2/repository/commons-codec/commons-codec/1.4/commons-codec-1.4.jar \
    prebuilts/tools/common/m2/repository/org/apache/httpcomponents/httpclient/4.1.1/httpclient-4.1.1.jar \
    prebuilts/tools/common/m2/repository/org/apache/httpcomponents/httpcore/4.1/httpcore-4.1.jar \
    prebuilts/tools/common/m2/repository/org/apache/httpcomponents/httpmime/4.1/httpmime-4.1.jar"

# temporarily disabled
#    prebuilts/tools/common/m2/repository/org/bouncycastle/bcpkix-jdk15on/1.48/bcpkix-jdk15on-1.48.jar \
#    prebuilts/tools/common/m2/repository/org/bouncycastle/bcprov-jdk15on/1.48/bcprov-jdk15on-1.48.jar"

LIBS="$LIBS $BASE_PLUGIN_LIBS"
CP_FILES="$CP_FILES @:$BASE_PLUGIN_DEST $BASE_PLUGIN_LIBS $BASE_PLUGIN_PREBUILTS"


### ADT ###

ADT_DEST="sdk/eclipse/plugins/com.android.ide.eclipse.adt/libs"
ADT_LIBS="make:ant-glob base:asset-studio base:lint-api base:lint-checks base:ninepatch make:propertysheet \
          base:rule-api swt:sdkuilib swt:swtmenubar base:manifest-merger"
ADT_PREBUILTS="\
    prebuilts/tools/common/freemarker/freemarker-2.3.19.jar \
    prebuilts/tools/common/m2/repository/org/ow2/asm/asm/4.0/asm-4.0.jar \
    prebuilts/tools/common/m2/repository/org/ow2/asm/asm-tree/4.0/asm-tree-4.0.jar \
    prebuilts/tools/common/m2/repository/org/ow2/asm/asm-analysis/4.0/asm-analysis-4.0.jar \
    prebuilts/tools/common/m2/repository/com/android/tools/external/lombok/lombok-ast/0.2.2/lombok-ast-0.2.2.jar"

LIBS="$LIBS $ADT_LIBS"
CP_FILES="$CP_FILES @:$ADT_DEST $ADT_LIBS $ADT_PREBUILTS"


### DDMS ###

DDMS_DEST="sdk/eclipse/plugins/com.android.ide.eclipse.ddms/libs"
DDMS_LIBS="base:ddmlib swt:ddmuilib swt:swtmenubar swt:uiautomatorviewer"

DDMS_PREBUILTS="\
    prebuilts/tools/common/m2/repository/jfree/jcommon/1.0.12/jcommon-1.0.12.jar \
    prebuilts/tools/common/m2/repository/jfree/jfreechart/1.0.9/jfreechart-1.0.9.jar \
    prebuilts/tools/common/m2/repository/jfree/jfreechart-swt/1.0.9/jfreechart-swt-1.0.9.jar"

LIBS="$LIBS $DDMS_LIBS"
CP_FILES="$CP_FILES @:$DDMS_DEST $DDMS_LIBS $DDMS_PREBUILTS"


### TEST ###

TEST_DEST="sdk/eclipse/plugins/com.android.ide.eclipse.tests"
TEST_LIBS="make:easymock base:testutils"
TEST_PREBUILTS="prebuilts/tools/common/m2/repository/net/sf/kxml/kxml2/2.3.0/kxml2-2.3.0.jar"

LIBS="$LIBS $TEST_LIBS"
CP_FILES="$CP_FILES @:$TEST_DEST $TEST_LIBS $TEST_PREBUILTS"


### BRIDGE ###

if [[ $PLATFORM != "windows-x86" ]]; then
  # We can't build enough of the platform on Cygwin to create layoutlib
  BRIDGE_LIBS="make:layoutlib base:ninepatch"

  LIBS="$LIBS $BRIDGE_LIBS"
fi


### HIERARCHYVIEWER ###

HV_DEST="sdk/eclipse/plugins/com.android.ide.eclipse.hierarchyviewer/libs"
HV_LIBS="swt:hierarchyviewer2lib swt:swtmenubar"

LIBS="$LIBS $HV_LIBS"
CP_FILES="$CP_FILES @:$HV_DEST $HV_LIBS"


### TRACEVIEW ###

TV_DEST="sdk/eclipse/plugins/com.android.ide.eclipse.traceview/libs"
TV_LIBS="swt:traceview"

LIBS="$LIBS $TV_LIBS"
CP_FILES="$CP_FILES @:$TV_DEST $TV_LIBS"


### MONITOR ###

MONITOR_DEST="sdk/eclipse/plugins/com.android.ide.eclipse.monitor/libs"
MONITOR_LIBS="swt:sdkuilib"

LIBS="$LIBS $MONITOR_LIBS"
CP_FILES="$CP_FILES @:$MONITOR_DEST $MONITOR_LIBS"


### SDKMANAGER ###

SDKMAN_LIBS="swt:swtmenubar"

LIBS="$LIBS $SDKMAN_LIBS"


### GL DEBUGGER ###

if [[ $PLATFORM != "windows-x86" ]]; then
  # liblzf doesn't build under cygwin. If necessary, this should be fixed first.

  GLD_DEST="sdk/eclipse/plugins/com.android.ide.eclipse.gldebugger/libs"
  GLD_LIBS="make:host-libprotobuf-java-2.3.0-lite make:liblzf"

  LIBS="$LIBS $GLD_LIBS"
  CP_FILES="$CP_FILES @:$GLD_DEST $GLD_LIBS"
fi


#--- Determine what to build

UNPROCESSED=""
GRADLE_SWT=""
GRADLE_BASE=""
MAKE_TARGETS=""
for LIB in $LIBS; do
  if [[ "${LIB:0:5}" == "base:" ]]; then
    GRADLE_BASE="$GRADLE_BASE :${LIB:5}:assemble"
  elif [[ "${LIB:0:4}" == "swt:" ]]; then
    GRADLE_SWT="$GRADLE_SWT :${LIB:4}:assemble"
  elif [[ "${LIB:0:5}" == "make:" ]]; then
    MAKE_TARGETS="$MAKE_TARGETS ${LIB:5}"
  else
    UNPROCESSED="$UNPROCESSED $LIB"
  fi
done

unset LIBS   # we shouldn't use this anymore, it has been split up just above.


if [[ -n $UNPROCESSED ]]; then
  die "## The following libs lack a prefix (make:, base: or swt:): $UNPROCESSED"
fi

# In the mode to only echo dependencies, output them and we're done
if [[ -n $ONLY_SHOW_DEPS ]]; then
  echo $MAKE_TARGETS
  exit 0
fi

# --- Gradle Build ---

# tools/base: if we need it for SWT, we build them all and public local.
# Otherwise we do a specific tools/base build on just the requested targets.

if [[ -n "$GRADLE_SWT" ]]; then
  echo "### Starting tools/base: gradlew assemble publishLocal"
  (cd tools/base && ./gradlew assemble publishLocal)
elif [[ -n "$GRADLE_BASE" ]]; then
  echo "### Starting tools/base: gradlew $GRADLE_BASE"
  (cd tools/base && ./gradlew $GRADLE_BASE)
fi

# tools/swt: build requested targets

if [[ -n "$GRADLE_SWT" ]]; then
  echo "### Starting tools/swt: gradlew $GRADLE_SWT"
  (cd tools/swt && ./gradlew $GRADLE_SWT)
fi

# --- Android.mk Build ---

# If some of the libs are available in prebuilts/devtools, use link to them directly
# instead of trying to rebuild them so remove them from the libs to build. Note that
# they are already listed in CP_FILES so we'll adjust the source to copy later.

NEW_TARGETS=""
for LIB in $MAKE_TARGETS; do
  J="prebuilts/devtools/tools/lib/$LIB.jar"
  if [[ ! -f $J ]]; then
    J="prebuilts/devtools/adt/lib/$LIB.jar"
  fi
  if [[ -f $J ]]; then
    warn "## Using existing $J"
  else
    NEW_TARGETS="$NEW_TARGETS $LIB"
  fi
done
MAKE_TARGETS="$NEW_TARGETS"
unset NEW_TARGETS

if [[ -z $ONLY_COPY_DEPS ]]; then
  if [[ -n $MAKE_TARGETS ]]; then
    ( # Make sure we have lunch sdk-<something>
      if [[ ! "$TARGET_PRODUCT" ]]; then
        warn "## TARGET_PRODUCT is not set, running build/envsetup.sh"
        . build/envsetup.sh
        warn "## lunch sdk-eng"
        lunch sdk-eng
      fi

      J="4"
      [[ $(uname) == "Darwin" ]] && J=$(sysctl hw.ncpu | cut -d : -f 2 | tr -d ' ')
      [[ $(uname) == "Linux"  ]] && J=$(cat /proc/cpuinfo | grep processor | wc -l)

      warn "## Building libs: make -j$J $MAKE_TARGETS"
      make -j${J} $MAKE_TARGETS
    )
  fi
fi

# --- Copy resulting files ---

DEST=""
for SRC in $CP_FILES; do
  if [[ "${SRC:0:2}" == "@:" ]]; then
    DEST="${SRC:2}"
    mkdir -vp "$DEST"
    continue
  fi

  ORIG_SRC="$SRC"
  DEST_FILE=""

  if [[ "${SRC:0:5}" == "base:" ]]; then
    SRC="${SRC:5}"
    ORIG_SRC="$SRC"
    DEST_FILE="$SRC.jar"
    SRC=$(printGradleJarPath tools/base $SRC)
  elif [[ "${SRC:0:4}" == "swt:" ]]; then
    SRC="${SRC:4}"
    ORIG_SRC="$SRC"
    DEST_FILE="$SRC.jar"
    SRC=$(printGradleJarPath tools/swt $SRC)
  elif [[ "${SRC:0:5}" == "make:" ]]; then
    SRC="${SRC:5}"
    ORIG_SRC="$SRC"
  fi

  if [[ ! -f "$SRC" ]]; then
    # Take a prebuilts/devtools instead of a framework one if possible.
    SRC="prebuilts/devtools/tools/lib/$SRC.jar"
    if [[ ! -f "$SRC" ]]; then
      SRC="prebuilts/devtools/adt/lib/$ORIG_SRC.jar"
    fi
    if [[ ! -f "$SRC" ]]; then
      SRC="out/host/$PLATFORM/framework/$ORIG_SRC.jar"
    fi
  fi
  if [[ -f "$SRC" ]]; then
    if [[ ! -d "$DEST" ]]; then
      die "Invalid cp_file dest directory: $DEST"
    fi

    cpfile "$SRC" "$DEST" "$DEST_FILE"
  else
    die "## Unknown source '$ORIG_SRC' to copy in '$DEST'"
  fi
done

# OS-specific post operations

if [ "${HOST:0:6}" == "CYGWIN" ]; then
  chmod -v a+rx "$ADT_DEST"/*.jar
fi

echo "### $0 done"
