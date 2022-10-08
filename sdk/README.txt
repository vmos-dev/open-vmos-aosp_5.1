Some of the SDK tools sources have moved out of the sdk.git project.
They are no longer found here.

Instead they can be found in the tools/base.git and the tools/swt.git projects.
If you need to view/change the source and lack these folders, you can bring
them by using a repo init command such as:

$ repo init -u https://android.googlesource.com/platform/manifest -g all,-notdefault,tools
$ repo sync [-j N]

The libraries that are sourced in tools/base and tools/swt are converted to
prebuilts which are located in prebuilts/devtools. These prebuilts are the
ones being used when doing a "make sdk".


----------
1- I don't build full SDKs but I want to change tool X:
----------

Let's say as an example you want to change lint.
It's now located in tools/base/lint.

To build it from the command-line, you'd use "gradle" as such:

$ cd tools/base
$ ./gradlew lint:build

Output is located in $TOP/out/host/gradle/tools/base/lint/libs/

Some comments/tips:
- Gradle is a build system, a bit like make or ant.
  If you want to know more, visit http://www.gradle.org/

- On Windows with the CMD shell, use ./gradlew.bat.
  For Cygwin, Linux or Mac, use ./gradlew.

- Gradle targets are in the form "project-name:task-name".
  To get a list of possible tasks, try this:  $ ./gradlew lint:tasks

- Generally there are only 2 task names to remember:
  $ ./gradlew lint:assemble ==> builds but do not run tests.
  $ ./gradlew lint:check    ==> runs tests and checks such as findbugs.

- To find the list of project-names you can use with gradle:
  $ ./gradlew projects

The new moved projects are unsurprisingly named like their former "make"
counterparts. They are split between 2 repos:
- tools/swt contains all SWT-dependent projects.
- tools/base contains all other non-SWT projects.

However that means that when you want to modify a project using both repos,
you need an extra step.

For example, the SDK Manager UI is located in /tools/swt/sdkmanager.
However it does depend on /tools/base/sdklib. Let's say you want to
make a change in both sdklib and sdkuilib. Here are the steps:

$ # Edit tools/base/sdklib files.
$ cd tools/base ; ./gradlew sdklib:publishLocal
  => this builds sdklib and "publishes" an sdklib.JAR into a local maven
     repo located in the out/gradle folder. Note that this is just a
     temporary build artifact and is NOT used by "make sdk".

$ # Edit tools/swt/sdkmanager/sdkuilib files to use the changes from sdklib.
$ cd ../../tools/swt ; ./gradlew sdkuilib:assemble
  => this builds sdkuilib by using the local JAR of sdklib that is
     located in the out/gradlew folder.



----------
2- How do I change some tools sources and build a new SDK using these?
----------

Let's say you changed something in tools/base/lint and run "make sdk" from
the top dir. Your changes will NOT be included in the resulting SDK.

That's because the SDK has been changed to only rely on the prebuilts located
in /prebuilts/devtools. There are pros and cons with this approach and we're
not going to discuss them here. Instead we'll focus on what you need to do.

It's fairly simple. Go back to the top dir on your Android tree and run:

$ prebuilts/devtools/update_jars.sh -f
$ make sdk

Now your changes are included in the generated SDK.

What you should know about the update_jars.sh script:
- Without argument, it prints what it would do but does nothing.
  Use the "-f" argument to make it build/copy stuff.

- It builds indiscriminiately. It just builds ALL the libs from
  tools/base and tools/swt and updates all the JARs under prebuilts/devtools.
  That should only take 20-30 seconds though. Eventually we'll work on
  making it smarter because obviously we don't want anyone to just try
  to submit 30 JARs just because their timestamp changed.

- It generates a git merge msg in prebuilts/devtools that has the sha1
  of the corresponding tools/base and tools/swt projects.
  Use option -m to prevent this.


------

Need a place to discuss all this?
http://groups.google.com/group/adt-dev is the right place.

--- RM 20130409


