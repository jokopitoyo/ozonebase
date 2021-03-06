/** @addtogroup Processors */
/*@{*/

#ifndef OZ_FACE_DETECTOR_H
#define OZ_FACE_DETECTOR_H

#include "../base/ozDetector.h"


#include <set>
#include <map>
#include <list>

class MotionData;

///
/// Processor that detects faces on a video frame feed 
/// This processor uses dlib and also requires presence
/// of the dlib shape detector dat file
///  You can get the shape_predictor_68_face_landmarks.dat file from:
///   http://dlib.net/files/shape_predictor_68_face_landmarks.dat.bz2

class FaceDetector : public virtual Detector
{
CLASSID(FaceDetector);

public:
    typedef enum { OZ_FACE_MARKUP_NONE=0, OZ_FACE_MARKUP_OUTLINE=1, OZ_FACE_MARKUP_DETAIL=2, OZ_FACE_MARKUP_ALL=3 } FaceMarkup;

private:
    std::string mObjectData;
    FaceMarkup  mFaceMarkup;

/**
* @brief 
*
* @param name Name of instance

\code
nvrcam.face = new FaceDetector( "face-cam0" );
\endcode
*/
public:
    FaceDetector( const std::string &name, const std::string &objectData, FaceMarkup faceMarkup=OZ_FACE_MARKUP_ALL );
    FaceDetector( const std::string &objectData, FaceMarkup faceMarkup, VideoProvider &provider, const FeedLink &link=gQueuedVideoLink );
    ~FaceDetector();

    uint16_t width() const { return( videoProvider()->width() ); }
    uint16_t height() const { return( videoProvider()->height() ); }
    AVPixelFormat pixelFormat() const { return( Image::getFfPixFormat( Image::FMT_RGB ) ); }
    FrameRate frameRate() const { return( videoProvider()->frameRate() ); }

protected:
    int run();
};

#endif // OZ_FACE_DETECTOR_H

/*@}*/
