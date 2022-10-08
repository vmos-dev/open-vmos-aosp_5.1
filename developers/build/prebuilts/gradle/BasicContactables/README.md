Android BasicContactables Sample
===================================

This sample shows how to search for contacts, displaying a SearchView in the Action Bar for user input and implementing a query Cursor with CommonDataKinds.Contactables.

Introduction
------------

This sample displays a [SearchView][1] in the Action Bar when the search icon is clicked. It then implements the [LoaderManager.LoaderCallbacks][2] interface to query the contacts table, using a [CursorLoader][3].

For details on how to implement the [SearchView][1], refer to the training guide [Setting up the search interface][4].

For details on how to implement the [LoaderManager.LoaderCallbacks][2] interface, refer to the [Using the LoaderManager Callbacks][5] guide.

For details on how to query the contacts provider, refer to the [Contacts Provider Access][6] guide.

[1]: http://developer.android.com/reference/android/widget/SearchView.html
[2]: http://developer.android.com/reference/android/app/LoaderManager.LoaderCallbacks.html
[3]: http://developer.android.com/reference/android/content/CursorLoader.html
[4]: http://developer.android.com/training/search/setup.html
[5]: http://developer.android.com/guide/components/loaders.html#callback
[6]: http://developer.android.com/guide/topics/providers/contacts-provider.html#Access

Pre-requisites
--------------

- Android SDK v21
- Android Build Tools v21.1.1
- Android Support Repository

Screenshots
-------------

<img src="screenshots/1-main.png" height="400" alt="Screenshot"/> <img src="screenshots/2-search.png" height="400" alt="Screenshot"/> <img src="screenshots/3-results.png" height="400" alt="Screenshot"/> 

Getting Started
---------------

This sample uses the Gradle build system. To build this project, use the
"gradlew build" command or use "Import Project" in Android Studio.

Support
-------

- Google+ Community: https://plus.google.com/communities/105153134372062985968
- Stack Overflow: http://stackoverflow.com/questions/tagged/android

If you've found an error in this sample, please file an issue:
https://github.com/googlesamples/android-BasicContactables

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
