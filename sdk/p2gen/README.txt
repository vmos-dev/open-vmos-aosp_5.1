Summary:

To generate p2 artifacts from jars, run:
$ mvn --no-snapshot-updates --offline p2:site \
    -Dmaven.repo.local=../../out/host/maven/localrepo
The folder ${build.directory}/repository contains the resultant
p2 repository. ${build.directory} is defined inside pom.xml

Details:

The Eclipse plugins in $root/sdk/eclipse/plugins depend on a number
of jar files from:
    $root/tools/base
    $root/tools/swt
    $root/prebuilts/tools/common/m2/repository

In earlier versions, a script (create_all_symlinks.sh) knew about
which plugins depended on which jars, and when executed, it would
copy over those jars into the plugin's libs folder. Each plugin
included the jars from its libs folder as part of its classpath,
and the entire libs folder was bundled inside the plugin.

Such a scheme has a number of issues:
 - bundling jars inside libs/ folder inside each plugin is not
   recommended by Eclipse. This causes performance degradation
   at runtime.
 - at build time, the script modifies the source folders to add
   the contents inside libs folder. Ideally, the source folders
   shouldn't be modified during build.
 - the script has to maintain state about which plugin depends
   on which jars.

The approach suggested by Eclipse is to replace the regular jars
with OSGI bundles, and for each plugin to explicitly add a dependency
to the required OSGI bundles. In essence, this makes each of the
jars to be similar to a plugin.

This folder contains scripts that can be used to convert a set
of jars into OSGI bundles using the p2-maven-plugin
(https://github.com/reficio/p2-maven-plugin).

$ mvn --no-snapshot-updates \
      --offline \
      -Dmaven.repo.local=../../out/host/maven/localRepo \
      p2:site

The pom.xml file lists the set of jars to be processed. The
runtime options to Maven include:
 --offline: We don't want Maven to fetch anything from the internet.
   All required dependencies must be checked into git.
 --no-snapshot-updates: If the tools artifacts have a -SNAPSHOT
   in them, Maven will attempt to re-download those artifacts,
   which would fail since we are running in offline mode. This
   option instructs Maven to not attempt to download these
   snapshots and use whatever is available in the local repositories.
 -Dmaven.repo.local=path to the local repository that should be
   used by maven. Without this, it'll use $HOME/.m2. This should
   be initialized with all the necessary artifacts if running in
   offline mode.

Additional considerations for running in offline mode:

When running in online mode, there are 3 sources from which files
are downloaded by Maven:
 1 Maven Central
 2 the repository where the tools/base and tools/swt artifacts are
   generated
 3 the prebuilts/tools/common/m2 repository (this is a subset of 1).
Even though 2 and 3 are available locally, we cannot just use them
in offline mode since Maven treats repositories with file:/// urls
as remote as well. As a result, the only way to run offline is to
first explicitly copy the contents of 2 and 3 into the local repository
(-Dmaven.repo.local) before initiating an offline build.
