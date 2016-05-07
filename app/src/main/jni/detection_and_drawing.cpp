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

enum Color {COLOR_C, COLOR_D, COLOR_E, COLOR_F, COLOR_G, COLOR_A, COLOR_H, COLOR_TEXT};
enum Chord {C, D, E, F, G, A, H};

extern "C" {

// x coordinates of circles for a chord, struct made for returning from function
struct ChordXCoords {
    double coords[3];
};

// x coordinates of circles for each key
double KeyCirclesXCoords[8] = {0};

string intToString(int num)
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

    // Store minimal ID for determining octave.
    sortedIds.push_back(minId);

    return sortedIds;
}

Scalar getColor(Color c) {
    static Scalar colors[15];
    colors[static_cast<int>(COLOR_TEXT)] = Scalar(0, 210, 0);
    colors[static_cast<int>(COLOR_C)] = Scalar(239, 10, 0);
    colors[static_cast<int>(COLOR_D)] = Scalar(0, 14, 239);
    colors[static_cast<int>(COLOR_E)] = Scalar(250, 90, 7);
    colors[static_cast<int>(COLOR_F)] = Scalar(240, 0, 230);
    colors[static_cast<int>(COLOR_G)] = Scalar(240, 240, 0);
    colors[static_cast<int>(COLOR_A)] = Scalar(117, 44, 0);
    colors[static_cast<int>(COLOR_H)] = Scalar(0, 230, 240);

    return colors[c];
}

int getOctave(int id, int keysCount) {
    switch(keysCount) {
        case 49:
        case 61:
        case 76:
            return (id + 3) / 2;
        case 88:
            return (id + 1) / 2;
        default: return (id + 3) / 2;
    }
}

void drawNoteNames(Mat &img, int octave) {
    int fontFace = FONT_HERSHEY_SIMPLEX;
    double fontScale = 2.0;
    int thickness = 3;
    double horizontalEighth = img.cols / 8;
    double verticalEighth = img.rows / 8;
    Point2f notePosition = Point2f((horizontalEighth / 8), (img.rows - verticalEighth));
    string notes("CDEFGAHC");

    for(auto c : notes) {
        putText(img, c + intToString(octave), notePosition, fontFace, fontScale, getColor(COLOR_TEXT), thickness);
        notePosition.x += horizontalEighth;
    }
}

double getKeyXCoord(int index, double eighth) {
    if(KeyCirclesXCoords[0] == 0) {
        double initialX = eighth / 8;
        for(unsigned int i = 0; i < 8; ++i) {
            KeyCirclesXCoords[i] = initialX;
            initialX += eighth;
        }
    }

    double returnedCoord = KeyCirclesXCoords[index];
    KeyCirclesXCoords[index] += eighth / 4;

    return returnedCoord;
}

ChordXCoords getChordXCoords(Chord c, double eighth) {
    ChordXCoords chordXCoords;

    switch(c) {
        case C:
            chordXCoords.coords[0] = getKeyXCoord(0, eighth);
            chordXCoords.coords[1] = getKeyXCoord(2, eighth);
            chordXCoords.coords[2] = getKeyXCoord(4, eighth);
            break;
        case D:
            chordXCoords.coords[0] = getKeyXCoord(1, eighth);
            chordXCoords.coords[1] = getKeyXCoord(3, eighth);
            chordXCoords.coords[2] = getKeyXCoord(5, eighth);
            break;
        case E:
            chordXCoords.coords[0] = getKeyXCoord(2, eighth);
            chordXCoords.coords[1] = getKeyXCoord(4, eighth);
            chordXCoords.coords[2] = getKeyXCoord(6, eighth);
            break;
        case F:
            chordXCoords.coords[0] = getKeyXCoord(3, eighth);
            chordXCoords.coords[1] = getKeyXCoord(0, eighth);
            chordXCoords.coords[2] = getKeyXCoord(5, eighth);
            break;
        case G:
            chordXCoords.coords[0] = getKeyXCoord(4, eighth);
            chordXCoords.coords[1] = getKeyXCoord(1, eighth);
            chordXCoords.coords[2] = getKeyXCoord(6, eighth);
            break;
        case A:
            chordXCoords.coords[0] = getKeyXCoord(5, eighth);
            chordXCoords.coords[1] = getKeyXCoord(2, eighth);
            chordXCoords.coords[2] = getKeyXCoord(0, eighth);
            break;
        case H:
            chordXCoords.coords[0] = getKeyXCoord(4, eighth);
            chordXCoords.coords[1] = getKeyXCoord(1, eighth);
            chordXCoords.coords[2] = getKeyXCoord(6, eighth);
            break;
        default: break;
    }
    return chordXCoords;
}

void drawChords(Mat &octaveRoi, Mat &wholeScreen) {
    int fontFace = FONT_HERSHEY_SIMPLEX;
    double fontScale = 1.4;
    int thickness = 5;
    double horizontalEighth = octaveRoi.cols / 8;
    double verticalEighth = octaveRoi.rows / 8;
    string chordNames("CDEFGAH");
    Point2f namePosition = Point2f(horizontalEighth, verticalEighth);
    Color color = COLOR_C;
    int colorInt = static_cast<int>(color);
    double chordYCoord = verticalEighth * 6 - 10;
    ChordXCoords chordXCoords;

    Point2f circlePosition = Point2f(0.0, chordYCoord);

    for(unsigned int i = 0; i < chordNames.size(); ++i) {
        putText(wholeScreen, chordNames.substr(i, 1), namePosition, fontFace, fontScale, getColor(static_cast<Color>(i)), thickness);
        namePosition.x += horizontalEighth;
        //++colorInt;
        //color = static_cast<Color>(colorInt);

        chordXCoords = getChordXCoords(static_cast<Chord>(i), horizontalEighth);
        circlePosition.x = chordXCoords.coords[0];
        // First circle is always root note, draw black border.
        circle(octaveRoi, circlePosition, 10, getColor(static_cast<Color>(i)), -1);
        circle(octaveRoi, circlePosition, 11, Scalar(0, 0, 0), 2);
        circlePosition.x = chordXCoords.coords[1];
        circle(octaveRoi, circlePosition, 10, getColor(static_cast<Color>(i)), -1);
        circlePosition.x = chordXCoords.coords[2];
        circle(octaveRoi, circlePosition, 10, getColor(static_cast<Color>(i)), -1);
    }
    for(unsigned int i = 0; i < 8; ++i) {
        KeyCirclesXCoords[i] = 0.0;
    }

}

void draw(Mat &mRgb, vector< vector<Point2f> > &markerCorners, vector<int> sortedIds) {
    Mat overlay(mRgb.rows, mRgb.cols, CV_8UC3);
    Mat overlayWarped, mask, maskInv, result1, result2;
    Mat H;
    vector< Point2f > octaveCorners;
    vector< Point2f > overlayCorners;
    int octave = getOctave(sortedIds.back(), 49);
    // double alpha = 0.7; // The lower the value, the more visible overlay is.

    overlayCorners.push_back(Point2f(0.0, 0.0));
    overlayCorners.push_back(Point2f(0.0, overlay.rows));
    overlayCorners.push_back(Point2f(overlay.cols, 0.0));
    overlayCorners.push_back(Point2f(overlay.cols, overlay.rows));

    for(unsigned int i = 0; i < (sortedIds.size() - 4); i += 2)
    {
        drawNoteNames(overlay, octave);
        drawChords(overlay, mRgb);
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
Java_cz_email_michalchomo_cardboardkeyboard_MainActivity_detectMarkersAndDraw(JNIEnv *env, jobject instance,
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