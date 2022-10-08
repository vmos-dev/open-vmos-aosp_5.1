Android EmbeddedApp Sample
===================================

This simple app demonstrates how to embed a wearable app into a phone app.

Introduction
------------

Wearable apps can be installed directly onto Android Wear devices during development, using either a direct ADB
connection or ADB-over-Bluetooth. However, when releasing your app to end users, you must package your
wearable APK inside of a traditional APK for distribution via a paired phone.

When end users install this APK onto their phone, the wearable APK will be automatically detected, extracted, and pushed
to their any paired wearable devices.

This sample demonstrates how to properly package a wearable app for release in this manner. The wearable app is inside
the `Wearable` directory, and the phone app (which will be used as a container for distribution) is the `Application`
directory. There is nothing special about these apps, other than the `wearApp` dependency in the (host) phone app's
`build.gradle` file:

```groovy
dependencies {
    compile 'com.google.android.gms:play-services-wearable:6.5.+'
    wearApp project(':Wearable')
}
```

This dependency will automatically package the wearable APK during a **release build** (e.g. using the "Build > Generate
Signed APK..." command in Android Studio). Note that this packaging is **not** performed for debug builds for
performance reasons. During development, your wearable and phone apps must still be pushed individually to their
respective devices using an ADB connection.

Pre-requisites
--------------

- Android SDK v21
- Android Build Tools v21.1.1
- Android Support Repository

Screenshots
-------------

<img src="screenshots/embedded_wearable_app.png" height="400" alt="Screenshot"/> <img src="screenshots/phone_app.png" height="400" alt="Screenshot"/> 

Getting Started
---------------

This sample uses the Gradle build system. To build this project, use the
"gradlew build" command or use "Import Project" in Android Studio.

Support
-------

- Google+ Community: https://plus.google.com/communities/105153134372062985968
- Stack Overflow: http://stackoverflow.com/questions/tagged/android

If you've found an error in this sample, please file an issue:
https://github.com/googlesamples/android-EmbeddedApp

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
