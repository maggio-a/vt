#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <cv.h>
#include <highgui.h>
#include <RaspiCamCV.h>

using namespace std;
using namespace cv;

/*int main(int argc, const char** argv){

    RaspiCamCvCapture * capture = raspiCamCvCreateCameraCapture(0);
    IplImage* image = NULL;
    IplImage* gray = NULL;
    IplImage* canny = NULL;
    IplImage* rgbcanny = NULL;
    CvMemStorage* storage = NULL;
    CvSeq* circles = NULL;
    cvNamedWindow("RaspiCamTest", 1);
        image = raspiCamCvQueryFrame(capture);

        gray = cvCreateImage(cvGetSize(image), IPL_DEPTH_8U, 1);
        storage = cvCreateMemStorage(0);

        cvCvtColor(image, gray, CV_BGR2GRAY);

        // This is done so as to prevent a lot of false circles from being detected
        cvSmooth(gray, gray, CV_GAUSSIAN, 7, 7);

        canny = cvCreateImage(cvGetSize(image),IPL_DEPTH_8U,1);
        rgbcanny = cvCreateImage(cvGetSize(image),IPL_DEPTH_8U,3);
        cvCanny(gray, canny, 50, 100, 3);

        circles = cvHoughCircles(gray, storage, CV_HOUGH_GRADIENT, 1, gray->height/3, 250, 100);
        cvCvtColor(canny, rgbcanny, CV_GRAY2BGR);

        for (size_t i = 0; i < circles->total; i++)
            {
                 // round the floats to an int
                 float* p = (float*)cvGetSeqElem(circles, i);
                 cv::Point center(cvRound(p[0]), cvRound(p[1]));
                 int radius = cvRound(p[2]);

                 // draw the circle center
                 cvCircle(rgbcanny, center, 3, CV_RGB(0,255,0), -1, 8, 0 );

                 // draw the circle outline
                 cvCircle(rgbcanny, center, radius+1, CV_RGB(0,0,255), 2, 8, 0 );

                 printf("x: %d y: %d r: %d\n",center.x,center.y, radius);
            }

        cvNamedWindow("circles", 1);
        cvShowImage("circles", rgbcanny);

    cvDestroyWindow("circles");
    raspiCamCvReleaseCapture(&capture);
    return 0;
}

IplImage* GetThresholdedImage(IplImage* imgHSV){
       IplImage* imgThresh=cvCreateImage(cvGetSize(imgHSV),IPL_DEPTH_8U, 1);
       cvInRangeS(imgHSV, cvScalar(170,160,60), cvScalar(180,256,256), imgThresh);
       return imgThresh;
}*/
/*
int main()
{
    RaspiCamCvCapture* capture =0;

       capture = raspiCamCvCreateCameraCapture(0);
       if(!capture){
             printf("Capture failure\n");
             return -1;
       }

       IplImage* frame=0;

       cvNamedWindow("Video");
       cvNamedWindow("Red");

       //iterate through each frames of the video
       while(true){

             frame = raspiCamCvQueryFrame(capture);
             if(!frame) break;

             frame=cvCloneImage(frame);
             Mat imgMat(raspiCamCvQueryFrame(capture));

             cvSmooth(frame, frame, CV_GAUSSIAN,3,3); //smooth the original image using Gaussian kernel

             IplImage* imgHSV = cvCreateImage(cvGetSize(frame), IPL_DEPTH_8U, 3);
             cvCvtColor(frame, imgHSV, CV_BGR2HSV); //Change the color format from BGR to HSV
             IplImage* imgThresh = GetThresholdedImage(imgHSV);
             cvSmooth(imgThresh, imgThresh, CV_GAUSSIAN,3,3); //smooth the binary image using Gaussian kernel


             for(int i = 0; i < imgMat.rows; i++)
             {
                 for(int j = 0; j < imgMat.cols; j++)
                 {
                     Vec3b &bgrPixel = imgMat.at<Vec3b>(i, j);

                     if (bgrPixel.val[2] > 200) {
                         cout << "delfocacca" << endl;
                         bgrPixel.val[0] = 255;
                         bgrPixel.val[1] = 255;
                         bgrPixel.val[2] = 255;
                     }

                     else
                         bgrPixel.val[0] = 0;
                         bgrPixel.val[1] = 0;
                         bgrPixel.val[2] = 0;
                 }
             }

             IplImage* boh = &imgMat;
             cvShowImage("Red", boh);
             cvShowImage("Video", frame);

             //Clean up used images
             //cvReleaseImage(&imgHSV);
             //cvReleaseImage(&imgThresh);
             cvReleaseImage(&frame);

             //Wait 50ms
             int c = cvWaitKey(10);
             //If 'ESC' is pressed, break the loop
             if((char)c==27 ) break;
       }

       cvDestroyAllWindows() ;
       raspiCamCvReleaseCapture(&capture);

       return 0;
 }

int main() {
  RaspiCamCvCapture * camera = raspiCamCvCreateCameraCapture(0);

  namedWindow("red", CV_WINDOW_AUTOSIZE);
  namedWindow("binary", CV_WINDOW_AUTOSIZE);
  while(true){
      Mat imgMat(raspiCamCvQueryFrame(camera));
      Mat binMat = imgMat.clone();
      for(int i = 0; i < imgMat.rows; i++)
                   {
                       for(int j = 0; j < imgMat.cols; j++)
                       {
                           Vec3b &bgrPixel = imgMat.at<Vec3b>(i, j);
                           Vec3b &binPixel = binMat.at<Vec3b>(i, j);

                           if ((binPixel.val[2] > 100) && (binPixel.val[0] < 150) && (binPixel.val[1] < 150)) {
                               binPixel.val[0] = 255;
                               binPixel.val[1] = 255;
                               binPixel.val[2] = 255;
                           }

                           else {
                               binPixel.val[0] = 0;
                               binPixel.val[1] = 0;
                               binPixel.val[2] = 0;
                           }

                           bgrPixel.val[1] = 0;
                           bgrPixel.val[0] = 0;
                       }
                   }
      imshow("red", imgMat);
      imshow("binary", binMat);
      int c = waitKey(10);
      //If 'ESC' is pressed, break the loop
      if((char)c==27 ) break;
  }

  cvDestroyAllWindows() ;
  raspiCamCvReleaseCapture(&camera);

  return 0;
}
*/
