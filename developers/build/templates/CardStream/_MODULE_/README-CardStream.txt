<#--
        Copyright 2013 The Android Open Source Project

        Licensed under the Apache License, Version 2.0 (the "License");
        you may not use this file except in compliance with the License.
        You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

        Unless required by applicable law or agreed to in writing, software
        distributed under the License is distributed on an "AS IS" BASIS,
        WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
        See the License for the specific language governing permissions and
        limitations under the License.
-->

Steps to implement CardStream template:
-in template-params.xml.ftl:
    -add the following templates
        <template src="base"/>
        <template src="CardStream"/>
    -add the following line to common imports
        <common src="activities"/>
        <common src="logger"/>

-Add a Fragment to handle behavior.  In your MainActivity.java class, it will reference a Fragment
 called (yourProjectName)Fragment.java.  Create that file in your project, using the "main" source
 folder instead of "common" or "templates".
   For instance, if your package name is com.example.foo, create the file
   src/main/java/com/example/foo/FooFragment.java

-Now it's time to deal with cards. Implement a method like this in your Fragment to access the CardStream:
private CardStreamFragment getCardStream() {
    if (mCards == null) {
        mCards = ((CardStream) getActivity()).getCardStream();
    }
    return mCards;
}


-Create a instance of Card.Builder with a tag String that *must* be unique among all cards.

	Card.Builder builder = new Card.Builder(UNIQUE_TAG_STRING);

-Set the properties for your card in the builder. Some properties (title, description, progress type) can also
be changed later.

	builder.setTitle(String title)

-Cards can also have more than one action that is shown as a button at the bottom of the card.
All actions *must* be defined through the builder. They can be hidden or shown later again, but they must be defined
in the builder before .build() is called.

-To implement an action, use Builder.addAction with a label, id, type (Neutral/Positive/Negative) and
 a CardActionCallback.
 Actions can be distinguished by their id to avoid the use of a large number of unnamed callback instances.
 For convenience, the tag of the card the action belongs to is also returned in the callback.

        builder.addAction(actionLabel1, 0, Card.ACTION_NEUTRAL, new Card.CardActionCallback() {
            @Override
            public void onClick(int cardActionId, String tag) {
		...
            }
        });


-After finishing setup process, call Buidler.build() to return a new instance of a Card.

        final Card card = builder.build(activity);

-Inside your MainActivity.java class, call getCardStream() to get the instance of the CardStreamFragment through
which cards are shown on screen.
A card needs to be added to the CardStreamFragment first before it can be shown.
Cards are identified by their unique tag.

        Card myCard = ...
        getCardStreamFragment().addCard(myCard);
        getCardStreamFragment().show(myCard.getTag());
        getCardStreamFragment().hide("MyCardTag");
        getCardStreamFragment().show("MyCardTag",false); // can't be dismissed by user


