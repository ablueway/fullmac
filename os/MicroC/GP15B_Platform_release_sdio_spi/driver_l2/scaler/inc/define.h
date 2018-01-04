#ifndef __DEFINE_H__
#define __DEFINE_H__

#include "project.h"

#define IMG_FMT_GRAY	0
#define IMG_FMT_YUYV	1
#define IMG_FMT_YCbYCr	2
#define IMG_FMT_RGB		3
#define IMG_FMT_UYVY	4
#define IMG_FMT_CbYCrY	5

#define MAX(x, y)	((x) >= (y) ? (x) : (y))
#define MIN(x, y)	((x) <  (y) ? (x) : (y))
#define ABS(x)		((x) >= 0 ? (x) : -(x))

typedef struct
{
	INT16S width;
	INT16S height;
    INT16S widthStep;
	INT8S ch;
	INT8S format;
	INT8U *ptr;
} gpImage;

typedef struct
{
    INT16S x;
    INT16S y;
    INT16S width;
    INT16S height;
} gpRect;

typedef struct
{
    INT16S width;
    INT16S height;
} gpSize;

typedef struct
{
	INT16S x;
	INT16S y;
} gpPoint;

typedef struct {
	gpPoint	left_pt;
	gpPoint right_pt;
	gpPoint top_pt;
	gpPoint bottom_pt;
} gpFeaturePoint;

typedef struct {
	gpPoint	left_pt;
	gpPoint right_pt;
	gpPoint rtop_pt; 
	gpPoint ltop_pt; 
	gpPoint rbottom_pt;
	gpPoint lbottom_pt;
} gpFeaturePoint2;

#endif // __DEFINE_H__
