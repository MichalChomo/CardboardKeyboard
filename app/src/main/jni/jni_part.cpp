#include <jni.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <vector>
#include <android/log.h>
#include "aruco.hpp"

#define APPNAME "CardboardKeyobard"

using namespace std;
using namespace cv;
using namespace aruco;

extern "C" {

JNIEXPORT void JNICALL
Java_cz_email_michalchomo_cardboardkeyboard_MainActivity_FindFeatures(JNIEnv *env, jobject instance,
                                                                      jlong matAddrGr,
                                                                      jlong matAddrRgba) {

    Mat &mGr = *(Mat *) matAddrGr;
    Mat &mRgb = *(Mat *) matAddrRgba;

//__android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "Marker ID is %d", markers[i].id);
    vector< int > markerIds;
    vector< vector<Point2f> > markerCorners, rejectedCandidates;
    Ptr<DetectorParameters> parameters = DetectorParameters::create();
    Ptr<Dictionary> dictionary = getPredefinedDictionary(DICT_4X4_50);

    parameters->doCornerRefinement = true;
    detectMarkers(mGr, dictionary, markerCorners, markerIds, parameters, rejectedCandidates);

    cvtColor(mRgb, mRgb, COLOR_BGRA2BGR);

    if(markerIds.size() > 0) {
        drawDetectedMarkers(mRgb, markerCorners, markerIds ) ;
    }


}
}