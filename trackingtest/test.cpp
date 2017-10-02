#include <iostream>
#include <stdio.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <string>
#include <direct.h> 
#include <opencv2/opencv.hpp>
#include<io.h>
#include<vector>

#include "autel_tk.h"
#include "tk_api.h"
#include "tk_interface.h"
#include "rectangle.h"

using namespace std;
using namespace cv;

vector<string> res;
#define show_kcf
#define show_vatic
#define show_dsst

cv::Point g_topLeft(0, 0);
cv::Point g_botRight(0, 0);
cv::Point current(0, 0);
bool plot = false;
bool bbxIsInitialize = false;
bool bIsInitialize = false;

cv::Size half_sz;

vector<string> listdir(const string &path)
{

	string dir = path;
	vector<string> s;
	_finddata_t fileDir;
	long lfDir;

	if ((lfDir = _findfirst(dir.c_str(), &fileDir)) == -1l)
		printf("No file is found\n");
	else{
		do{

			string str(fileDir.name);
			if (str.find('.') == -1)
				s.push_back(str);


		} while (_findnext(lfDir, &fileDir) == 0);
	}
	_findclose(lfDir);
	return s;

}

void findfile(const string &str)
{
	string s = str;
	vector<string> tmp = listdir(s + "\\*");
	for (int i = 0; i<tmp.size(); i++)
	{


		string temp = s + "\\" + tmp[i];
		res.push_back(temp);
		findfile(temp);
	}
}

static void onMouse(int event, int x, int y, int, void* param)
{
	cv::Mat img = ((cv::Mat *)param)->clone();
	if (event == cv::EVENT_LBUTTONDOWN)
	{
		bIsInitialize = false;
		bbxIsInitialize = false;
		g_topLeft = Point(x, y);
		plot = true;
	}
	else if (event == cv::EVENT_LBUTTONUP && !bbxIsInitialize)
	{
		g_botRight = Point(x, y);
		plot = false;
		int width = g_topLeft.x < g_botRight.x ? g_botRight.x - g_topLeft.x : g_topLeft.x - g_botRight.x;
		int height = g_topLeft.y < g_botRight.y ? g_botRight.y - g_topLeft.y : g_topLeft.y - g_botRight.y;
		if ((width>10) || (height>10))
		{
			bbxIsInitialize = true;
		}
		else
		{
			printf("target is too small!\nplz choose target again!\n");
		}
	}
	else if (event == cv::EVENT_MOUSEMOVE && !bbxIsInitialize)
	{
		//plot bbox
		current = Point(x, y);
		int minx = g_topLeft.x < current.x ? g_topLeft.x : current.x;
		int miny = g_topLeft.y < current.y ? g_topLeft.y : current.y;
		int width = g_topLeft.x < current.x ? current.x - g_topLeft.x : g_topLeft.x - current.x;
		int height = g_topLeft.y < current.y ? current.y - g_topLeft.y : g_topLeft.y - current.y;
		if (plot)
		{
			cv::rectangle(img, Point(minx, miny), Point(minx + width, miny + height), cv::Scalar(0, 255, 0), 2);
			imshow("demo", img);
		}
	}
}

//#define TEST
int test(string str)
{
	string str_cpy = str;
	vector<char*> vec_str;

	const char * split = "\\.";
	char *p = strtok((char*)str_cpy.c_str(), split);
	while (p)
	{
		vec_str.push_back(p);
		//printf("%s\n", p);
		p = strtok(NULL, split);
	}
	char* sequence_name = vec_str[vec_str.size() - 1];
	//int sequence_num = atoi(vec_str[vec_str.size() - 1]);
	//printf("sequence_num: %04d\n", sequence_num);

	char text[256];
	int icnt = 0;
	Mat frame;
	int img_width = 0;
	int img_height = 0;
	char filename[256];
	double temp, x1, y1, x2, y2;
	sprintf(filename, "%s\\initialization.txt", str.c_str());
	ifstream initial_file(filename);

	bool bisuseinitial = false;
	if (!initial_file)
	{
		printf("\n no initialize file, plz input initialization box by mouse\n");
	}
	else
	{
		bisuseinitial = true;
		string line;
		getline(initial_file, line);
		std::istringstream vatic_coor(line);
		vatic_coor >> x1 >> y1 >> x2 >> y2;
#ifdef RESIZE
		x1 = x1 / 2;
		y1 = y1 / 2;
		x2 = x2 / 2;
		y2 = y2 / 2;
#endif
	}

	sprintf(filename, "%s\\%d.jpg", str.c_str(), icnt);
	//printf("%s\n", filename);
	frame = cv::imread(filename);
	if (frame.empty())
	{
		printf("this file is not property folder\n");
		return 0;
	}
#ifdef RESIZE
	cv::resize(frame, frame, cv::Size(640, 360));
#endif
	img_width = frame.cols;
	img_height = frame.rows;
	string window_name = "demo";
	namedWindow(window_name, 1);

	tk_params params;
	params.padding = 1.0;
	params.output_sigma_factor = 1 / 16.0;
	params.scale_sigma_factor = 1 / 5.0;
	params.lambda = 1e-2;
	params.learning_rate = 0.025;
	params.number_of_scales = 33;
	params.scale_step = 1.04;
	params.scale_model_max_area = 512;
	if (!bisuseinitial)
	{
		cv::setMouseCallback("demo", onMouse, &frame);
		while (!bbxIsInitialize)
		{
			imshow("demo", frame);
			char key = (char)waitKey(0);
			if (key == 27)
			{
				return 0;
			}
		}
	}
	sprintf(filename, "%s\\result", str.c_str());
	_mkdir(filename);
	sprintf(filename, "%s\\result\\ZB_%s.avi", str.c_str(), sequence_name);
	cv::VideoWriter vwriter(filename, CV_FOURCC('M', 'J', 'P', 'G'), 25, cv::Size(img_width, img_height));
	vwriter << frame;
	double xMin = 0;
	double yMin = 0;
	double width = 0;
	double height = 0;
	if (bisuseinitial)
	{
		xMin = x1;
		yMin = y1;
		width = x2 - x1;
		height = y2 - y1;
	}
	else
	{
		xMin = g_topLeft.x < g_botRight.x ? g_topLeft.x : g_botRight.x;
		yMin = g_topLeft.y < g_botRight.y ? g_topLeft.y : g_botRight.y;
		width = g_topLeft.x < g_botRight.x ? g_botRight.x - g_topLeft.x : g_topLeft.x - g_botRight.x;
		height = g_topLeft.y < g_botRight.y ? g_botRight.y - g_topLeft.y : g_topLeft.y - g_botRight.y;
	}
	sprintf(filename, "%s\\result\\ZB_%s.txt", str.c_str(), sequence_name);
	ofstream coordflie(filename, ios::out);
	sprintf(filename, "%s\\result\\ZB_%s_time.txt", str.c_str(), sequence_name);
	ofstream timefile(filename, ios::out);

	double time_cost = 0;
	double total_time = 0;
	coordflie << (int)xMin << " " << (int)yMin << " " << (int)(xMin + width) << " " << (int)(yMin + height) << " " << icnt << endl;
	timefile << total_time << endl;
	icnt++;
	cv::Point2i tk_pt;
	tk_pt.x = xMin + width / 2;;
	tk_pt.y = yMin + height / 2;

	//const char * split = "\\";
	//char *p = strtok((char*)str.c_str(), split);
	//printf("p: %s\n", p);

	cv::Size tk_sz;
	tk_sz.width = width;
	tk_sz.height = height;

	tk_data track_obj;
	tk_init(frame, tk_pt, tk_sz, &track_obj, params);

#ifdef TEST
	ifstream vatic_file;
	string line;
	sprintf(filename, "%s\\ZB_%s.txt", str.c_str(), sequence_name);
	vatic_file.open(filename);
	getline(vatic_file, line);
	std::istringstream init_coor(line);
	init_coor >> x1 >> y1 >> x2 >> y2;
#endif

	sprintf(filename, "%s\\%d.jpg", str.c_str(), icnt);
	frame = cv::imread(filename);
#ifdef RESIZE
	cv::resize(frame, frame, cv::Size(640, 360));
#endif
	vwriter << frame;
	int ilost_cnt = 0;
	while (!frame.empty())
	{
#ifdef RESIZE
		cv::resize(frame, frame, cv::Size(640, 360));
#endif
		double t1 = cv::getTickCount();
		tk_track(frame, tk_pt, tk_sz, &track_obj, params);
		//printf("%d, %d\n", tk_sz.height, tk_sz.width);
		time_cost = ((double)cv::getTickCount() - t1) / cv::getTickFrequency();
		total_time += time_cost;
		//printf("track cost: %.4f\n", time_cost);
		int height = tk_sz.height*track_obj.scale_fator;
		int width = tk_sz.width*track_obj.scale_fator;

		int xx1 = tk_pt.x - width / 2;
		int yy1 = tk_pt.y - height / 2;
		int xx2 = tk_pt.x + width / 2;
		int yy2 = tk_pt.y + height / 2;
		coordflie << xx1 << " " << yy1 << " " << xx2 << " " << yy2 << " " << icnt << endl;
		timefile << total_time << endl;
		icnt += 1;
	/*	cv::Point2i detect_pt;
		cv::Size detect_sz = tk_sz;
		tk_detect(frame, detect_pt, detect_sz, &track_obj, params);*/
		if (track_obj.tk_psr>h_psr_th)
		{
			cv::rectangle(frame, cv::Point2i(tk_pt.x - width / 2, tk_pt.y - height / 2), cv::Point2i(tk_pt.x + width / 2, tk_pt.y + height / 2), CV_RGB(0, 255, 0), 2);
			ilost_cnt = 0;
		}
		else
		{
			if (ilost_cnt > 5)
			{
				cv::Point2i detect_pt;
				cv::Size detect_sz = tk_sz;
				tk_detect(frame, detect_pt, detect_sz, &track_obj, params);
				if (track_obj.tk_psr > h_psr_th)
				{
					track_obj.tk_ptf.x = detect_pt.x;
					track_obj.tk_ptf.y = detect_pt.y;
					cv::rectangle(frame, cv::Point2i(detect_pt.x - width / 2, detect_pt.y - height / 2), cv::Point2i(detect_pt.x + width / 2, detect_pt.y + height / 2), CV_RGB(0, 255, 255), 2);
					cv::waitKey(0);
				}
				else
				{
					cv::rectangle(frame, cv::Point2i(detect_pt.x - width / 2, detect_pt.y - height / 2), cv::Point2i(detect_pt.x + width / 2, detect_pt.y + height / 2), CV_RGB(255, 0, 255), 2);
				}
			}
			else
			{
				cv::rectangle(frame, cv::Point2i(tk_pt.x - width / 2, tk_pt.y - height / 2), cv::Point2i(tk_pt.x + width / 2, tk_pt.y + height / 2), CV_RGB(255, 0, 0), 2);
			}
			ilost_cnt++;
		}

#ifdef TEST
		getline(vatic_file, line);
		std::istringstream vatic_coor(line);
		vatic_coor >> x1 >> y1 >> x2 >> y2;
		rectangle(frame, Point(x1, y1), Point(x2, y2), Scalar(0, 0, 255), 2, 8);
#endif

		imshow(window_name, frame);
		//printf("psr: %.4f\n", track_obj.tk_psr);
		//imshow(window_name, frame);
		vwriter << frame;
		sprintf(filename, "%s\\%d.jpg", str.c_str(), icnt);

		frame = cv::imread(filename);

		//cv::putText(frame, text, cv::Point(100, 100), 1, 5, CV_RGB(255, 0, 0), 2);
		char key = (char)waitKey(1);
		if (key == ' ')
			waitKey(0);
		if (key == 27)
			break;
	}
	tk_release(&track_obj, params);
	plot = false;
	bbxIsInitialize = false;
	bIsInitialize = false;
	return 0;
}

#define show_vatic
int test_video()
{
	printf("plz input video path!\n");
	string str;
	getline(cin, str);

	string str_cpy = str;
	vector<char*> vec_str;

	const char * split = "\\.";
	char *p = strtok((char*)str_cpy.c_str(), split);
	while (p)
	{
		vec_str.push_back(p);
		//printf("%s\n", p);
		p = strtok(NULL, split);
	}
	char* sequence_name = vec_str[vec_str.size() - 2];

	tk_params params;
	params.padding = 1.0;
	params.output_sigma_factor = 1 / 16.0;
	params.scale_sigma_factor = 1 / 5.0;
	params.lambda = 0.01;
	params.learning_rate = 0.03;
	params.number_of_scales = 33;
	params.scale_step = 1.04;
	params.scale_model_max_area = 512;
	
	VideoCapture capture(str);
	if (!capture.isOpened())
	{
		printf("fail to open video!\n");
		return -1;
	}
	int totalFrameNumber = capture.get(CV_CAP_PROP_FRAME_COUNT);
	printf("total number: %d\n", totalFrameNumber);

	cv::Mat frame;
	string window_name = "demo";
	namedWindow(window_name, 1);
//	cv::setMouseCallback("demo", onMouse, &frame);
//	while (capture.read(frame) && !bbxIsInitialize)
//	{
//#ifdef RESIZE
//		cv::resize(frame, frame, cv::Size(640, 360));
//#endif
//		imshow("demo", frame);
//		char key = (char)waitKey(0);
//		if (key == 27)
//		{
//			return 0;
//		}
//	}

	double xMin = 0;
	double yMin = 0;
	double width = 0;
	double height = 0;

	//xMin = g_topLeft.x < g_botRight.x ? g_topLeft.x : g_botRight.x;
	//yMin = g_topLeft.y < g_botRight.y ? g_topLeft.y : g_botRight.y;
	//width = g_topLeft.x < g_botRight.x ? g_botRight.x - g_topLeft.x : g_topLeft.x - g_botRight.x;
	//height = g_topLeft.y < g_botRight.y ? g_botRight.y - g_topLeft.y : g_topLeft.y - g_botRight.y;
	//printf("%f, %f, %f, %f, %f\n", xMin, yMin, width, height, temp);

	char *FileName = "E:/H2_test_video/";
	char FilePath[256];
	ifstream zb_file;
	string line;
	sprintf(FilePath, "%s/%s/ZB_%s.txt", FileName, sequence_name, sequence_name);
	zb_file.open(FilePath);
	getline(zb_file, line);
	std::istringstream init_coor(line);
	double temp, x1, y1, x2, y2;
	init_coor >> x1 >> y1 >> x2 >> y2 >> temp;

	double bx1, by1, bx2, by2;
	ifstream mkcf_file;
	sprintf(FilePath, "%s/%s/MKCF_%s.txt", FileName, sequence_name, sequence_name);
	mkcf_file.open(FilePath);
	getline(mkcf_file, line);
	std::istringstream bacf_init(line);
	bacf_init >> bx1 >> by1 >> bx2 >> by2;

	cv::Point2i tk_pt;
	tk_pt.x = (x1 + x2)/4;
	tk_pt.y = (y1 + y2)/4;

	cv::Size tk_sz;
	tk_sz.width = (x2 - x1) / 2.0;
	tk_sz.height = (y2 - y1) / 2.0;

	tk_data track_obj;
	if (capture.read(frame))
	{	
#ifdef RESIZE
		cv::resize(frame, frame, cv::Size(640, 360));
#endif
		get_optimize_rect(frame, tk_pt, tk_sz);
		printf("init sz:  %d, %d\n", tk_sz.height, tk_sz.width);
		tk_init(frame, tk_pt, tk_sz, &track_obj, params);
		cv::rectangle(frame, cv::Point2i(tk_pt.x - tk_sz.width / 2, tk_pt.y - tk_sz.height / 2), cv::Point2i(tk_pt.x + tk_sz.width / 2, tk_pt.y + tk_sz.height / 2), CV_RGB(0, 255, 255), 2);
		cv::imshow("init_bbox", frame);
		cv::waitKey(0);
	}
	else
	{
		printf("open video fail!\n");
		return 0;
	}

	int nFrames = 1;
	int frame_num_staple = 1;
	int frame_num_mkcf = 1;
	int ilost_cnt = 0;
	while (capture.read(frame))
	{
#ifdef RESIZE
		cv::resize(frame, frame, cv::Size(640, 360));
#endif
		if (nFrames % 2 == 0)
		{
			nFrames++;
			continue;
		}
		double t1 = cv::getTickCount();
		tk_track(frame, tk_pt, tk_sz, &track_obj, params);
		//printf("%d, %d\n", tk_sz.height, tk_sz.width);
		//printf("track cost: %.4f\n", time_cost);
		int height = tk_sz.height*track_obj.scale_fator;
		int width = tk_sz.width*track_obj.scale_fator;

		int xx1 = tk_pt.x - width / 2;
		int yy1 = tk_pt.y - height / 2;
		int xx2 = tk_pt.x + width / 2;
		int yy2 = tk_pt.y + height / 2;
		//cv::Point2i detect_pt;
		//cv::Size detect_sz = tk_sz;
		//tk_detect(frame, detect_pt, detect_sz, &track_obj, params);
		if (track_obj.tk_psr>h_psr_th)
		{
			cv::rectangle(frame, cv::Point2i(tk_pt.x - width / 2, tk_pt.y - height / 2), cv::Point2i(tk_pt.x + width / 2, tk_pt.y + height / 2), CV_RGB(0, 255, 0), 2);
			ilost_cnt = 0;
		}
		else
		{
			if (ilost_cnt > 50)
			{
				cv::Point2i detect_pt;
				cv::Size detect_sz = tk_sz;
				tk_detect(frame, detect_pt, detect_sz, &track_obj, params);
				if (track_obj.dt_psr > h_psr_th)
				{
					track_obj.tk_ptf.x = detect_pt.x;
					track_obj.tk_ptf.y = detect_pt.y;
					cv::rectangle(frame, cv::Point2i(detect_pt.x - width / 2, detect_pt.y - height / 2), cv::Point2i(detect_pt.x + width / 2, detect_pt.y + height / 2), CV_RGB(0, 255, 255), 2);
					cv::waitKey(0);
				}
				else
				{
					cv::rectangle(frame, cv::Point2i(detect_pt.x - width / 2, detect_pt.y - height / 2), cv::Point2i(detect_pt.x + width / 2, detect_pt.y + height / 2), CV_RGB(255, 0, 255), 2);
				}
			}
			else
			{
				cv::rectangle(frame, cv::Point2i(tk_pt.x - width / 2, tk_pt.y - height / 2), cv::Point2i(tk_pt.x + width / 2, tk_pt.y + height / 2), CV_RGB(255, 0, 0), 2);
			}
			ilost_cnt++;
		}
		printf("frame: %d, %d, %d\n", nFrames, frame_num_mkcf, frame_num_staple);

		while ((frame_num_staple < nFrames) && getline(zb_file, line))
		{
			std::istringstream staple_coor(line);
			staple_coor >> x1 >> y1 >> x2 >> y2 >> frame_num_staple;
		}
		rectangle(frame, Point(x1/2, y1/2), Point(x2/2, y2/2), Scalar(0, 0, 255), 2, 8);

		while ((frame_num_mkcf < nFrames) && getline(mkcf_file, line))
		{
			std::istringstream mkcf_coor(line);
			mkcf_coor >> bx1 >> by1 >> bx2 >> by2 >> frame_num_mkcf;
		}
		rectangle(frame, Point(bx1 / 2, by1 / 2), Point(bx2 / 2, by2 / 2), Scalar(255, 0, 0), 2, 8);
		nFrames++;
		
		imshow(window_name, frame);
		char key = (char)waitKey(1);
		if (key == ' ')
			waitKey(0);
		if (key == 27)
			break;
	}
	return  0;

}

int just_for_test()
{
	FILE * file = fopen("./roi_mat.txt", "rb");
	if (!file)
	{
		printf("empty file\n");
		getchar();
	}
	char HEADER[5];
	int height = 0;
	int width = 0;
	int type = 0;
	fread(HEADER, 1, 5, file);
	fread(&width, sizeof(int), 1, file);
	fread(&height, sizeof(int), 1, file);
	fread(&type, sizeof(int), 1, file);
	printf("%s, %d, %d\n", HEADER, height, width);

	cv::Mat roi_mat(height, width, CV_8UC3);
	fread(roi_mat.data, sizeof(uchar), height*width * 3, file);
	cv::imshow("roi", roi_mat);
	cv::waitKey(0);
	return 0;
}

int main(int argc, char **argv)
{
	//string str;
	//if (argc <= 1)
	//{
	//	printf("plz input sequences path!\n");
	//	getline(cin, str);
	//}
	//else if (argc == 2)
	//{
	//	str=argv[1];
	//}
	//else
	//{
	//	printf("Too many arguments supplied.\n ");
	//}
	//findfile(str);
	//
	//for (int i = 0; i < res.size(); i++)
	//{
	//	test(res[i]);
	//}
	test_video();
	return 0; 
}
