#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <linux/videodev2.h>

#include "../../utils.h"
#include "../../mjpg_streamer.h"

#include "cmd.h"
#include "Gauger.h"
#include "gauger_observer.h"

extern globals *pglobal;
TARGETPARAM targetParam;

struct _control vst_param[] = {
	{
        {CMD_TARGET, V4L2_CTRL_TYPE_BUTTON, "初始化",  1, 0, 0, 0, 0, 0},
		0,
		NULL,
		0,
		0
	},
	{
        {CMD_START, V4L2_CTRL_TYPE_BUTTON, "开始测量", 1, 0, 0, 0, 0, 0},
		0,
		NULL,
		0,
		1
	},
	{
        {CMD_STOP, V4L2_CTRL_TYPE_BUTTON, "停止测量", 1, 0, 0, 0, 0, 0},
		0,
		NULL,
		0,
		1
	},
	{
		{3, V4L2_CTRL_TYPE_BUTTON, "截图", 1, 0, 0, 0, 0, 0},
		0,
		NULL,
		0,
		1
	},
	{
        {CMD_CAL, V4L2_CTRL_TYPE_BUTTON, "计算1次", 1, 0, 0, 0, 0, 0},
		0,
		NULL,
		0,
		1
	}
};


void init_ctrl_param(output *out)
{
	out->out_parameters = vst_param;
	out->parametercount = 5;
}


CGauger* get_gauger_instance()
{
	TEMPPARAM tempParam;
	int height, width;

	char *resolution = pglobal->in[0].param.argv[4];
	sscanf(resolution, "%dx%d", &width, &height);

	memset(&targetParam, 0, sizeof(TARGETPARAM));
	memset(&tempParam, 0, sizeof(TEMPPARAM));

	targetParam.ImgWidth = width;
	targetParam.ImgHeight = height;
	printf("ImgWidth: %d, ImgHeight: %d\n", targetParam.ImgWidth, targetParam.ImgHeight);
	targetParam.SearchRoundRange = 250;
	targetParam.MinPixCenterRound = 50;
	targetParam.DistLR = 25.0;
	targetParam.DistTB = 25.0,
	targetParam.PixReducSpaceX = 1;
	targetParam.PixReducSpaceY = 1;

	tempParam.TempWidth = 20;
	tempParam.TempHeight = 20;
	tempParam.BlackPix = 165;
	tempParam.WhitePix = 30;

	return new CGauger(&targetParam, &tempParam);	
}

