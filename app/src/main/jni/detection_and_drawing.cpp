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
enum OctaveNote {C, D, E, F, G, A, H, CC};

extern "C" {

struct ChordLinesPoints {
    Point2f lineStarts[3];
    Point2f lineEnds[3];
};

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
    static Scalar colors[8];
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

double getXCoordOfNote(OctaveNote note, double horizontalEighth) {
    switch(note) {
        case C:
            return 0.0;
        case D:
            return horizontalEighth;
        case E:
            return horizontalEighth * 2;
        case F:
            return horizontalEighth * 3;
        case G:
            return horizontalEighth * 4;
        case A:
            return horizontalEighth * 5;
        case H:
            return horizontalEighth * 6;
        case CC:
            return horizontalEighth * 7;
        default: break;
    }
}

ChordLinesPoints getChordLinePoints(OctaveNote chord, double horizontalEighth, double verticalEighth) {
    ChordLinesPoints linesPoints;
    Point2f point;

    switch(chord) {
        case C:
            point.y = verticalEighth * 5.5;
            point.x = getXCoordOfNote(C, horizontalEighth);
            linesPoints.lineStarts[0] = point;
            point.x += horizontalEighth;
            linesPoints.lineEnds[0] = point;

            point.x = getXCoordOfNote(E, horizontalEighth);
            linesPoints.lineStarts[1] = point;
            point.x += horizontalEighth;
            linesPoints.lineEnds[1] = point;

            point.x = getXCoordOfNote(G, horizontalEighth);
            linesPoints.lineStarts[2] = point;
            point.x += horizontalEighth;
            linesPoints.lineEnds[2] = point;
            break;
        case D:
            point.y = verticalEighth * 5.6;
            point.x = getXCoordOfNote(D, horizontalEighth);
            linesPoints.lineStarts[0] = point;
            point.x += horizontalEighth;
            linesPoints.lineEnds[0] = point;

            point.x = getXCoordOfNote(F, horizontalEighth);
            linesPoints.lineStarts[1] = point;
            point.x += horizontalEighth;
            linesPoints.lineEnds[1] = point;

            point.x = getXCoordOfNote(A, horizontalEighth);
            linesPoints.lineStarts[2] = point;
            point.x += horizontalEighth;
            linesPoints.lineEnds[2] = point;
            break;
        case E:
            point.y = verticalEighth * 5.7;
            point.x = getXCoordOfNote(E, horizontalEighth);
            linesPoints.lineStarts[0] = point;
            point.x += horizontalEighth;
            linesPoints.lineEnds[0] = point;

            point.x = getXCoordOfNote(G, horizontalEighth);
            linesPoints.lineStarts[1] = point;
            point.x += horizontalEighth;
            linesPoints.lineEnds[1] = point;

            point.x = getXCoordOfNote(H, horizontalEighth);
            linesPoints.lineStarts[2] = point;
            point.x += horizontalEighth;
            linesPoints.lineEnds[2] = point;
            break;
        case F:
            point.y = verticalEighth * 5.8;
            point.x = getXCoordOfNote(F, horizontalEighth);
            linesPoints.lineStarts[0] = point;
            point.x += horizontalEighth;
            linesPoints.lineEnds[0] = point;

            point.x = getXCoordOfNote(A, horizontalEighth);
            linesPoints.lineStarts[1] = point;
            point.x += horizontalEighth;
            linesPoints.lineEnds[1] = point;

            point.x = getXCoordOfNote(CC, horizontalEighth);
            linesPoints.lineStarts[2] = point;
            point.x += horizontalEighth;
            linesPoints.lineEnds[2] = point;
            break;
        case G:
            point.y = verticalEighth * 5.9;
            point.x = getXCoordOfNote(G, horizontalEighth);
            linesPoints.lineStarts[0] = point;
            point.x += horizontalEighth;
            linesPoints.lineEnds[0] = point;

            point.x = getXCoordOfNote(H, horizontalEighth);
            linesPoints.lineStarts[1] = point;
            point.x += horizontalEighth;
            linesPoints.lineEnds[1] = point;

            point.x = getXCoordOfNote(D, horizontalEighth);
            linesPoints.lineStarts[2] = point;
            point.x += horizontalEighth;
            linesPoints.lineEnds[2] = point;
            break;
        case A:
            point.y = verticalEighth * 6.0;
            point.x = getXCoordOfNote(A, horizontalEighth);
            linesPoints.lineStarts[0] = point;
            point.x += horizontalEighth;
            linesPoints.lineEnds[0] = point;

            point.x = getXCoordOfNote(E, horizontalEighth);
            linesPoints.lineStarts[1] = point;
            point.x += horizontalEighth;
            linesPoints.lineEnds[1] = point;

            point.x = getXCoordOfNote(CC, horizontalEighth);
            linesPoints.lineStarts[2] = point;
            point.x += horizontalEighth;
            linesPoints.lineEnds[2] = point;
            break;
        case H:
            point.y = verticalEighth * 6.1;
            point.x = getXCoordOfNote(H, horizontalEighth);
            linesPoints.lineStarts[0] = point;
            point.x += horizontalEighth;
            linesPoints.lineEnds[0] = point;

            point.x = getXCoordOfNote(F, horizontalEighth);
            linesPoints.lineStarts[1] = point;
            point.x += horizontalEighth;
            linesPoints.lineEnds[1] = point;

            point.x = getXCoordOfNote(D, horizontalEighth);
            linesPoints.lineStarts[2] = point;
            point.x += horizontalEighth;
            linesPoints.lineEnds[2] = point;
            break;
        default: break;
    }

    return linesPoints;
}

void drawChords(Mat &octaveRoi, Mat &wholeScreen) {
    int fontFace = FONT_HERSHEY_SIMPLEX;
    double fontScale = 1.4;
    int textThickness = 5;
    double horizontalEighth = octaveRoi.cols / 8;
    double verticalEighth = octaveRoi.rows / 8;
    string chordNames("CDEFGAH");
    Point2f namePosition = Point2f(horizontalEighth, verticalEighth);
    Color color = COLOR_C;
    int colorInt = static_cast<int>(color);
    int lineThickness = 3;
    ChordLinesPoints linesPoints;

    for(unsigned int i = 0; i < chordNames.size(); ++i) {
        putText(wholeScreen, chordNames.substr(i, 1), namePosition, fontFace, fontScale, getColor(static_cast<Color>(i)), textThickness);
        namePosition.x += horizontalEighth;

        linesPoints = getChordLinePoints(static_cast<OctaveNote>(i), horizontalEighth, verticalEighth);
        for(unsigned int j = 0; j < 3; ++j) {
            line(octaveRoi, linesPoints.lineStarts[j], linesPoints.lineEnds[j], getColor(static_cast<Color>(i)), lineThickness);
            // Emphasize root note with white circle in the center of the line.
            if(j == 0) {
                Point2f point = Point2f((linesPoints.lineStarts[j].x + linesPoints.lineEnds[j].x) / 2, linesPoints.lineStarts[j].y);
                circle(octaveRoi, point, 5, Scalar(255, 255, 255), -1);
            }
        }
    }

}

void draw(Mat &mRgb, vector< vector<Point2f> > &markerCorners, vector<int> sortedIds) {
    Mat overlay(mRgb.rows, mRgb.cols, CV_8UC4);
    Mat overlayWarped, mask, maskInv, result1, result2;
    Mat H;
    vector< Point2f > octaveCorners;
    vector< Point2f > overlayCorners;
    int octave = getOctave(sortedIds.back(), 49);

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
    Ptr<Dictionary> dictionary = getPredefinedDictionary(DICT_4X4_50);

    try {
        detectMarkers(mGr, dictionary, markerCorners, markerIds);
    } catch (cv::Exception& e) {
        __android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "%s", e.what());
    }

    if(markerIds.size() > 3) {
        try {
            //cvtColor(mRgb, mRgb, COLOR_BGRA2BGR);
            //drawDetectedMarkers(mRgb, markerCorners, markerIds);
            draw(mRgb, markerCorners, getSortedIds(markerIds));
        } catch (cv::Exception& e) {
            __android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "%s", e.what());
        }
    }

}

}