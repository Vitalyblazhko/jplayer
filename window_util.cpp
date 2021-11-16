#include "window_util.h"
#include <opencv2/highgui.hpp>
#include <X11/Xlib.h>

#define FONT_THICKNESS    1.5
#define TEXT_LINE_SPACING 2

#if (GTK_MAJOR_VERSION == 3)
#define GTK_VERSION3 1
#endif //GTK_MAJOR_VERSION == 3

const std::string LEGEND_WINDOW_NAME = "Legend";
const std::string STITCH_WINDOW_NAME = "JPlayer";

int gDesktopBorderLeft;
int gDesktopBorderTop;
int gDesktopWorkAreaH;
int gDesktopWorkAreaV;

int WindowUtil::getDesktopBorderLeft()
{
    return gDesktopBorderLeft;
}

int WindowUtil::getDesktopBorderTop()
{
    return gDesktopBorderTop;
}

int WindowUtil::getDesktopWorkAreaH()
{
    return gDesktopWorkAreaH;
}

int WindowUtil::getDesktopWorkAreaV()
{
    return gDesktopWorkAreaV;
}

void gtkCallback(GtkWidget *widget)
{
    gtk_window_get_position((GtkWindow *)widget, &gDesktopBorderLeft, &gDesktopBorderTop);
    gtk_window_get_size((GtkWindow *)widget, &gDesktopWorkAreaH, &gDesktopWorkAreaV);

    gtk_widget_destroy(widget);
    gtk_main_quit();
}

void getDesktopWorkArea(int argc, char **argv)
{
    GtkWidget *window;
    gtk_init(&argc, &argv);
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    #if defined(GTK_VERSION3)
        gtk_widget_set_opacity(window, 0.0);
    #else
        gtk_window_set_opacity((GtkWindow *)window, 0.0);
    #endif //GTK_VERSION3
    gtk_window_set_decorated((GtkWindow *)window, (gboolean)FALSE);
    gtk_window_maximize((GtkWindow *)window);
    gtk_widget_show(window);

    g_signal_connect(window, "map-event", G_CALLBACK(gtkCallback), NULL);
    gtk_main();
}

WindowUtil::WindowUtil(int argc, char **argv)
{
    getDesktopWorkArea(argc, argv);
}

WindowUtil::~WindowUtil(){}

int WindowUtil::getWindowTitleHeight()
{
    Display *display = XOpenDisplay(0);
    Window window, root;
    Atom a, t;
    int s = DefaultScreen(display);
    int f;
    unsigned long n, b;
    unsigned char *data = 0;
    long *extents;
    XEvent e;

    root = DefaultRootWindow(display);
    window = XCreateSimpleWindow(display, root, 0, 0, 200, 200, 0, BlackPixel(display, s), WhitePixel(display, s));
    XSelectInput(display, window, ExposureMask|ButtonPressMask|KeyPressMask|PropertyChangeMask);

    XMapWindow(display, window);

    a = XInternAtom(display, "_NET_FRAME_EXTENTS", True);

    while (XGetWindowProperty(display, window, a, 0, 4, False, AnyPropertyType, &t, &f, &n, &b, &data) !=
        Success || n != 4 || b != 0)
    {
        XNextEvent(display, &e);
    }

    extents = (long*) data;

    XCloseDisplay(display);
    XFree(data);

    return extents[2];
}

std::string WindowUtil::generateWindowName(int winNumber, std::string fileName)
{
    return "Window " + std::to_string(winNumber + 1) + ": " + fileName;
}

std::string WindowUtil::getStitchWindowName()
{
    return STITCH_WINDOW_NAME;
}

void WindowUtil::createLegendWindow()
{
    cv::Mat frame = cv::Mat(gDesktopWorkAreaV / 5, gDesktopWorkAreaH / 4, CV_8UC3, CV_RGB(240, 240, 240));

    std::string legendText = "Space     - Toggle play/pause\n"
                             ".         - Forward one frame\n"
                             ",         - Backward one frame\n"
                             "a/A       - Forward 10 frames\n"
                             "l/L       - Toggle Legend window\n"
                             "Esc       - Exit\n";

    WindowUtil::putText(frame, legendText, 0, cv::Scalar(0));

    cv::imshow(LEGEND_WINDOW_NAME, frame);
}

void WindowUtil::closeLegendWindow()
{
    if (cv::getWindowProperty(LEGEND_WINDOW_NAME, CV_WND_PROP_AUTOSIZE) >= 0)
    {
        cv::destroyWindow(LEGEND_WINDOW_NAME);
    }
}

void WindowUtil::putText(const cv::Mat &frame, std::string &text, int textPositionY, cv::Scalar textColor)
{
    int textIndent = frame.cols - (frame.cols * 0.95);

    std::istringstream issText(text);
    std::string line;

    double fontScale = 0.5;
    int baseLine = 0;

    while(std::getline(issText, line, '\n'))
    {
        cv::Size lineSize = cv::getTextSize(line, CV_FONT_HERSHEY_SIMPLEX, fontScale, FONT_THICKNESS, &baseLine);
        while((textIndent + lineSize.width) > (frame.cols - textIndent))
        {
            fontScale *= 0.95;
            lineSize = cv::getTextSize(line, CV_FONT_HERSHEY_SIMPLEX, fontScale, FONT_THICKNESS, &baseLine);
        }

        textPositionY += lineSize.height * TEXT_LINE_SPACING;
        cv::putText(frame, line, cv::Point(textIndent, textPositionY), cv::FONT_HERSHEY_SIMPLEX, fontScale,
            textColor, FONT_THICKNESS);
        fontScale = 0.5;
    }
}