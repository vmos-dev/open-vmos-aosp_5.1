package com.android.custompropertyanimation;

import android.content.Context;
import android.view.View;

public class MyView extends View {

    float mFoo = 0;
    
    public MyView(Context context) {
        super(context);
    }
    
    public void setFoo(float foo) {
        System.out.println("foo = " + foo);
        mFoo = foo;
    }

    public float getFoo() {
        System.out.println("getFoo() returning " + mFoo);
        return mFoo;
    }

}
