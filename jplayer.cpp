#include <chrono>
#include <thread>
#include "window_util.h"

struct Options {
    int nextFrame = 1;
    int fps = 0;
    bool pause = false;
    bool stitch = false;
    std::vector<std::string> mediaFiles;
} inputOptions;

void printUsage(const std::string &name = "./jplayer")
{
    std::cout << std::endl;
    std::cout << "Run player in the following way: " << name << " file_1 ... file_n [arguments]" << std::endl;
    std::cout << std::endl;
    std::cout << "Allowed arguments:" << std::endl;
    std::cout << "\t-s=<value> [ --start_frame=<value> ]        Start playing from frame number" << std::endl;
    std::cout << "\t-f=<value> [ --fps=<value> ]                FPS rate" << std::endl;
    std::cout << "\t-p [ --pause ]                              Pause" << std::endl;
    std::cout << "\t-t [ --stitch ]                             Show in stitch mode" << std::endl;
    std::cout << "\t-h [ --help ]                               Show help message" << std::endl;
}

void parseOptions(int argc, char **argv)
{
    const std::string keys =
        "{start_frame s |1|}"
        "{fps f         |0|}"
        "{pause p       | |}"
        "{stitch t      | |}"
        "{help h        | |}";

    cv::CommandLineParser parser(argc, argv, keys);

    if (parser.has("help"))
    {
        printUsage();
        exit(0);
    }

    inputOptions.nextFrame = parser.get<int>("start_frame");
    inputOptions.fps = parser.get<int>("fps");
    inputOptions.pause = parser.has("pause");
    inputOptions.stitch = parser.has("stitch");

    for (int argNum = 1; argNum < argc; argNum++)
    {
        std::string arg = std::string(argv[argNum]);
        if (arg[0] == '-')
        {
            size_t pos = arg.find('=');
            if (pos != std::string::npos)
            {
                arg = arg.substr(0, pos);
            }
            arg.erase(std::remove(arg.begin(), arg.end(), '-'), arg.end());
            try
            {
                parser.has(arg);
            }
            catch (...)
            {
                std::cout << "Failed to parse arguments: Argument " << arg << " does not exist" << std::endl;
                printUsage();
                exit(1);
            }
        }
        else
        {
            inputOptions.mediaFiles.push_back(arg);
        }
    }

    if (!parser.check())
    {
        parser.printErrors();
        exit(1);
    }

    if (inputOptions.nextFrame <= 0)
    {
        std::cout << "Failed to parse arguments: Set start_frame value greater than 0" << std::endl;
        exit(1);
    }
    if (inputOptions.fps < 0)
    {
        std::cout << "Failed to parse arguments: Set fps value greater than or equal to 0" << std::endl;
        exit(1);
    }
    if (inputOptions.mediaFiles.size() == 0)
    {
        std::cout << "Failed to parse arguments: Pass at least one file as argument" << std::endl;
        exit(1);
    }
}

void createCapturedGrid(WindowUtil *windowUtil, std::vector<cv::VideoCapture> &captures, int columnsGrid, int rowsGrid,
    int filesNum, int frameNum, const std::vector<std::string> &filesList, std::vector<cv::Rect> &rectsList)
{
    int windowPositionX;
    int windowPositionY;

    if (inputOptions.stitch)
    {
        windowPositionX = 0;
        windowPositionY = 0;
    }
    else
    {
        windowPositionX = windowUtil->getDesktopBorderLeft();
        windowPositionY = windowUtil->getDesktopBorderTop();
    }

    int captureWidth = windowUtil->getDesktopWorkAreaH() / columnsGrid;
    int captureHeight = windowUtil->getDesktopWorkAreaV() / rowsGrid;

    int windowTitleHeight = windowUtil->getWindowTitleHeight();

    int filesCounter = 0;

    std::string windowName;

    for (int h = 0; h < rowsGrid; h++)
    {
        for (int w = 0; w < columnsGrid; w++)
        {
            if (filesCounter > filesNum - 1)
                break;

            std::string mediaFile = filesList[filesCounter];
            cv::VideoCapture capture(mediaFile);

            if (!capture.isOpened())
            {
                std::cout << "File " << mediaFile << ": Unable to open" << std::endl;
                exit(1);
            }

            if ((capture.get(CV_CAP_PROP_FRAME_COUNT) < inputOptions.nextFrame) && (inputOptions.nextFrame > 0))
            {
                std::cout << "File " << mediaFile << ": Start frame number " << inputOptions.nextFrame + 1 <<
                    " exceeds total frame count" << std::endl;
                exit(1);
            }

            capture.set(CV_CAP_PROP_POS_FRAMES, frameNum);

            windowName = windowUtil->generateWindowName(filesCounter, mediaFile);

            double captureRatio = capture.get(CV_CAP_PROP_FRAME_HEIGHT) / capture.get(CV_CAP_PROP_FRAME_WIDTH);

            if (captures.empty())
            {
                if (((double)(captureHeight - windowTitleHeight) / (double)captureWidth) > captureRatio)
                {
                    if (inputOptions.stitch)
                        captureHeight = captureWidth * captureRatio;
                    else
                        captureHeight = (captureWidth * captureRatio) + windowTitleHeight;
                }
                else if (((double)(captureHeight - windowTitleHeight) / (double)captureWidth) < captureRatio)
                {
                    if (inputOptions.stitch)
                    {
                        captureHeight -= windowTitleHeight;
                        captureWidth = captureHeight / captureRatio;
                    }
                    else
                    {
                        captureWidth = (captureHeight - windowTitleHeight) / captureRatio;
                    }
                }
            }

            if (inputOptions.stitch)
            {
                rectsList.push_back(cv::Rect(windowPositionX, windowPositionY, captureWidth, captureHeight));
            }
            else
            {
                cv::namedWindow(windowName, CV_WINDOW_NORMAL);
                cv::moveWindow(windowName, windowPositionX, windowPositionY);
                cv::resizeWindow(windowName, captureWidth, captureHeight - windowTitleHeight);
            }

            windowPositionX += captureWidth;
            filesCounter++;
            captures.push_back(capture);
        }

        if (inputOptions.stitch)
            windowPositionX = 0;
        else
            windowPositionX = windowUtil->getDesktopBorderLeft();

        windowPositionY += captureHeight;
    }
}

int setNextFrame(int &nextFrame, int framesJump, bool forward, bool &pause, std::vector<cv::VideoCapture> &captures)
{
    if (forward)
        nextFrame += framesJump;
    else
        nextFrame -= framesJump;

    for (int i = 0; i < captures.size(); i++)
        captures[i].set(CV_CAP_PROP_POS_FRAMES, nextFrame);

    pause = true;

    return nextFrame;
}

int main(int argc, char **argv)
{
    parseOptions(argc, argv);

    inputOptions.nextFrame--;
    int currentFrame = inputOptions.nextFrame;

    int mediaFilesNum = inputOptions.mediaFiles.size();

    WindowUtil mWindowUtil(argc, argv);

    int columnsMediaFiles = ceil(sqrt(mediaFilesNum));
    int rowsMediaFiles = (mediaFilesNum / columnsMediaFiles) + (((mediaFilesNum % columnsMediaFiles) == 0) ? 0 : 1);

    std::vector<cv::VideoCapture> capuresList;
    std::vector<cv::Rect> stitchRectsList;

    createCapturedGrid(&mWindowUtil, capuresList, columnsMediaFiles, rowsMediaFiles, mediaFilesNum, currentFrame,
        inputOptions.mediaFiles, stitchRectsList);

    std::string windowName;
    bool isLegendOpened = false;

    cv::Mat stitchFrame;
    if (inputOptions.stitch)
    {
        stitchFrame = cv::Mat(stitchRectsList[0].height * rowsMediaFiles, stitchRectsList[0].width * columnsMediaFiles,
            CV_MAKETYPE(8, 3), CV_RGB(100, 100, 100));
    }

    std::vector<std::string> emptyCapturesList;

    while (true)
    {
        double timeStart = (double)cv::getTickCount();

        char c = (char)cv::waitKey(1);

        if (!inputOptions.pause || (inputOptions.pause && (currentFrame == inputOptions.nextFrame)))
        {
            for (int i = 0; i < mediaFilesNum; i++)
            {
                windowName = mWindowUtil.generateWindowName(i, inputOptions.mediaFiles[i]);
                cv::Mat frame;
                cv::VideoCapture capture = capuresList[i];
                capture >> frame;

                if (!frame.empty())
                {
                    if (inputOptions.stitch)
                    {
                        cv::resize(frame, frame, cv::Size(stitchRectsList[i].width, stitchRectsList[i].height));
                        mWindowUtil.putText(frame, windowName, frame.rows * 0.8, CV_RGB(118, 185, 0));
                        frame.copyTo(cv::Mat(stitchFrame, stitchRectsList[i]));
                        cv::imshow(mWindowUtil.getStitchWindowName(), stitchFrame);
                    }
                    else
                    {
                        cv::imshow(windowName, frame);
                    }
                }
                else
                {
                    if (capture.isOpened())
                    {
                        cv::Mat emptyFrame(capture.get(CV_CAP_PROP_FRAME_HEIGHT),
                            capture.get(CV_CAP_PROP_FRAME_WIDTH), CV_8UC3, cv::Scalar(0, 0, 0));
                        if (!inputOptions.stitch)
                        {
                            cv::imshow(windowName, emptyFrame);
                        }
                        else
                        {
                            cv::resize(emptyFrame, emptyFrame,
                                cv::Size(stitchRectsList[i].width, stitchRectsList[i].height));
                            emptyFrame.copyTo(cv::Mat(stitchFrame, stitchRectsList[i]));
                            cv::imshow(mWindowUtil.getStitchWindowName(), stitchFrame);
                        }
                    }

                    if (std::find(std::begin(emptyCapturesList), std::end(emptyCapturesList), windowName) ==
                        std::end(emptyCapturesList))
                    {
                        emptyCapturesList.push_back(windowName);
                    }
                }
                // Destroys window on clicking Close for multiple windows
                if (!inputOptions.stitch && capture.isOpened() &&
                    cv::getWindowProperty(windowName, CV_WND_PROP_AUTOSIZE) != 0)
                {
                    cv::destroyWindow(windowName);
                    capture.release();
                }
            }
            currentFrame++;
        }

        if (inputOptions.pause)
            c = (char)cv::waitKey(0);

        if (c == 27) // ESC - Exit
        {
            break;
        }
        else if (c == 32) // Space - pause on/off
        {
            inputOptions.pause = !inputOptions.pause;
        }
        else if (c == 46) // . - forward one frame
        {
            inputOptions.pause = true;
            inputOptions.nextFrame = currentFrame;
        }
        else if (c == 44) // , - backward one frame
        {
            inputOptions.nextFrame = setNextFrame(currentFrame, 2, false, inputOptions.pause, capuresList);
        }
        else if (c == 97 || c == 65) // a or A - forward 10 frames
        {
            inputOptions.nextFrame = setNextFrame(currentFrame, 9, true, inputOptions.pause, capuresList);
        }
        else if (c == 108 || c == 76) // l or L - toggle Legend window
        {
            if (!isLegendOpened)
                mWindowUtil.createLegendWindow();
            else
                mWindowUtil.closeLegendWindow();
            isLegendOpened = !isLegendOpened;
        }

        if (emptyCapturesList.size() == mediaFilesNum)
            break;

        if (inputOptions.fps > 0)
        {
            while ((1000 / inputOptions.fps) > ((double)cv::getTickCount() - timeStart) / cv::getTickFrequency() * 1000)
                std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }

    for (int i = 0; i < mediaFilesNum; i++)
        capuresList[i].release();

    cv::destroyAllWindows();

    return 0;
}
