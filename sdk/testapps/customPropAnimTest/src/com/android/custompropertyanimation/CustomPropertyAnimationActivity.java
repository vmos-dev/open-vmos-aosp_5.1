package com.android.custompropertyanimation;

import android.animation.ObjectAnimator;
import android.app.Activity;
import android.os.Bundle;
import android.widget.LinearLayout;

public class CustomPropertyAnimationActivity extends Activity {
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
        LinearLayout container = (LinearLayout) findViewById(R.id.container);
        
        MyView view = new MyView(this);
        container.addView(view);
        
        ObjectAnimator anim = ObjectAnimator.ofFloat(view, "foo", 1);
        anim.start();
    }
}