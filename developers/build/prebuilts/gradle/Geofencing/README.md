Android Geofencing Sample
===================================

When the user enters the vicinity of the Android building (B44) or the Yerba Buena
Gardens near the Moscone center in San Francisco, a notification silently appears on their
wearable with an option to check in. This notification automatically disappears when they leave
the area, and reappears the next time they are at one of these locations.

Introduction
------------

Geofencing combines awareness of the user's current location with awareness of
nearby features, defined as the user's proximity to locations that may be of
interest. To mark a location of interest, you specify its latitude and longitude.
To adjust the proximity for the location, you add a radius. The latitude,
longitude, and radius define a geofence. You can have multiple active
geofences at one time.

To use geofencing, start by defining the geofences you want to monitor.
Although you usually store geofence data in a local database or download
it from the network, you need to send a geofence to Location Services as
an instance of [Geofence][2], which you create with `Geofence.Builder`. Each
Geofence object contains the following information:

1. Latitude, longitude, and radius
2. Expiration time
3. Transition type
4. Geofence ID

Read more about geofences in [Creating and Monitoring Geofences][1].

[1]:http://developer.android.com/training/location/geofencing.html
[2]:http://developer.android.com/reference/com/google/android/gms/location/Geofence.html

Pre-requisites
--------------

- Android SDK v21
- Android Build Tools v21.1.1
- Android Support Repository

Screenshots
-------------

<img src="screenshots/android_building_check_in.png" height="400" alt="Screenshot"/> 

Getting Started
---------------

This sample uses the Gradle build system. To build this project, use the
"gradlew build" command or use "Import Project" in Android Studio.

Support
-------

- Google+ Community: https://plus.google.com/communities/105153134372062985968
- Stack Overflow: http://stackoverflow.com/questions/tagged/android

If you've found an error in this sample, please file an issue:
https://github.com/googlesamples/android-Geofencing

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
