/************************************************************************************
 *
 *  Copyright (C), Autel Intelligent Technology Co., Ltd.
 *
 *  Description   : tracking interface head file
 *  Version       :
 *      V1.0   
 *          Date  : 2017/1/1
 *          Modification: Created file
 *      V1.1
 *          Date  : 2017/06/07
 *          Modifiction: 
 *              1¡¢Modify pfRegsiterTrackerResultInfo() parameter;
 *              2¡¢Add pfGetTrackerLibInfo() function;
 *              3¡¢Add pfSetTrackerLogLevel() function;
 *
 ************************************************************************************/
#ifndef __TK_INTERFACE_H__
#define __TK_INTERFACE_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define  TRACKER_INTERFACE_VERSION    0x0101


typedef enum {
    TRACKER_RESULT_MOVIDIUS = 0,   //  result of tracking_output_movidius
    TRACKER_RESULT_APP,            //  result of tracking_output_app

    TRACKER_RESULT_MAX,
} tracking_result_type;

//// transmitting H2 data to Movidius
typedef struct {
    unsigned long long timeStamp;
    int u;           // pixel coordinate x;
    int v;           // pixel coordinate y;
    int w;           // width of bounding box in pixel
    int h;           // height of bounding box in pixel
    float dist1;     // yaw in deg
    float dist2;     // ground coordinate x
    float dist3;     // ground coordinate y
    float dist4;     // ground coordinate z
    int   status;
} tracking_output_movidius;
//// end

//// H2-->App: define box of object, tracking status, distance between camera to object
typedef struct {
  unsigned long long timeStamp; // time stamp
  float x;        // ratio of x coordinate in image width
  float y;        // ratio of y coordinate in image height
  float width;    // ratio of box width in image width;
  float height;   // ratio of box height in image height;
  char  status;   // tracking status: 0: failed; 1: OK;
  float distance; // distance between camera with target
} tracking_output_app;

typedef void (* TrackingResult_F)(tracking_result_type type, void *result, int result_len);

//// transmitting Movidius to H2
typedef struct {
    unsigned long long timeStamp1;
    unsigned long long timeStamp2;
    float pos[3];
    float vel[3];
    float rpy[3];
} movidius_pose;

typedef struct {
    char brief[32];        //  eg: mkcf
    char version[32];      //  eg: V0.0.1
    char buildTime[32];    //  eg: 2017-06-07 18:00:00
} tracker_lib_info;

typedef enum {
    TRACKER_LOG_CLOSE = 0,
    TRACKER_LOG_DEBUG,
    TRACKER_LOG_INFO,
    TRACKER_LOG_ERROR,
} tracker_log_level;

typedef struct {
    const unsigned int version;   //  TRACKER_INTERFACE_VERSION

    const char  *module;

    // set bounding box of object for initializing tracking algorithm
    // API
    // int pfSetTrackerArea(float x, float y, float width, float height);
    //
    // x                - x coordinate of object bounding box in first frame(proportion to image)
    // y                - y coordinate of object bounding box in first frame(proportion to image)
    // width            - width of object bounding box in first frame(proportion to image)
    // height           - height of object bounding box in first frame(proportion to image)
    int  (*pfSetTrackerArea)(float x, float y, float width, float height);

    // stop tracking algorithm
    // API
    // void pfStopTracking();
    void (*pfStopTracking)(void);

    // recall function for getting result of tracking
    // API
    //
    // p_function         - function pointer
    int  (*pfRegsiterTrackerResultInfo)(TrackingResult_F p_function);

    // release function pointer
    // API
    //
    // p_function         - function pointer
    int  (*pfUnRegsiterTrackerResultInfo)(TrackingResult_F p_function);

    // interface of tracking algorithm, input: initialized bounding box of object and frames of video;
    // output: rectangle of tracking for object in frame
    //
    // buffer             - data of every frame
    // width              - image width
    // height             - image height
    // data_fomat         - data format: 0: ( YUV 4:2:0 ); 1: ( YUV 4:2:2 ); 2: ( RGB or BGR )
    int  (*pfTrackerInterface)(unsigned char* buffer, int width, int height, int data_format, uint64_t pts);

    // Movidius ---> H2
    int  (*pfSetMovidiusDataToH2)(movidius_pose* data);

    // Get tracker lib version¡¢build time...
    int  (*pfGetTrackerLibInfo)(tracker_lib_info *p_info);

    // Set tracker lib log level
    int  (*pfSetTrackerLogLevel)(tracker_log_level level);

} TrackerInterface;

#ifdef __cplusplus
}
#endif

#endif
