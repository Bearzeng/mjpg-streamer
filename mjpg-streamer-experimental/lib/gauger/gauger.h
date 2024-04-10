#ifndef GAUGER_H
#define GAUGER_H

#include <vector>
#include <opencv2/opencv.hpp>

typedef struct tagPointI {
    int x;
    int y;
    int z;
} POINT_I;

typedef struct tagPointF {
    float x;
    float y;
    float z;
} POINT_F;

using POINTS_I = std::vector<POINT_I>;
using POINTS_F = std::vector<POINT_F>;

class Gauger
{
public:
    Gauger();

    /**
     * @brief initTarget 初始化目标
     * @param mat [IN] 目标图像
     * @param initial_pts [OUT] 返回识别到的目标坐标
     * @return 目标识别成功返回true
     */
    bool initTarget(const cv::Mat &mat, POINTS_I &initial_pts);

    /**
     * @brief measureTarget 计算与原点的位移值，单位毫米
     * @param mat [IN] 图像
     * @param initial_pts [IN]  目标点原始位置
     * @param offsets [OUT]  对应目标点偏移值 [mm]
     * @return 计算成功返回true
     */
    bool measureTarget(const cv::Mat &mat, const POINTS_I &initial_pts, POINTS_F &offsets);
};

#endif // GAUGER_H
