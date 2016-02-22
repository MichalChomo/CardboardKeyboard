package cz.email.michalchomo.cardboardkeyboard;

import android.app.Activity;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceView;

import com.google.vrtoolkit.cardboard.CardboardView;

import org.opencv.android.CameraBridgeViewBase;
import org.opencv.android.JavaCameraView;
import org.opencv.android.OpenCVLoader;
import org.opencv.core.Mat;

/**
 * Created by Michal Chomo on 16. 2. 2016.
 */
public class MainActivity extends Activity implements CameraBridgeViewBase.CvCameraViewListener2 {

    private static final String TAG = "MainActivity";

    private JavaCameraView mOpenCvCameraView;
    private CardboardView mCardboardView;

    private Renderer renderer;

    private Mat mRgba;
    private Mat mGray;

    static {
        if (!OpenCVLoader.initDebug()) {
            Log.e(TAG, "Error loading OpenCV.");
        }
        //System.loadLibrary("opencv_java3");
        System.loadLibrary("imageproc");
        //System.loadLibrary("vrtoolkit");
    }

    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.ui);

        mOpenCvCameraView = (JavaCameraView) findViewById(R.id.camera_view);
        mOpenCvCameraView.setVisibility(SurfaceView.VISIBLE);
        mOpenCvCameraView.setCvCameraViewListener(this);
        mOpenCvCameraView.enableView();

        renderer = new Renderer();
        mCardboardView = (CardboardView) findViewById(R.id.cardboard_view);
        mCardboardView.setRenderer(renderer);
        mCardboardView.setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);
        mCardboardView.setZOrderMediaOverlay(true);
    }

    @Override
    public void onResume() {
        super.onResume();
    }

    @Override
    public void onPause() {
        super.onPause();
        if (mOpenCvCameraView != null)
            mOpenCvCameraView.disableView();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (mOpenCvCameraView != null)
            mOpenCvCameraView.disableView();
    }

    @Override
    public void onCameraViewStarted(int width, int height) {
        mRgba = new Mat();

    }

    @Override
    public void onCameraViewStopped() {
        mRgba.release();
    }

    @Override
    public Mat onCameraFrame(CameraBridgeViewBase.CvCameraViewFrame inputFrame) {
        mRgba = inputFrame.rgba();
        mGray = inputFrame.gray();
        FindFeatures(mGray.getNativeObjAddr(), mRgba.getNativeObjAddr());
        return mRgba;
    }

    public native void FindFeatures(long matAddrGr, long matAddrRgba);
}
