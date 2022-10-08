Android FragmentTransition Sample
===================================

This sample demonstrates how to start a Transition after a Fragment Transaction.

Introduction
------------

This sample uses [the Transition framework][1] to show a nice visual effect on
Fragment Transaction.

Animation for fragment _transaction_ can be customized by overriding
[onCreateAnimation][2]. In this sample, we set up an AnimationListener
for this Animation, and start our Transition on [onAnimationEnd][3].

Transition is started by calling [TransitionManager.go][4]. We don't
instantiate a Scene for the initial state of Transition, as we never
go back. One thing we need to be careful here is that we need to
populate the content of Views after starting the Transition.

[1]: https://developer.android.com/reference/android/transition/package-summary.html
[2]: http://developer.android.com/reference/android/support/v4/app/Fragment.html#onCreateAnimation(int, boolean, int)
[3]: http://developer.android.com/reference/android/view/animation/Animation.AnimationListener.html#onAnimationEnd(android.view.animation.Animation)
[4]: http://developer.android.com/reference/android/transition/TransitionManager.html#go(android.transition.Scene)

Pre-requisites
--------------

- Android SDK v21
- Android Build Tools v21.1.1
- Android Support Repository

Screenshots
-------------

<img src="screenshots/grid.png" height="400" alt="Screenshot"/> <img src="screenshots/main.png" height="400" alt="Screenshot"/> 

Getting Started
---------------

This sample uses the Gradle build system. To build this project, use the
"gradlew build" command or use "Import Project" in Android Studio.

Support
-------

- Google+ Community: https://plus.google.com/communities/105153134372062985968
- Stack Overflow: http://stackoverflow.com/questions/tagged/android

If you've found an error in this sample, please file an issue:
https://github.com/googlesamples/android-FragmentTransition

Patches are encouraged, and may be submitted by forking this project and
submitting a pull request through GitHub. Please see CONTRIBUTING.md for more details.

License
-------

Copyright 2014 The Android Open Source Project, Inc.

Licensed to the Apache Software Foundation (ASF) under one or more contributor
license agreements.  See the NOTICE file distributed with this work for
additional information regarding copyright ownership.  The ASF licenses this
file to you under the Apache License, Version 2.0 (the "License"); you may not
use this file except in compliance with the License.  You may obtain a copy of
the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
License for the specific language governing permissions and limitations under
the License.
