Android BatchStepSensor Sample
===================================

Sample demonstrating how to set up SensorEventListeners for step
detectors and step counters.

Introduction
------------

This sample demonstrates the use of the two step [sensors][1] (step detector and counter) and
sensor batching.

It shows how to register a [SensorEventListener][2] with and without
batching and shows how these events are received.

The Step Detector sensor fires an
event when a step is detected, while the step counter returns the total number of
steps since a listener was first registered for this sensor.

Both sensors only count steps while a listener is registered. This sample only covers the
basic case, where a listener is only registered while the app is running. Likewise,
batched sensors can be used in the background (when the CPU is suspended), which
requires manually flushing the [sensor event][3] queue before it overflows, which is not
covered in this sample.

[1]: https://developer.android.com/reference/android/hardware/Sensor.html
[2]: https://developer.android.com/reference/android/hardware/SensorEventListener.html
[3]: https://developer.android.com/reference/android/hardware/SensorEvent.html

Pre-requisites
--------------

- Android SDK v21
- Android Build Tools v21.1.1
- Android Support Repository

Screenshots
-------------

<img src="screenshots/screenshot1.png" height="400" alt="Screenshot"/> <img src="screenshots/screenshot2.png" height="400" alt="Screenshot"/> <img src="screenshots/screenshot3.png" height="400" alt="Screenshot"/> <img src="screenshots/screenshot4.png" height="400" alt="Screenshot"/> <img src="screenshots/screenshot5.png" height="400" alt="Screenshot"/> <img src="screenshots/screenshot6.png" height="400" alt="Screenshot"/> 

Getting Started
---------------

This sample uses the Gradle build system. To build this project, use the
"gradlew build" command or use "Import Project" in Android Studio.

Support
-------

- Google+ Community: https://plus.google.com/communities/105153134372062985968
- Stack Overflow: http://stackoverflow.com/questions/tagged/android

If you've found an error in this sample, please file an issue:
https://github.com/googlesamples/android-BatchStepSensor

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
