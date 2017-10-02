#include "rectangle.h"

int getThreshold(cv::Mat img, int width, int height)
{
	int size = width*height;
	cv::MatND outputhist;
	int hisSize[1] = { 256 };
	float range[2] = { 0.0, 255.0 };
	const float *ranges; ranges = &range[0];
	calcHist(&img, 1, 0, Mat(), outputhist, 1, hisSize, &ranges);
	double sum = 0;
	for (int i = 0; i < 256; i++)
	{
		sum = sum + i * outputhist.at<float>(i);
	}
	int threshold = 0;
	float sumvaluew = 0.0, sumvalue = 0.0, maximum = 0.0, wF, p12, diff, between;
	for (int i = 0; i < 256; i++)
	{
		sumvalue = sumvalue + outputhist.at<float>(i);
		sumvaluew = sumvaluew + i * outputhist.at<float>(i);
		wF = size - sumvalue;
		p12 = wF * sumvalue;
		if (p12 == 0){ p12 = 1; }
		diff = sumvaluew * wF - (sum - sumvaluew) * sumvalue;
		between = (float)diff * diff / p12;
		if (between >= maximum){
			threshold = i;
			maximum = between;
		}
	}
	return threshold;
}

cv::Rect getrectangular(cv::Mat img)
{
	IplImage pImg = IplImage(img);
	CvMemStorage * storage = cvCreateMemStorage(0);
	CvSeq * contour = 0, *contmax = 0;
	cvFindContours(&pImg, storage, &contour, sizeof(CvContour), CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, cvPoint(0, 0));
	double area = 0, maxArea = 0;
	for (; contour; contour = contour->h_next)
	{
		area = fabs(cvContourArea(contour, CV_WHOLE_SEQ));
		if (area > maxArea)
		{
			contmax = contour;
			maxArea = area;
		}
	}
	Rect maxRect = cvBoundingRect(contmax, 0);
	return maxRect;
}

cv::Mat CombinationandPostProcessing(cv::Mat salientMap, int width, int height)
{
	int threshold = getThreshold(salientMap, width, height);
	cv::Mat binaryMap = cv::Mat(salientMap.rows, salientMap.cols, CV_8UC1);
	//threshold = threshold - 5 > 0 ? threshold - 5 : threshold;
	for (int i = 0; i < width*height; i++)
	{	
		if (salientMap.data[i] >= threshold)
			binaryMap.data[i] =255;
		else
			binaryMap.data[i] = 0;
	}
	return binaryMap;
}

cv::Rect getSalientMap(cv::Mat img, int width, int height)
{
	Mat element = getStructuringElement(MORPH_RECT, Size(5, 5));
	cv::Mat salientMap = getBoundaryDissimilarityMap(img, 8);
	cv::dilate(salientMap, salientMap, element);
	cv::imshow("salient", salientMap);
	cv::Mat binaryMap = CombinationandPostProcessing(salientMap, width, height);
	cv::erode(binaryMap, binaryMap, element);
	cv::dilate(binaryMap, binaryMap, element);
	cv::dilate(binaryMap, binaryMap, element);
	cv::imshow("binary", binaryMap);
	cv::Rect salrect;
	salrect = getrectangular(binaryMap);

	return salrect;
}

#define MIN_SZ 25
int get_optimize_rect(cv::Mat image, cv::Point2i &tk_pt, cv::Size &tk_sz)
{
	cv::Rect init_rect;
	int init_width = tk_sz.width * 1.5;
	int init_height = tk_sz.height * 1.5;

	init_rect.x = tk_pt.x - init_width / 2 < 0 ? 0 : tk_pt.x - init_width / 2;
	init_rect.y = tk_pt.y - init_height / 2 < 0 ? 0 : tk_pt.y - init_height / 2;
	init_rect.width = init_width < image.cols - init_rect.x ? init_width : image.cols - init_rect.x;
	init_rect.height = init_height < image.rows - init_rect.y ? init_height : image.rows - init_rect.y;

	cv::Mat roimat = image(init_rect);
	cv::Size fixed_size(120, 120);
	cv::resize(roimat, roimat, fixed_size);
	cv::Rect optimize_rect = getSalientMap(roimat, 120, 120);

	float ratiox = init_rect.width / 120.0;
	float ratioy = init_rect.height / 120.0;

	optimize_rect.x = optimize_rect.x * ratiox + 0.5;
	optimize_rect.y = optimize_rect.y * ratioy + 0.5;
	optimize_rect.width = optimize_rect.width * ratiox + 0.5;
	optimize_rect.height = optimize_rect.height * ratioy + 0.5;

	if (optimize_rect.area() > init_rect.area() / 16)
	{
		tk_pt.x = init_rect.x + optimize_rect.x + optimize_rect.width / 2;
		tk_pt.y = init_rect.y + optimize_rect.y + optimize_rect.height / 2;


		tk_sz.width = optimize_rect.width < MIN_SZ ? MIN_SZ : optimize_rect.width;
		tk_sz.height = optimize_rect.height < MIN_SZ ? MIN_SZ : optimize_rect.height;

		int max_sz = tk_sz.width > tk_sz.height ? tk_sz.width : tk_sz.height;
		if (tk_sz.width < max_sz * 0.4)
		{
			tk_sz.width = max_sz * 0.4;
		}
		if (tk_sz.height < max_sz * 0.4)
		{
			tk_sz.height = max_sz * 0.4;
		}
	}
	return 0;
}