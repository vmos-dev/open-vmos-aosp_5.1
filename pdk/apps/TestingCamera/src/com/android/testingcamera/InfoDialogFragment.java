package com.android.testingcamera;

import android.hardware.Camera;
import android.hardware.Camera.CameraInfo;
import android.os.Bundle;
import android.app.DialogFragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

public class InfoDialogFragment extends DialogFragment implements View.OnClickListener {

    Button mOkButton;
    TextView mInfoText;

    String mTitleString = "";
    String mInfoString = "";

    public InfoDialogFragment() {
        // Empty constructor required for DialogFragment
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_info, container);

        mOkButton = (Button) view.findViewById(R.id.info_ok_button);
        mOkButton.setOnClickListener(this);

        mInfoText= (TextView) view.findViewById(R.id.info_text);
        mInfoText.setText(mInfoString);

        getDialog().setTitle(mTitleString);
        return view;
    }

    @Override
    public void onClick(View v) {
        this.dismiss();
    }

    public void updateInfo(int cameraId, Camera camera) {
        mTitleString = "Current camera " + cameraId + " state";
        StringBuilder b = new StringBuilder();

        CameraInfo cameraInfo = new CameraInfo();
        Camera.getCameraInfo(cameraId, cameraInfo);
        b.append("getInfo() output:\n");
        b.append("  Facing: ");
        b.append(cameraInfo.facing == CameraInfo.CAMERA_FACING_FRONT ? "FRONT\n" : "BACK\n");
        b.append("  Orientation: ");
        b.append(cameraInfo.orientation);
        b.append("\n\n");
        b.append("getParameters() output:\n  ");
        Camera.Parameters p = camera.getParameters();
        String[] flatP = p.flatten().split(";");
        for (String param : flatP) {
            b.append(param);
            b.append("\n  ");
        }

        mInfoString = b.toString();
    }

}
