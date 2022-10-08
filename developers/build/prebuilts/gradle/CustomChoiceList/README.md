Android CustomChoiceList Sample
===================================

This sample demonstrates how to create custom checkable layouts, for use with ListView's choiceMode
attribute.

Introduction
------------

This sample demonstrates how to create custom single- or multi-choice [ListView][1] UIs on Android.

When a ListView has a `android:choiceMode` attribute set, it will allow users to "choose" one or more items. For
exmaple, refer to `res/layout/sample_main.xml` in this project:

```xml
<ListView android:id="@android:id/list"
    android:layout_width="match_parent"
    android:layout_height="0dp"
    android:layout_weight="1"
    android:paddingLeft="@dimen/page_margin"
    android:paddingRight="@dimen/page_margin"
    android:scrollbarStyle="outsideOverlay"
    android:choiceMode="multipleChoice" />
```

The framework provides these default list item layouts that show standard radio buttons or check boxes next to a single
line of text:

- android.R.layout.simple_list_item_single_choice
- android.R.layout.simple_list_item_multiple_choice.

In some cases, you may want to customize this layout. When doing so, the root view must implement the Checkable
interface. For an example, see this sample's `CheckableLinearLayout` class.

Lastly, remember to use padding on your ListViews to adhere to the standard metrics described in the Android Design
guidelines. When doing so, you should set the `android:scrollbarStyle` attribute such that the scrollbar doesn't inset.

[1]: http://developer.android.com/reference/android/widget/ListView.html

Pre-requisites
--------------

- Android SDK v21
- Android Build Tools v21.1.1
- Android Support Repository

Screenshots
-------------

<img src="screenshots/1-main.png" height="400" alt="Screenshot"/> <img src="screenshots/2-settings.png" height="400" alt="Screenshot"/> 

Getting Started
---------------

This sample uses the Gradle build system. To build this project, use the
"gradlew build" command or use "Import Project" in Android Studio.

Support
-------

- Google+ Community: https://plus.google.com/communities/105153134372062985968
- Stack Overflow: http://stackoverflow.com/questions/tagged/android

If you've found an error in this sample, please file an issue:
https://github.com/googlesamples/android-CustomChoiceList

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
