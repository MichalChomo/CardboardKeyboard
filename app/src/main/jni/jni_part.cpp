#include <jni.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <vector>
#include <android/log.h>
#include "aruco.hpp"

#define APPNAME "CardboardKeyobard"
#define SORTED_IDS_SIZE 42
#define TOP_LEFT 0
#define TOP_RIGHT 1
#define BOTTOM_RIGHT 2
#define BOTTOM_LEFT 3

using namespace std;
using namespace cv;
using namespace aruco;

extern "C" {

// Return vector which indexes are markerIds from 0 to n and its values are indexes of markerCorners.
// Example: marker with id 4 was detected first, so sortedIds[4] == 0
vector<int> getSortedIds(vector<int> &markerIds) {
    // Initialize vector with fixed size and values -1.
    vector<int> sortedIds(SORTED_IDS_SIZE, -1);
    int i = 0;

    for(vector<int>::iterator it = markerIds.begin(); it != markerIds.end(); it++) {
        // Put markerId index as value and markerId as index to sortedIds.
        sortedIds.at(*it) = i;
        ++i;
    }
    // Erase all unused elements(value == -1).
    sortedIds.erase(remove(sortedIds.begin(), sortedIds.end(), -1), sortedIds.end());

    return sortedIds;
}

void draw(Mat &mRgb, vector< vector<Point2f> > &markerCorners, vector<int> sortedIds) {
    int fontFace = FONT_HERSHEY_SIMPLEX;
    double fontScale = 2;
    int thickness = 3;
    Point2f point;

    for(unsigned int i = 0; i < (sortedIds.size() - 1); i += 2)
    {
        __android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "DEBUG size = %d", sortedIds.size());
//        rectangle(mRgb, markerCorners[sortedIds[i]][BOTTOM_RIGHT], markerCorners[sortedIds[i+1]][TOP_LEFT], Scalar(0));
        point.x = markerCorners[sortedIds[i]][BOTTOM_RIGHT].x; // + ((markerCorners[sortedIds[i+1]][BOTTOM_RIGHT].x - markerCorners[sortedIds[i]][BOTTOM_RIGHT].x) / 16);
        point.y = markerCorners[sortedIds[i+1]][TOP_LEFT].y; // + ((markerCorners[sortedIds[i]][TOP_LEFT].y - markerCorners[sortedIds[i+1]][TOP_LEFT].y) / 8);
        __android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "i = %d sortedIds[i] = %d", i, sortedIds[i]);
        putText(mRgb, "C42", point, fontFace, fontScale, Scalar(0), thickness);
    }
}

JNIEXPORT void JNICALL
Java_cz_email_michalchomo_cardboardkeyboard_MainActivity_detectMarkers(JNIEnv *env, jobject instance,
                                                                      jlong matAddrGr,
                                                                      jlong matAddrRgba) {
    Mat &mGr = *(Mat *) matAddrGr;
    Mat &mRgb = *(Mat *) matAddrRgba;

    vector< int > markerIds;
    vector< vector<Point2f> > markerCorners;
    Ptr<DetectorParameters> parameters = DetectorParameters::create();
    Ptr<Dictionary> dictionary = getPredefinedDictionary(DICT_4X4_50);

    //parameters->doCornerRefinement = true;
    try {
        detectMarkers(mGr, dictionary, markerCorners, markerIds, parameters);
    } catch (cv::Exception& e) {
        __android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "%s", e.what());
    }

    cvtColor(mRgb, mRgb, COLOR_BGRA2BGR);

    if(markerIds.size() > 2) {
        try {
            drawDetectedMarkers(mRgb, markerCorners, markerIds);
            draw(mRgb, markerCorners, getSortedIds(markerIds));
        } catch (cv::Exception& e) {
            __android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "%s", e.what());
        }
    }

}

}