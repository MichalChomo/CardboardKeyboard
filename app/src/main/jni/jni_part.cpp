#include <jni.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include "opencv2/features2d.hpp"
#include "opencv2/core/affine.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include <vector>
#include <sstream>
#include <android/log.h>
#include "aruco.hpp"

#define APPNAME "CardboardKeyboard"
#define SORTED_IDS_SIZE 25
#define TOP_LEFT 0
#define TOP_RIGHT 1
#define BOTTOM_RIGHT 2
#define BOTTOM_LEFT 3

using namespace std;
using namespace cv;
using namespace aruco;

extern "C" {

string to_string(int num)
{
    ostringstream convert;
    convert << num;
    return convert.str();
}

// Return vector which indexes are markerIds from 0 to n and its values are indexes of markerCorners.
// Example: marker with id 4 was detected first, so sortedIds[4] == 0
vector<int> getSortedIds(vector<int> &markerIds) {
    // Initialize vector with fixed size and values -1.
    vector<int> sortedIds(SORTED_IDS_SIZE, -1);
    int i = 0;
    int minId = SORTED_IDS_SIZE; // Lowest markerId, needed to determine octave.

    for(vector<int>::iterator it = markerIds.begin(); it != markerIds.end(); it++) {
        // Put markerId index as value and markerId as index to sortedIds.
        sortedIds.at(*it) = i;
        ++i;
        if(*it < minId) minId = *it;
    }
    // Erase all unused elements(value == -1).
    sortedIds.erase(remove(sortedIds.begin(), sortedIds.end(), -1), sortedIds.end());
    sortedIds.push_back(minId);

    return sortedIds;
}

void drawNoteNames(Mat &img, int octave) {
    int fontFace = FONT_HERSHEY_SIMPLEX;
    double fontScale = 2.0;
    int thickness = 3;
    double horizontalEighth = img.cols / 8;
    double verticalEighth = img.rows / 8;
    Point2f point = Point2f((horizontalEighth / 8), (img.rows - verticalEighth));
    string notes("CDEFGAHC");

    for(auto c : notes) {
        putText(img, c + to_string(octave), point, fontFace, fontScale, Scalar(0, 255, 0), thickness);
        point.x += horizontalEighth;
    }

}

void draw(Mat &mRgb, vector< vector<Point2f> > &markerCorners, vector<int> sortedIds) {
    Mat overlay(mRgb.rows, mRgb.cols, CV_8UC3);
    Mat overlayWarped, mask, maskInv, result1, result2;
    Mat H;
    vector< Point2f > octaveCorners;
    vector< Point2f > overlayCorners;
    int octave = sortedIds.back();

    overlayCorners.push_back(Point2f(0.0, 0.0));
    overlayCorners.push_back(Point2f(0.0, overlay.rows));
    overlayCorners.push_back(Point2f(overlay.cols, 0.0));
    overlayCorners.push_back(Point2f(overlay.cols, overlay.rows));

    for(unsigned int i = 0; i < (sortedIds.size() - 4); i += 2)
    {
        drawNoteNames(overlay, octave);
        ++octave;
        octaveCorners.push_back(markerCorners[sortedIds[i]][BOTTOM_LEFT]);
        octaveCorners.push_back(markerCorners[sortedIds[i+1]][BOTTOM_LEFT]);
        octaveCorners.push_back(markerCorners[sortedIds[i+2]][BOTTOM_RIGHT]);
        octaveCorners.push_back(markerCorners[sortedIds[i+3]][BOTTOM_RIGHT]);
        H = findHomography(overlayCorners, octaveCorners, RHO);
        octaveCorners.clear();
        if(H.empty()) {
            return;
        }
        warpPerspective(overlay, overlayWarped, H, mRgb.size());
        cvtColor(overlayWarped, mask, CV_BGR2GRAY);
        threshold(mask, mask, 0, 255, CV_THRESH_BINARY);
        bitwise_not(mask, maskInv);
        mRgb.copyTo(result1, maskInv);
        overlayWarped.copyTo(result2, mask);
        mRgb = result1 + result2;

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

    try {
        detectMarkers(mGr, dictionary, markerCorners, markerIds, parameters);
    } catch (cv::Exception& e) {
        __android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "%s", e.what());
    }

    if(markerIds.size() > 3) {
        try {
            cvtColor(mRgb, mRgb, COLOR_BGRA2BGR);
            //drawDetectedMarkers(mRgb, markerCorners, markerIds);
            draw(mRgb, markerCorners, getSortedIds(markerIds));
        } catch (cv::Exception& e) {
            __android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "%s", e.what());
        }
    }

}

}