#!/bin/bash

OLD="$1"
NEW="$2"

# sanity check in input args
if [ -z "$OLD" ] || [ -z "$NEW" ]; then
    cat <<EOF
Usage: $0 <old> <new>
Changes the ADT plugin revision number.
Example:
  cd sdk/eclipse
  scripts/update_version.sh 0.1.2 0.2.3
EOF
    exit 1
fi

# sanity check on current dir
if [ `basename "$PWD"` != "eclipse" ]; then
    echo "Please run this from sdk/eclipse."
    exit 1
fi

# sanity check the new version number
if [[ "$NEW" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
  echo "## Version $NEW: seems valid."
else
  echo "## Version $NEW: does not conform to major.mino.micro format."
  exit 1
fi

function replace() {
  if [[ -f "$1" ]]; then
    echo "### Change $SED_OLD => $SED_NEW in $1"
    if [[ $(uname) == "Linux" ]]; then
      sed -i  "s/$SED_OLD/$SED_NEW/g" "$1"
    else
      # sed on Mac doesn't handle -i the same way as on Linux
      sed -i ""  "s/$SED_OLD/$SED_NEW/g" "$1"
    fi
  fi
}

# ---1--- Change Eclipse's qualified version numbers
# quote dots for regexps
SED_OLD="${OLD//./\.}\.qualifier"
SED_NEW="${NEW//./\.}\.qualifier"

for i in $(grep -rl "$OLD" * | grep -E "\.xml$|\.MF$|\.product$"); do
  if [[ -f "$i" && $(basename "$i") != "build.xml" ]]; then
    replace "$i"
  fi
done

# ---2--- Change unqualified version numbers in specific files
SED_OLD="${OLD//./\.}"
SED_NEW="${NEW//./\.}"
for i in artifacts/*/pom.xml \
         plugins/com.android.ide.eclipse.adt.package/ide.product   \
         plugins/com.android.ide.eclipse.monitor/monitor.product   \
         plugins/com.android.ide.eclipse.monitor/plugin.properties \
         plugins/com.android.ide.eclipse.*/pom.xml \
         features/com.android.ide.eclipse.*/pom.xml \
         features/com.android.ide.eclipse.adt.package/feature.xml ; do
  if grep -qs "$OLD" "$i"; then
    replace "$i"
  fi
done

# do another grep for older version without the qualifier. We don't
# want to replace those automatically as it could be something else.
# Printing out occurence helps find ones to update manually, but exclude
# some known useless files.
echo
echo "#### ----------------"
echo "#### Remaining instances of $OLD"
echo
grep -r "$OLD" * | grep -v -E "/build.xml:|/javaCompiler\.\.\.args:"

