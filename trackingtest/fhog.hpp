
#ifndef _FHOG_H_
#define _FHOG_H_

#include <stdio.h>
#include "opencv2/imgproc/imgproc_c.h"

typedef struct{
    int sizeX;
    int sizeY;
    int numFeatures;
    float *map;
} st_fhog_feture;


#include "float.h"

#define PI    CV_PI

#define EPS 0.000001

#define F_MAX FLT_MAX
#define F_MIN -FLT_MAX

// The number of elements in bin
// The number of sectors in gradient histogram building
#define NUM_SECTOR 9

// The number of levels in image resize procedure
// We need Lambda levels to resize image twice
#define LAMBDA 10

// Block size. Used in feature pyramid building procedure
#define SIDE_LENGTH 8

#define VAL_OF_TRUNCATE 0.2f 


//modified from "_lsvm_error.h"
#define LATENT_SVM_OK 0
#define LATENT_SVM_MEM_NULL 2
#define DISTANCE_TRANSFORM_OK 1
#define DISTANCE_TRANSFORM_GET_INTERSECTION_ERROR -1
#define DISTANCE_TRANSFORM_ERROR -2
#define DISTANCE_TRANSFORM_EQUAL_POINTS -3
#define LATENT_SVM_GET_FEATURE_PYRAMID_FAILED -4
#define LATENT_SVM_SEARCH_OBJECT_FAILED -5
#define LATENT_SVM_FAILED_SUPERPOSITION -6
#define FILTER_OUT_OF_BOUNDARIES -7
#define LATENT_SVM_TBB_SCHEDULE_CREATION_FAILED -8
#define LATENT_SVM_TBB_NUMTHREADS_NOT_CORRECT -9
#define FFT_OK 2
#define FFT_ERROR -10
#define LSVM_PARSER_FILE_NOT_FOUND -11



/*
// Getting feature map for the selected subimage  
//
// API
// int getFeatureMaps(const IplImage * image, const int k, featureMap **map);
// INPUT
// image             - selected subimage
// k                 - size of cells
// OUTPUT
// map               - feature map
// RESULT
// Error status
*/
int getFeatureMaps_neon(unsigned char* src_data, int width, int height, st_fhog_feture **map);

int getFeatureMaps(const IplImage * image, const int k, st_fhog_feture **map);

int getFeatureMaps_gray(const IplImage* image, const int k, st_fhog_feture **map);


/*
// Feature map Normalization and Truncation 
//
// API
// int normalizationAndTruncationFeatureMaps(featureMap *map, const float alfa);
// INPUT
// map               - feature map
// alfa              - truncation threshold
// OUTPUT
// map               - truncated and normalized feature map
// RESULT
// Error status
*/
int normalizeAndTruncate(st_fhog_feture *map, const float alfa);

/*
// Feature map reduction
// In each cell we reduce dimension of the feature vector
// according to original paper special procedure
//
// API
// int PCAFeatureMaps(featureMap *map)
// INPUT
// map               - feature map
// OUTPUT
// map               - feature map
// RESULT
// Error status
*/
int PCAFeatureMaps(st_fhog_feture *map);


//modified from "lsvmc_routine.h"

int allocFeatureMapObject(st_fhog_feture **obj, const int sizeX, const int sizeY,
                          const int p);


int freeFeatureMapObject (st_fhog_feture **obj);


#endif
