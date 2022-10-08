Android ClippingBasic Sample
===================================

A basic app showing how to clip on a View using [ViewOutlineProvider][1] interface,
by which a View builds its outline, used for shadowing and clipping.

Introduction
------------

The [ViewOutlineProvider][1] interface offers you a method to populate the outline of a View.
You need to implement a getOutline(android.view.View, android.graphics.Outline)
method to clip a View in a specific shape.

This example clips the outline of a View as a rounded rectangle by defining a class that
 implements ViewOutlineProvider by following code:

```java
private class ClipOutlineProvider extends ViewOutlineProvider {
    @Override
    public void getOutline(View view, Outline outline) {
        final int margin = Math.min(view.getWidth(), view.getHeight()) / 10;
        outline.setRoundRect(margin, margin, view.getWidth() - margin,
                view.getHeight() - margin, margin / 2);
    }
}
```

To clip a View by the defined outline, setting a OutlineProvider to a View
to be clipped is needed like following:

```java
final View clippedView = view.findViewById(R.id.frame);
clippedView.setOutlineProvider(mOutlineProvider);
```

You can toggle if the View is clipped by calling [setClipToOutline(boolean)][2]
like following code:

```java
clippedView.setClipToOutline(true); // Setting false disable clipping
```

[1]: https://developer.android.com/reference/android/view/ViewOutlineProvider.html
[2]: https://developer.android.com/reference/android/view/View.html#setClipToOutline(boolean)

Pre-requisites
--------------

- Android SDK v21
- Android Build Tools v21.1.1
- Android Support Repository

Screenshots
-------------

<img src="screenshots/screenshot-1.png" height="400" alt="Screenshot"/> <img src="screenshots/screenshot-2.png" height="400" alt="Screenshot"/> 

Getting Started
---------------

This sample uses the Gradle build system. To build this project, use the
"gradlew build" command or use "Import Project" in Android Studio.

Support
-------

- Google+ Community: https://plus.google.com/communities/105153134372062985968
- Stack Overflow: http://stackoverflow.com/questions/tagged/android

If you've found an error in this sample, please file an issue:
https://github.com/googlesamples/android-ClippingBasic

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
