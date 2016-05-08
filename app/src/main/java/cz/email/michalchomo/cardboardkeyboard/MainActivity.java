package cz.email.michalchomo.cardboardkeyboard;

import android.app.Activity;
import android.hardware.Camera;
import android.os.Bundle;
import android.os.Debug;
import android.util.Log;

import org.opencv.android.CameraBridgeViewBase;
import org.opencv.android.OpenCVLoader;
import org.opencv.core.Mat;
import org.opencv.core.Rect;
import org.opencv.core.Size;
import org.opencv.imgproc.Imgproc;

import java.util.List;
import java.util.ListIterator;

/**
 * Created by Michal Chomo on 16. 2. 2016.
 */
public class MainActivity extends Activity implements CameraBridgeViewBase.CvCameraViewListener2 {

    private static final String TAG = "MainActivity";
    private static final int ResolutionX = 800;
    private static final int ResolutionY = 480;

    private CameraView mCameraView;

    private Mat mRgba;
    private Mat mGray;
    private Mat mHalf;

    // Load OpenCV and imageproc libraries.
    static {
        if (!OpenCVLoader.initDebug()) {
            Log.e(TAG, "Error loading OpenCV.");
        }
        try {
            System.loadLibrary("imageproc");
        } catch (Exception e) {
            Log.e(TAG, e.getMessage());
        }
    }

    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.layout);

        mCameraView = (CameraView) findViewById(R.id.camera_view);
        mCameraView.setCvCameraViewListener(this);
        mCameraView.enableView();
    }

    @Override
    public void onResume() {
        super.onResume();
    }

    @Override
    public void onPause() {
        super.onPause();
        if (mCameraView != null)
            mCameraView.disableView();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (mCameraView != null)
            mCameraView.disableView();
    }

    @Override
    public void onCameraViewStarted(int width, int height) {
        List<Camera.Size> resList = mCameraView.getResolutionList();

        ListIterator<Camera.Size> resolutionItr = resList.listIterator();
        while(resolutionItr.hasNext()) {
            Camera.Size element = resolutionItr.next();
            if(element.width == ResolutionX && element.height == ResolutionY) {
                mCameraView.setResolution(element);
                break;
            }
        }
    }

    @Override
    public void onCameraViewStopped() {
        mRgba.release();
    }

    @Override
    public Mat onCameraFrame(CameraBridgeViewBase.CvCameraViewFrame inputFrame) {
        mRgba = inputFrame.rgba();
        mGray = inputFrame.gray();

        detectMarkersAndDraw(mGray.getNativeObjAddr(), mRgba.getNativeObjAddr());

        mHalf = mRgba.clone();

        try {
            Imgproc.resize(mHalf, mHalf, new Size(mHalf.cols() / 2, mHalf.rows()), 0, 0, Imgproc.INTER_LINEAR);
            mHalf.copyTo(mRgba.submat(new Rect(0, 0, mHalf.cols(), mHalf.rows())));
            mHalf.copyTo(mRgba.submat(new Rect(mHalf.cols(), 0, mHalf.cols(), mHalf.rows())));
        } catch (Exception e) {
            Log.e(TAG, e.getMessage());
        }

        mHalf.release();

        return mRgba;
    }

    public native void detectMarkersAndDraw(long matAddrGr, long matAddrRgba);
}