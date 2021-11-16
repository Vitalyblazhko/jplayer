#pragma once

#ifndef _WINDOWUTIL_H_
#define _WINDOWUTIL_H_

#include <gtk/gtk.h>
#include <iostream>
#include <opencv2/opencv.hpp>

class WindowUtil
{
public:
    WindowUtil(int argc, char **argv);
    ~WindowUtil();

    std::string generateWindowName(int winNumber, std::string fileName);
    void createLegendWindow();
    std::string getStitchWindowName();
    void closeLegendWindow();
    void putText(const cv::Mat &frame, std::string &text, int textPositionY, cv::Scalar textColor);
    int getWindowTitleHeight();
    int getDesktopBorderLeft();
    int getDesktopBorderTop();
    int getDesktopWorkAreaH();
    int getDesktopWorkAreaV();
};

#endif //_WINDOWUTIL_H_
