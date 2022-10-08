package com.google.android.leanbacklauncher.partnerwidget;

import android.appwidget.AppWidgetManager;
import android.appwidget.AppWidgetProvider;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.Bundle;
import android.widget.RemoteViews;

public class ClockWidgetProvider extends AppWidgetProvider {

    private static final String SHARED_PREFS = "widget-prefs";
    private static final int INET_CONDITION_THRESHOLD = 50;
    private static final String INET_CONDITION_ACTION = "android.net.conn.INET_CONDITION_ACTION";
    private static final String EXTRA_INET_CONDITION = "inetCondition";

    @Override
    public void onReceive(Context context, Intent intent) {
        String intentAction = intent.getAction();

        if (INET_CONDITION_ACTION.equals(intentAction) ||
                ConnectivityManager.CONNECTIVITY_ACTION.equals(intentAction)) {

            if (INET_CONDITION_ACTION.equals(intentAction)) {
                // a broadcast with this intent action is only fired when we are actually connected
                // (i.e connectionStatus = 100).  So, clearing connectivity status when changing
                // networks is required
                int connectionStatus = intent.getIntExtra(EXTRA_INET_CONDITION, -551);
                writeConnectivity(context, connectionStatus > INET_CONDITION_THRESHOLD);
            }

            update(context);
        }

        super.onReceive(context, intent);
    }

    @Override
    public void onAppWidgetOptionsChanged(Context context, AppWidgetManager appWidgetManager,
            int appWidgetId, Bundle newOptions) {
        update(context);
    }

    public void onUpdate(Context context, AppWidgetManager appWidgetManager, int[] appWidgetIds) {
        update(context);
        super.onUpdate(context, appWidgetManager, appWidgetIds);
    }

    private void update(Context context) {
        AppWidgetManager appWidgetManager = AppWidgetManager.getInstance(context);
        ComponentName thisWidget = new ComponentName(context, ClockWidgetProvider.class);
        appWidgetManager.updateAppWidget(thisWidget, getRemoteViews(context));
    }

    private RemoteViews getRemoteViews(Context context) {
        RemoteViews remoteViews = new RemoteViews(context.getPackageName(), R.layout.clock_widget);
        remoteViews.setImageViewResource(R.id.connectivity_indicator, getConnectedResId(context));

        return remoteViews;
    }

    private static int getConnectedResId(Context context) {
        ConnectivityManager cm = (ConnectivityManager) context
                .getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo info = cm.getActiveNetworkInfo();

        int resId = 0;
        if (info == null || !info.isAvailable() || !info.isConnected()) {
            // can't have Internet access with no network
            writeConnectivity(context, false);
            resId = R.drawable.ic_widget_wifi_not_connected;
        } else if (!readConnectivity(context)) {
            resId = R.drawable.ic_widget_wifi_no_internet;
        } else {
            // internet is connected and working, show nothing
            resId = android.R.color.transparent;
        }

        return resId;
    }

    private static void writeConnectivity(Context context, boolean inetConnected) {
        context.getSharedPreferences(SHARED_PREFS, Context.MODE_PRIVATE).edit()
                .putBoolean(EXTRA_INET_CONDITION, inetConnected).apply();
    }

    private static boolean readConnectivity(Context context) {
        return context.getSharedPreferences(SHARED_PREFS, Context.MODE_PRIVATE).getBoolean(
                EXTRA_INET_CONDITION, true);
    }

}
