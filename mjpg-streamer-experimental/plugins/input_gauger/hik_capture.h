#ifndef JLAB_HI_CAPTURE_H
#define JLAB_HI_CAPTURE_H
#include <memory>
#include <MvCameraControl.h>
#include <opencv2/opencv.hpp>

namespace jlab
{
    class HikCapture
    {
    public:
        HikCapture();
        ~HikCapture();
        bool open(int vcapId);
        bool open(const std::string &vcapName);
        void close();
        bool set(int propId, double value);
        bool grab();
        bool retrieve(cv::Mat &img);
        bool read(cv::Mat &img);

    private:
        MV_CC_DEVICE_INFO_LIST mStDeviceList = {0};
        void *mHandle = NULL;
        MVCC_INTVALUE mStParam;
        unsigned char *mData;
        int mDataSize;

    private:
        bool PrintDeviceInfo(MV_CC_DEVICE_INFO* pstMVDevInfo);
    };
} // namespace jlab

#endif
