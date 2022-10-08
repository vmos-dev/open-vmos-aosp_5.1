## Building a release version

By default builds use the -SNAPSHOT version.
To run a release build that remove the SNAPSHOT qualifier, run:

gradle --init-script release.gradle <tasks>