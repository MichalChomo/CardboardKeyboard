package cz.email.michalchomo.cardboardkeyboard;

import android.content.Context;
import android.hardware.Camera.Size;
import android.util.AttributeSet;

import org.opencv.android.JavaCameraView;

import java.util.List;

public class CameraView extends JavaCameraView {

    private static final String TAG = "CameraView";

    public CameraView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public List<Size> getResolutionList() {
        return mCamera.getParameters().getSupportedPreviewSizes();
    }

    public void setResolution(Size resolution) {
        disconnectCamera();
        mMaxHeight = resolution.height;
        mMaxWidth = resolution.width;
        connectCamera(getWidth(), getHeight());
    }

    public Size getResolution() {
        return mCamera.getParameters().getPreviewSize();
    }

    public List<int[]> getFpsRange() {
        return mCamera.getParameters().getSupportedPreviewFpsRange();
    }

    public void setFps(int[] fps) {
        mCamera.getParameters().setPreviewFpsRange(fps[0], fps[1]);
    }
}