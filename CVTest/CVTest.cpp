#pragma once

// Include GLEW
#include <GL/glew.h>

#include <iostream>
#include <random>
#include <Windows.h>
#include <stdio.h>
#include <math.h>
#include <opencv2/opencv.hpp>
#include <portaudio.h>

#define ILUT_USE_OPENGL
#include <IL/il.h>
#include <IL/ilu.h>
#include <IL/ilut.h>

#include "GLWindow.h"

#include <GL/GLU.h>

// Include GLFW
#include <GLFW/glfw3.h>

// Include GLM
#include <glm/glm.hpp>
using namespace glm;

#include <filesystem>

namespace fs = std::filesystem;

#include "IMGUI/imgui_impl_opengl3.h"
#include "IMGUI/imgui_stdlib.h"
#include "IMGUI/imgui_impl_glfw.h"




#define NUM_SECONDS   (5)
#define SAMPLE_RATE   (44100)
#define FRAMES_PER_BUFFER  (64)

#ifndef M_PI
#define M_PI  (3.14159265)
#endif

#define TABLE_SIZE   (200)

class Sine
{
public:
    Sine() : stream(0), left_phase(0), right_phase(0)
    {
        /* initialise sinusoidal wavetable */
        for (int i = 0; i < TABLE_SIZE; i++)
        {
            sine[i] = (float)sin(((double)i / (double)TABLE_SIZE) * M_PI * 2.);
        }

        printf(message, "No Message");
    }

    bool open(PaDeviceIndex index)
    {
        PaStreamParameters outputParameters;

        outputParameters.device = index;
        if (outputParameters.device == paNoDevice) {
            return false;
        }

        const PaDeviceInfo* pInfo = Pa_GetDeviceInfo(index);
        if (pInfo != 0)
        {
            printf("Output device name: '%s'\r", pInfo->name);
        }

        outputParameters.channelCount = 2;       /* stereo output */
        outputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
        outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
        outputParameters.hostApiSpecificStreamInfo = NULL;

        PaError err = Pa_OpenStream(
            &stream,
            NULL, /* no input */
            &outputParameters,
            SAMPLE_RATE,
            paFramesPerBufferUnspecified,
            paClipOff,      /* we won't output out of range samples so don't bother clipping them */
            &Sine::paCallback,
            this            /* Using 'this' for userData so we can cast to Sine* in paCallback method */
        );

        if (err != paNoError)
        {
            /* Failed to open stream to device !!! */
            return false;
        }

        err = Pa_SetStreamFinishedCallback(stream, &Sine::paStreamFinished);

        if (err != paNoError)
        {
            Pa_CloseStream(stream);
            stream = 0;

            return false;
        }

        return true;
    }

    bool close()
    {
        if (stream == 0)
            return false;

        PaError err = Pa_CloseStream(stream);
        stream = 0;

        return (err == paNoError);
    }


    bool start()
    {
        if (stream == 0)
            return false;

        PaError err = Pa_StartStream(stream);

        return (err == paNoError);
    }

    bool stop()
    {
        if (stream == 0)
            return false;

        PaError err = Pa_StopStream(stream);

        return (err == paNoError);
    }

private:
    /* The instance callback, where we have access to every method/variable in object of class Sine */
    int paCallbackMethod(const void* inputBuffer, void* outputBuffer,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags)
    {
        float* out = (float*)outputBuffer;
        unsigned long i;

        (void)timeInfo; /* Prevent unused variable warnings. */
        (void)statusFlags;
        (void)inputBuffer;

        for (i = 0; i < framesPerBuffer; i++)
        {
            *out++ = sine[left_phase];  /* left */
            *out++ = sine[right_phase];  /* right */
            left_phase += 1;
            if (left_phase >= TABLE_SIZE) left_phase -= TABLE_SIZE;
            right_phase += 3; /* higher pitch so we can distinguish left and right. */
            if (right_phase >= TABLE_SIZE) right_phase -= TABLE_SIZE;
        }

        return paContinue;

    }

    /* This routine will be called by the PortAudio engine when audio is needed.
    ** It may called at interrupt level on some machines so don't do anything
    ** that could mess up the system like calling malloc() or free().
    */
    static int paCallback(const void* inputBuffer, void* outputBuffer,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void* userData)
    {
        /* Here we cast userData to Sine* type so we can call the instance method paCallbackMethod, we can do that since
           we called Pa_OpenStream with 'this' for userData */
        return ((Sine*)userData)->paCallbackMethod(inputBuffer, outputBuffer,
            framesPerBuffer,
            timeInfo,
            statusFlags);
    }


    void paStreamFinishedMethod()
    {
        printf("Stream Completed: %s\n", message);
    }

    /*
     * This routine is called by portaudio when playback is done.
     */
    static void paStreamFinished(void* userData)
    {
        return ((Sine*)userData)->paStreamFinishedMethod();
    }

    PaStream* stream;
    float sine[TABLE_SIZE];
    int left_phase;
    int right_phase;
    char message[20];
};

using namespace cv;

BITMAPINFOHEADER createBitmapHeader(int width, int height)
{
	BITMAPINFOHEADER  bi;
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = width;
	bi.biHeight = -height;  
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;

	return bi;
}

Mat captureScreenMat(HWND hwnd)
{
	Mat src;

	// get handles to a device context (DC)
	HDC hwindowDC = GetDC(hwnd);
	HDC hwindowCompatibleDC = CreateCompatibleDC(hwindowDC);
	SetStretchBltMode(hwindowCompatibleDC, COLORONCOLOR);

	// define scale, height and width
	int scale = 1;
	int screenx = GetSystemMetrics(SM_XVIRTUALSCREEN);
	int screeny = GetSystemMetrics(SM_YVIRTUALSCREEN);
	int width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	int height = GetSystemMetrics(SM_CYVIRTUALSCREEN);

	// create mat object
	src.create(height, width, CV_8UC4);

	// create a bitmap
	HBITMAP hbwindow = CreateCompatibleBitmap(hwindowDC, width, height);
	BITMAPINFOHEADER bi = createBitmapHeader(width, height);

	// use the previously created device context with the bitmap
	SelectObject(hwindowCompatibleDC, hbwindow);

	// copy from the window device context to the bitmap device context
	StretchBlt(hwindowCompatibleDC, 0, 0, width, height, hwindowDC, screenx, screeny, width, height, SRCCOPY);  //change SRCCOPY to NOTSRCCOPY for wacky colors !
	GetDIBits(hwindowCompatibleDC, hbwindow, 0, height, src.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);            //copy from hwindowCompatibleDC to hbwindow

	// avoid memory leak
	DeleteObject(hbwindow);
	DeleteDC(hwindowCompatibleDC);
	ReleaseDC(hwnd, hwindowDC);

	return src;
}

class ScopedPaHandler
{
public:
    ScopedPaHandler()
        : _result(Pa_Initialize())
    {
    }
    ~ScopedPaHandler()
    {
        if (_result == paNoError)
        {
            Pa_Terminate();
        }
    }

    PaError result() const { return _result; }

private:
    PaError _result;
};

void BindCVMat2GLTexture(cv::Mat& image, GLuint& imageTexture)
{
    if (image.empty()) {
        std::cout << "image empty" << std::endl;
    }
    else {
        //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
        glGenTextures(1, &imageTexture);
        glBindTexture(GL_TEXTURE_2D, imageTexture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Set texture clamping method
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

        cv::cvtColor(image, image, cv::COLOR_BGR2RGB);

        glTexImage2D(GL_TEXTURE_2D,         // Type of texture
            0,                   // Pyramid level (for mip-mapping) - 0 is the top level
            GL_RGB,              // Internal colour format to convert to
            image.cols,          // Image width  i.e. 640 for Kinect in standard mode
            image.rows,          // Image height i.e. 480 for Kinect in standard mode
            0,                   // Border width in pixels (can either be 1 or 0)
            GL_RGB,              // Input image format (i.e. GL_RGB, GL_RGBA, GL_BGR etc.)
            GL_UNSIGNED_BYTE,    // Image data type
            image.ptr());        // The actual image data itself
    }
}

void DataFunct(GLWindow* wind) {

}

void SetupFunct(GLWindow* wind) {
    glEnable(GL_DEPTH_TEST); // Depth Testing
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);
    glCullFace(GL_BACK);
}

void KeyHandler(GLFWwindow* window, int key, int scancode, int action, int mods) {

    switch (key)
    {
    case GLFW_KEY_LEFT:

        break;
    default:
        break;
    }
}

void UpdateFunct(GLWindow* wind) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    glMatrixMode(GL_MODELVIEW_MATRIX);
    glLoadIdentity();
    glClearColor(0, 0, 0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

Sine sine;
VideoCapture cap(0);//Declaring an object to capture stream of frames from default camera//
Mat src_Desktop;
Mat src_Camera;
HWND Desktop_Window;
GLuint Desktop_Texture=0;
GLuint Cam_Texture=1;

bool shouldPlay = false;
std::string btnname = "Play";

void DrawFunct(GLWindow* wind) {





    //if (!cap.isOpened()) { //This section prompt an error message if no video stream is found//
    //    std::cout << "No video stream detected" << std::endl;
    //    system("pause");
    //    return-1;
    //}



    if ((char)cv::waitKey(25) == 27) {
        wind->SetShouldRun(false);
    }

    if (ImGui::Begin("Audio")) {
        if (ImGui::Button(btnname.c_str())) {
            if (shouldPlay) {
                sine.start();
                btnname = "Stop";
                shouldPlay = !shouldPlay;
            }
            else {
                sine.stop();
                btnname = "Play";
                shouldPlay = !shouldPlay;
            }
        }
    }
    ImGui::End();

    if(ImGui::Begin("Desktop")) {
        // capture desktop
        Desktop_Window = GetDesktopWindow();
        src_Desktop = captureScreenMat(Desktop_Window);
        BindCVMat2GLTexture(src_Desktop, Desktop_Texture);
        ImGui::Image((void*)Desktop_Texture, ImVec2(src_Desktop.cols,src_Desktop.rows));
    }
    ImGui::End();
    if(ImGui::Begin("Cam")) {
        //capture camera
        cap >> src_Camera;
        BindCVMat2GLTexture(src_Camera, Cam_Texture);
        ImGui::Image((void*)Cam_Texture, ImVec2(src_Camera.cols, src_Camera.rows));
    }
    ImGui::End();

    //imshow("Desktop", src_Desktop);
    //imshow("Camera", src_Camera);

    // Render dear imgui into screen
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

int main()
{
    srand(time(0));
    // Initialise all DevIL functionality
    ilutRenderer(ILUT_OPENGL);
    ilInit();
    iluInit();
    ilutInit();
    ilutRenderer(ILUT_OPENGL);    // Tell DevIL that we're using OpenGL for our rendering

    //init portaudio
    ScopedPaHandler paInit;
    if (paInit.result() != paNoError) {
        fprintf(stderr, "An error occurred while using the portaudio stream\n");
        fprintf(stderr, "Error number: %d\n", paInit.result());
        fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(paInit.result()));
        return 1;
    }

    //init glwindow
    GLWindow wind;
    wind.SetDPS(0);
    wind.SetFPS(0);
    wind.SetKeyHandle(&KeyHandler);
    wind.SetSetupFunct(&SetupFunct);
    wind.SetUpdateFunct(&UpdateFunct);
    wind.SetDrawFunct(&DrawFunct);
    //win.SetDataFunct(&DataFunct);
    wind.Init();


    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(wind.GetWindow(), true);
    ImGui_ImplOpenGL3_Init("#version 130");
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    if (sine.open(Pa_GetDefaultOutputDevice()))
    {
        wind.Loop();
    }
    sine.close();
    return paNoError;
}