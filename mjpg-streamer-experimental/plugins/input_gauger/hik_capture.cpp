#include "hik_capture.h"

namespace jlab
{
    HikCapture::HikCapture() : mData(NULL)
    {
    }

    HikCapture::~HikCapture()
    {
        if (mHandle != NULL)
        {
            MV_CC_DestroyHandle(mHandle);
            mHandle = NULL;
        }

        // ch:反初始化SDK | en:Finalize SDK
        //MV_CC_Finalize();
    }

    bool HikCapture::open(int vcapId)
    {
        int nRet = -1;
//        nRet = MV_CC_Initialize();
//        if (MV_OK != nRet)
//        {
//            printf("Initialize SDK fail! nRet [0x%x]\n", nRet);
//            return false;
//        }

        // 枚举子网内指定的传输协议对应的所有设备
        unsigned int nTLayerType = MV_GIGE_DEVICE | MV_USB_DEVICE;
        nRet = MV_CC_EnumDevices(nTLayerType, &mStDeviceList);
        if (MV_OK != nRet)
        {
            printf("error: EnumDevices fail [%x]\n", nRet);
            return false;
        }


        if (mStDeviceList.nDeviceNum > 0)
        {
            for (int i = 0; i < mStDeviceList.nDeviceNum; i++)
            {
                printf("[device %d]:\n", i);
                MV_CC_DEVICE_INFO* pDeviceInfo = mStDeviceList.pDeviceInfo[i];
                if (NULL == pDeviceInfo)
                {
                    break;
                }
                PrintDeviceInfo(pDeviceInfo);
            }
        }
        else
        {
            printf("Find No Devices!\n");
            return false;
        }


        //MV_CC_DEVICE_INFO stDevInfo = {0};
        //memcpy(&stDevInfo, mStDeviceList.pDeviceInfo[vcapId], sizeof(MV_CC_DEVICE_INFO));
        nRet = MV_CC_CreateHandle(&mHandle, mStDeviceList.pDeviceInfo[vcapId]);
        if (MV_OK != nRet)
        {
            printf("error: CreateHandle fail [%x]\n", nRet);
            return false;
        }
        // 连接设备
        //unsigned int nAccessMode = MV_ACCESS_Exclusive;
        //unsigned short nSwitchoverKey = 0;
        nRet = MV_CC_OpenDevice(mHandle/*, nAccessMode, nSwitchoverKey*/);
        if (MV_OK != nRet)
        {
            printf("error: OpenDevice fail [%x]\n", nRet);
            return false;
        }

        MV_CC_StartGrabbing(mHandle);

        nRet = MV_OK;
        // ch:获取数据包大小 | en:Get payload size
        memset(&mStParam, 0, sizeof(MVCC_INTVALUE));
        nRet = MV_CC_GetIntValue(mHandle, "PayloadSize", &mStParam);
        if (MV_OK != nRet)
        {
            printf("Get PayloadSize fail! nRet [0x%x]\n", nRet);
            return NULL;
        }

        mDataSize = mStParam.nCurValue;

        mData = (unsigned char *)malloc(sizeof(unsigned char) * mStParam.nCurValue);
        if (NULL == mData)
        {
            return false;
        }
        return true;
    }

    void HikCapture::close()
    {
        if (mData != NULL)
            free(mData);
        MV_CC_StopGrabbing(mHandle);
        int nRet = MV_CC_CloseDevice(mHandle);
        if (MV_OK != nRet)
        {
            printf("error: CloseDevice fail [%x]\n", nRet);
        }

        // 销毁句柄，释放资源
        nRet = MV_CC_DestroyHandle(mHandle);
        if (MV_OK != nRet)
        {
            printf("error: DestroyHandle fail [%x]\n", nRet);
        }
        mHandle = NULL;
    }

    bool HikCapture::open(const std::string &vcapName)
    {
        int vId = std::stoi(vcapName);
        return open(vId);
    }

    bool HikCapture::set(int propId, double value)
    {
        return true;
    }

    bool HikCapture::grab()
    {
        return true;
    }

    bool HikCapture::retrieve(cv::Mat &img)
    {
        return true;
    }

    static void frameToMatrix(unsigned char *data, int size, MV_FRAME_OUT_INFO_EX &imgInfo, cv::Mat &mtr)
    {
        //mtr = cv::imdecode(cv::Mat(1, size, CV_8UC1, data), cv::IMREAD_COLOR);
        mtr = cv::Mat(imgInfo.nHeight, imgInfo.nWidth, CV_8UC1, data);
    }

    bool HikCapture::read(cv::Mat &img)
    {
        MV_FRAME_OUT_INFO_EX stImageInfo = {0};
        memset(&stImageInfo, 0, sizeof(MV_FRAME_OUT_INFO_EX));
        auto ret = MV_CC_GetOneFrameTimeout(mHandle, mData, mDataSize, &stImageInfo, 20);
        if (ret == MV_OK)
        {
//            printf("GetOneFrame, Width[%d], Height[%d], nFrameNum[%d]\n",
//                   stImageInfo.nWidth, stImageInfo.nHeight, stImageInfo.nFrameNum);

            frameToMatrix(mData, mDataSize, stImageInfo, img);
            return true;
        }
        else
        {
            printf("No data[%x]\n", ret);
            return false;
        }
    }

    bool HikCapture::PrintDeviceInfo(MV_CC_DEVICE_INFO* pstMVDevInfo)
    {
        if (NULL == pstMVDevInfo)
        {
            printf("The Pointer of pstMVDevInfo is NULL!\n");
            return false;
        }
        if (pstMVDevInfo->nTLayerType == MV_GIGE_DEVICE)
        {
            int nIp1 = ((pstMVDevInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0xff000000) >> 24);
            int nIp2 = ((pstMVDevInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x00ff0000) >> 16);
            int nIp3 = ((pstMVDevInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x0000ff00) >> 8);
            int nIp4 = (pstMVDevInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x000000ff);

            // ch:打印当前相机ip和用户自定义名字 | en:print current ip and user defined name
            printf("Device Model Name: %s\n", pstMVDevInfo->SpecialInfo.stGigEInfo.chModelName);
            printf("CurrentIp: %d.%d.%d.%d\n" , nIp1, nIp2, nIp3, nIp4);
            printf("UserDefinedName: %s\n\n" , pstMVDevInfo->SpecialInfo.stGigEInfo.chUserDefinedName);
        }
        else if (pstMVDevInfo->nTLayerType == MV_USB_DEVICE)
        {
            printf("Device Model Name: %s\n", pstMVDevInfo->SpecialInfo.stUsb3VInfo.chModelName);
            printf("UserDefinedName: %s\n\n", pstMVDevInfo->SpecialInfo.stUsb3VInfo.chUserDefinedName);
        }
        else
        {
            printf("Not support.\n");
        }

        return true;
    }
} // namespace jlab
