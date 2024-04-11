#ifndef __GAUGER_TYPES_H__
#define __GAUGER_TYPES_H__

typedef unsigned char BYTE;//1
typedef unsigned short WORD;//2
typedef unsigned int DWORD;//4
typedef long LONG;//4

//位图文件头定义;  
//其中不包含文件类型信息（由于结构体的内存结构决定，  
//要是加了的话将不能正确读取文件信息）  
typedef struct  tagBITMAPFILEHEADER {
	//WORD bfType;//文件类型，必须是0x424D，即字符“BM”  
	DWORD bfSize;//文件大小  
	WORD  bfReserved1;//保留字  
	WORD  bfReserved2;//保留字  
	DWORD bfOffBits;//从文件头到实际位图数据的偏移字节数  
}BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER {
	DWORD  biSize;//信息头大小  
	DWORD  biWidth;//图像宽度  
	DWORD  biHeight;//图像高度  
	WORD   biPlanes;//位平面数，必须为1  
	WORD   biBitCount;//每像素位数  
	DWORD  biCompression; //压缩类型  
	DWORD  biSizeImage; //压缩图像大小字节数  
	DWORD  biXPelsPerMeter; //水平分辨率  
	DWORD  biYPelsPerMeter; //垂直分辨率  
	DWORD  biClrUsed; //位图实际用到的色彩数  
	DWORD  biClrImportant; //本位图中重要的色彩数  
}BITMAPINFOHEADER; //位图信息头定义  

typedef struct tagRGBQUAD {
	BYTE rgbBlue; //该颜色的蓝色分量  
	BYTE rgbGreen; //该颜色的绿色分量  
	BYTE rgbRed; //该颜色的红色分量  
	BYTE rgbReserved; //保留值  
}RGBQUAD;//调色板定义  

		 //像素信息  
typedef struct tagIMAGEDATA
{
	BYTE blue;
	//BYTE green;  
	//BYTE red;  
}IMAGEDATA, IMGDATA;

typedef struct  tagPointI {
	int x;
	int y;
}POINT_I;

typedef struct tagPointF {
	float x;
	float y;
}POINT_F;

typedef struct tagTargetParam {
	int     ImgWidth;//图像宽度
	int     ImgHeight;//图像高度
	float   DistLR;//上下标志点距离,单位mm
	float   DistTB;//左右标志点距离，单位mm
	POINT_I InitCenterPt;//初始中心点
	POINT_I LastCenterPt;//最后一个中心点
	float   Ox;
	float   Oy;
	float   Ax;
	float   Ay;
	//float   OldTx;//上一次的值
	//float   OldTy;
	float   IniTX;//初始值
	float   IniTy;
	float   Tx;
	float   Ty;//与初始点的y距离
	//float   Dx;
	//float   Dy;//与上次的y距离
	//float   D;//与初始的距离
	//float   DD;//与上次的距离
	float   MillimeterPerPixX;
	float   MillimeterPerPixY;
	//float   minSumSrcPrevious;//前一个相似程度
	//float   minSumSrcLast;//现在的相似程度
	//float   dtolSurSrc;//两个相似程度容差
					   //
	BYTE    PixReducSpaceX;//间隔几个像素作为下一个
	BYTE    PixReducSpaceY;
	//BYTE    FineSearchRange;//局部搜索范围
	int     SearchRoundRange;
	int     MinPixCenterRound;
}TARGETPARAM;

typedef struct tagTempParam {
	int     TempWidth;  //模板宽度
	int     TempHeight; //模板高度
	BYTE    BlackPix;   //黑色像素值
	BYTE    WhitePix;   //白色像素值
	double  MeanTem;    //均值
	double  SigmaTem;   //方差

}TEMPPARAM;

typedef struct tagTempMatchParam {
	//double sumsrc;
	double  sumImg;        //像素和
	double  sumSquareImg;  //像素均方值
	double  sumTemPix;     //与模板的乘积和
	//double sumTemImg;
	//double sumTemPix, sumTem_Img;
	//double DenominatorC;
	//double meanImg, sigmaImg;
	double  newMin;
	double  oldMin;
	POINT_I pt;
	int     update;//检查是否更新了
}TEMPMATCHPARAM;


#endif
