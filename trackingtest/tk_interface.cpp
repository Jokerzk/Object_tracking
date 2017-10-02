#include "tk_interface.h"
#include "autel_tk.h"
#include "tk_api.h"


int g_isInitialized = 0;         // Flag for Initialization and Tracking
AutelRect g_initialBox;
TrackingResult_F g_returnFunction; 
char g_trackingStatus = 0;       // Track Termination Flag
tk_data *track_obj = NULL;

tracker_log_level g_logLevel;
int SetTrackerLogLevel(tracker_log_level level)
{
  g_logLevel = level;
  return 0;
}

char buff_v[32] = "V0.0.2";

int GetTrackerLibInfo(tracker_lib_info *p_info)
{
  char buff_b[32] = "staple";
  
  memcpy(p_info->brief, buff_b, strlen(buff_b));
  memcpy(p_info->version, buff_v, strlen(buff_v));

  //Get Date
  char buff_t[32] = "2017-06-09 09:00:00";
  const char* date_0 = __DATE__; //DATE: month-day-year
  char date[256];
  strcpy(date, date_0);
  int len = strlen(date);

  int index = 0;
  int id = 0;
  for (index = 0; index < len; ++index)
  {
    if (date[index] == ' ')
    {
      date[index] = '-';
      if (id == index - 1)
      {
        date[index] = '0';
      }
      id = index;
    }
  }
  strcpy(buff_t, &date[id + 1]);
  date[id] = '\0';
  buff_t[4] = '-'; buff_t[5] = '\0';
  strcat(buff_t, date);
  memcpy(p_info->buildTime, buff_t, strlen(buff_t));

  return 0;
}

int SetTrackerArea(float x, float y, float width, float height)
{
	g_initialBox.x = x;
	g_initialBox.y = y;
	g_initialBox.width = width;
	g_initialBox.height = height;

	g_isInitialized = 2;  // Flag to Do Tracking

	return 1;
}
// Movidius --> H2
int SetMovidiusDataToH2(movidius_pose* _data)
{
	movidius_pose* data = _data;
	return 0;
}

void StopTracking()
{
	g_isInitialized = 0;
	tracking_output_app tracking_output = { 0, 0, 0, 0, 0, 0, 0 };
	g_returnFunction(TRACKER_RESULT_APP, (void*)&tracking_output, sizeof(tracking_output_app));
	tracking_output_movidius tracking_output_mv = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	g_returnFunction(TRACKER_RESULT_MOVIDIUS, (void*)&tracking_output_mv, sizeof(tracking_output_movidius));
}

int RegsiterTrackerResultInfo(TrackingResult_F p_function)
{
  g_returnFunction = p_function;
  return 0;
}


int UnRegsiterTrackerResultInfo(TrackingResult_F p_function)
{
  if (g_returnFunction == p_function)
    g_returnFunction = NULL;

  return 0;
}

tk_params params;
AutelPoint2i tk_pt;
AutelSize tk_sz;
bool bisfirst = true;
int TrackerAlgInterface(unsigned char* buffer, int width, int height, int data_format, uint64_t pts_camera)
{
	// this define the factor of scaling back and forth from normal
	//screen to tablet screen
	int data_type = 0;
    int timestampstart = 0;
    int timestampstop = 0;
	if (g_isInitialized == 0 || width == 0 || height == 0 )
		return 0;

	AutelMat image;
	image.width = width;
	image.height = height;
	image.buffer = buffer;

	AutelRect return_box = g_initialBox;

	if (g_isInitialized == 2)
	{
        if (track_obj != NULL) {
            delete track_obj;
            track_obj = NULL;
        }
        //printf("in track interface init\n");
        params.padding = 1.0;
		params.output_sigma_factor = 1 / 16.0;
		params.scale_sigma_factor = 1 / 5.0;
		params.lambda = 1e-2;
		params.learning_rate = 0.030;
		params.number_of_scales = 33;
		params.scale_step = 1.04;
		params.scale_model_max_area = 512;
		bisfirst = true;
		///
		/// INITIAL FRAME        
		/// Conversion -> box: proportion --> pixel coordinate
		///
		if (g_initialBox.width < 1 && g_initialBox.height < 1)
		{
			g_initialBox.width = g_initialBox.width * image.width;
			g_initialBox.height = g_initialBox.height * image.height;
			g_initialBox.x = g_initialBox.x * image.width;
			g_initialBox.y = g_initialBox.y * image.height;
		}
		g_isInitialized = 1;

		tk_pt.x = g_initialBox.x + g_initialBox.width / 2;;
		tk_pt.y = g_initialBox.y + g_initialBox.height / 2;

		tk_sz.width = g_initialBox.width;
		tk_sz.height = g_initialBox.height;
        track_obj = new tk_data;
		tk_init(image, tk_pt, tk_sz, track_obj, params);
	}
	else
	{
		//printf("in track interface loop\n");
		///
		/// TARGET DETECTION ON THE GIVEN FRAME USING THE KCF
		///
		timestampstart = getcurtime();
		tk_track(image, tk_pt, tk_sz, track_obj, params);
        int roi_width = tk_sz.width*track_obj->scale_fator;
        int roi_height = tk_sz.height*track_obj->scale_fator;
        timestampstop = getcurtime();
		/// Return data to APP
		///
		g_trackingStatus = 0;
		if (track_obj->tk_psr > l_psr_th)
		{
			g_trackingStatus = 1;
		}
	#ifdef PROFLING
		printf("total time cost = %d, status: %d, sscore: %f\n", timestampstop - timestampstart, g_trackingStatus, track_obj->tk_psr);
    #endif
		float startx = (tk_pt.x - roi_width / 2.0) / (float)image.width;
		float starty = (tk_pt.y - roi_height / 2.0) / (float)image.height;
		float frame_width = roi_width / (float)image.width;
		float frame_height = roi_height / (float)image.height;
        //fprintf(stderr, "psr = %f, tk_pt.x = %d, y = %d, tk_sz.width = %d, height = %d,  g_trackingStatus = %d, tk_track timevale = %d, %f, %f, %f, %f\n",
        //    track_obj->tk_psr, tk_pt.x, tk_pt.y, roi_width, roi_height, g_trackingStatus, timestampstop - timestampstart,
        //    startx, starty, frame_width, frame_height);

		if(bisfirst)
		{
			bisfirst = false;
			g_trackingStatus = 2;
		}

		tracking_output_app tracking_output = { pts_camera, startx, starty, frame_width, frame_height, g_trackingStatus, 0 };
		g_returnFunction(TRACKER_RESULT_APP, (void *)&tracking_output, sizeof(tracking_output_app));

		int stx = startx*image.width;
		int sty = starty*image.height;
		int stw = roi_width;
		int sth = roi_height;

		tracking_output_movidius tracking_output_mv = {pts_camera, stx, sty, stw, sth, 0, 0, 0, 0, g_trackingStatus};
		g_returnFunction(TRACKER_RESULT_MOVIDIUS, (void*)&tracking_output_mv, sizeof(tracking_output_movidius));
	}

	return 1;
}

extern "C" const TrackerInterface tracker = { TRACKER_INTERFACE_VERSION, "tracker", SetTrackerArea, StopTracking, RegsiterTrackerResultInfo, UnRegsiterTrackerResultInfo, 
TrackerAlgInterface, SetMovidiusDataToH2, GetTrackerLibInfo, SetTrackerLogLevel };