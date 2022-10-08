/*
 * Copyright 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.notificationstudio.editor;

import android.app.Activity;
import android.app.DatePickerDialog;
import android.app.DatePickerDialog.OnDateSetListener;
import android.app.Dialog;
import android.app.DialogFragment;
import android.app.FragmentTransaction;
import android.app.TimePickerDialog;
import android.app.TimePickerDialog.OnTimeSetListener;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.DatePicker;
import android.widget.TimePicker;

import com.android.notificationstudio.R;
import com.android.notificationstudio.editor.Editors.Editor;
import com.android.notificationstudio.model.EditableItem;

import java.text.SimpleDateFormat;
import java.util.Date;

public class DateTimeEditor implements Editor {
    private static final SimpleDateFormat YYYY_MM_DD = new SimpleDateFormat("yyyy/MM/dd");
    private static final SimpleDateFormat HH_MM_SS = new SimpleDateFormat("HH:mm:ss");

    @SuppressWarnings("deprecation")
    public Runnable bindEditor(View v, final EditableItem item, final Runnable afterChange) {

        final Button dateButton = (Button) v.findViewById(R.id.date_button);
        final Button timeButton = (Button) v.findViewById(R.id.time_button);
        final Button resetButton = (Button) v.findViewById(R.id.reset_button);

        int vPad = v.getResources().getDimensionPixelSize(R.dimen.editor_datetime_padding_v);
        int hPad = v.getResources().getDimensionPixelSize(R.dimen.editor_datetime_padding_h);
        for (Button b : new Button[] { dateButton, timeButton, resetButton }) {
            b.setVisibility(View.VISIBLE);
            b.setPadding(hPad, vPad, hPad, vPad);
        }

        final Runnable updateButtonText = new Runnable() {
            public void run() {
                Date d = getDateTime(item);
                String dateString = YYYY_MM_DD.format(d);
                dateButton.setText(dateString);
                String timeString = HH_MM_SS.format(d);
                timeButton.setText(timeString);
            }};
        updateButtonText.run();

        // wire up date button
        DialogFragment datePickerFragment = new DialogFragment() {
            @Override
            public Dialog onCreateDialog(Bundle savedInstanceState) {
                Date d = getDateTime(item);
                OnDateSetListener onDateSet = new OnDateSetListener() {
                    public void onDateSet(DatePicker view, int year,
                            int monthOfYear, int dayOfMonth) {
                        Date d = getDateTime(item);
                        d.setYear(year - 1900);
                        d.setMonth(monthOfYear);
                        d.setDate(dayOfMonth);
                        item.setValue(d.getTime());
                        updateButtonText.run();
                        afterChange.run();
                    }
                };
                return new DatePickerDialog(getActivity(), onDateSet,
                        d.getYear() + 1900, d.getMonth(), d.getDate());
            }
        };
        Activity activity = (Activity) v.getContext();
        launchDialogOnClick(activity, "datePicker", dateButton, datePickerFragment);

        // wire up time button
        DialogFragment timePickerFragment = new DialogFragment() {
            @Override
            public Dialog onCreateDialog(Bundle savedInstanceState) {
                Date d = getDateTime(item);
                OnTimeSetListener onTimeSet = new OnTimeSetListener() {
                    public void onTimeSet(TimePicker view, int hourOfDay,
                            int minute) {
                        Date d = getDateTime(item);
                        d.setHours(hourOfDay);
                        d.setMinutes(minute);
                        item.setValue(d.getTime());
                        updateButtonText.run();
                        afterChange.run();
                    }
                };
                return new TimePickerDialog(getActivity(),
                        onTimeSet, d.getHours(), d.getMinutes(), true);
            }
        };
        launchDialogOnClick(activity, "timePicker", timeButton, timePickerFragment);

        // wire up reset button
        resetButton.setOnClickListener(new OnClickListener(){
            public void onClick(View v) {
                item.setValue(null);
                updateButtonText.run();
                afterChange.run();
            }});
        return updateButtonText;
    }

    private static Date getDateTime(EditableItem item) {
        long value = item.hasValue() ? item.getValueLong() : System.currentTimeMillis();
        return new Date(value);
    }

    private static void launchDialogOnClick(final Activity activity,
            final String tag, Button button, final DialogFragment fragment) {
        button.setOnClickListener(new OnClickListener(){
            public void onClick(View v) {
                FragmentTransaction ft = activity.getFragmentManager().beginTransaction();
                fragment.show(ft, tag);
            }});
    }

}
