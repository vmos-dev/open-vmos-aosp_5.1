Android ElizaChat Sample
===================================

A basic sample showing how to add extensions to notifications on wearable using
[NotificationCompat.WearableExtender][1] API by providing a chat experience.

Introduction
------------

[NotificationCompat.WearableExtender][1] API offers you a functionality to
 add wearable extensions to notifications like [Add the Voice Input as a Notification Action][2].

This example also adds the wearable notifications as a voice input by the following code:

```java
NotificationCompat.Builder builder = new NotificationCompat.Builder(this)
        .setContentTitle(getString(R.string.eliza))
        .setContentText(mLastResponse)
        .setLargeIcon(BitmapFactory.decodeResource(getResources(), R.drawable.bg_eliza))
        .setSmallIcon(R.drawable.bg_eliza)
        .setPriority(NotificationCompat.PRIORITY_MIN);

Intent intent = new Intent(ACTION_RESPONSE);
PendingIntent pendingIntent = PendingIntent.getService(this, 0, intent,
        PendingIntent.FLAG_ONE_SHOT | PendingIntent.FLAG_CANCEL_CURRENT);
Notification notification = builder
        .extend(new NotificationCompat.WearableExtender()
                .addAction(new NotificationCompat.Action.Builder(
                        R.drawable.ic_full_reply, getString(R.string.reply), pendingIntent)
                        .addRemoteInput(new RemoteInput.Builder(EXTRA_REPLY)
                                .setLabel(getString(R.string.reply))
                                .build())
                        .build()))
        .build();
NotificationManagerCompat.from(this).notify(0, notification);
```

When you issue this notification, users can swipe to the left to see the "Reply" action button.

When you receive the response, you can do it by the following code:

```java
@Override
public int onStartCommand(Intent intent, int flags, int startId) {
    if (null == intent || null == intent.getAction()) {
        return Service.START_STICKY;
    }
    String action = intent.getAction();
    if (action.equals(ACTION_RESPONSE)) {
        Bundle remoteInputResults = RemoteInput.getResultsFromIntent(intent);
        CharSequence replyMessage = "";
        if (remoteInputResults != null) {
            replyMessage = remoteInputResults.getCharSequence(EXTRA_REPLY);
        }
        processIncoming(replyMessage.toString());
    } else if (action.equals(MainActivity.ACTION_GET_CONVERSATION)) {
        broadcastMessage(mCompleteConversation.toString());
    }
    return Service.START_STICKY;
}
```

[1]: https://developer.android.com/reference/android/support/v4/app/NotificationCompat.WearableExtender.html
[2]: https://developer.android.com/training/wearables/notifications/voice-input.html#AddAction

Pre-requisites
--------------

- Android SDK v21
- Android Build Tools v21.1.1
- Android Support Repository

Screenshots
-------------

<img src="screenshots/companion_eliza_chat_response.png" height="400" alt="Screenshot"/> <img src="screenshots/companion_eliza_chat.png" height="400" alt="Screenshot"/> <img src="screenshots/wearable_eliza_notification.png" height="400" alt="Screenshot"/> <img src="screenshots/wearable_voice_reply.png" height="400" alt="Screenshot"/> 

Getting Started
---------------

This sample uses the Gradle build system. To build this project, use the
"gradlew build" command or use "Import Project" in Android Studio.

Support
-------

- Google+ Community: https://plus.google.com/communities/105153134372062985968
- Stack Overflow: http://stackoverflow.com/questions/tagged/android

If you've found an error in this sample, please file an issue:
https://github.com/googlesamples/android-ElizaChat

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
