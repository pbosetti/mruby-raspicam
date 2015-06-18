#include <ctime>
#include <fstream>
#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <raspicam/raspicam_cv.h>
#include <raspicam/raspicam.h>

#include "laserCam.h"

//#define MARK_LINE printf("*** FILE: %s - LINE: %d\n", __FILE__, __LINE__)

#ifdef __cplusplus

RaspicamLaser::RaspicamLaser(int width, int height) {
  _available = false;
  _rect      = NULL;
  _camera = new raspicam::RaspiCam_Cv();
  _red_thr = 230;
  setFrameSize(width, height);
}

RaspicamLaser::~RaspicamLaser() {
  closeCamera();
  delete _rect;
  delete _camera;
}

void RaspicamLaser::set_rect(int x0, int y0, int x1, int y1) {
  if (! _rect) {
    _rect = new rect;
  }
  _rect->x0 = x0;
  _rect->y0 = y0;
  _rect->x1 = x1;
  _rect->y1 = y1;
}

void RaspicamLaser::reset_rect() {
  delete _rect;
  _rect = NULL;
}

bool RaspicamLaser::openCamera() {
  _available = _camera->open();
  return _available;
}

void RaspicamLaser::closeCamera() {
  _available = false;
  _camera->release();
}

void RaspicamLaser::setFrameSize(int width, int height) {
  // _camera.set(CV_CAP_PROP_BRIGHTNESS, 50);
  // _camera.set(CV_CAP_PROP_GAIN, 50);
  // _camera.set(CV_CAP_PROP_EXPOSURE, 80);
  
  _camera->set(CV_CAP_PROP_FRAME_WIDTH, width);
  _camera->set(CV_CAP_PROP_FRAME_HEIGHT, height);
}

bool RaspicamLaser::saveFrame(std::string &name) {
  if (_rect) {
    cv::rectangle(_lastFrame, cv::Point(_rect->x0, _rect->y0), cv::Point(_rect->x1, _rect->y1), cv::Scalar(0, 255, 0));
  }
  imwrite(name, _lastFrame);
  return true;
}

bool RaspicamLaser::acquireFrame(int slp) {
  if (!_available)
    return false;
  if (slp > 0)
    sleep(slp);
  _camera->grab();
  _camera->retrieve(_lastFrame);
  return true;
}

bool RaspicamLaser::position(int *x, int *y, int slp) {
  if (!_available)
    return false;
    acquireFrame(slp);
    *x = -1;
    *y = -1;
    const int FRAME_DIV = 50;
    int SCALE = 255;
    int square_col_size = ceil ( _lastFrame.cols / FRAME_DIV );
    int square_row_size = ceil ( _lastFrame.rows / FRAME_DIV );
    static cv::Mat sub_roi;
    const unsigned int LASER_COLOR = 2;
    int brightCondition = 0; int circlesCondition = 0;
    
    cv::Mat channels[3];
    cv::Point pMaxCh;
    cv::Vec3b intensity;
    unsigned int colors[3];
    unsigned int *c = colors;
    
    std::vector<cv::Vec3f> vecCircles;
    std::vector<cv::Vec3f>::iterator itrCircles;
    
    // Laser detector
    for ( register int row = 0; row < _lastFrame.rows - square_row_size; row += square_row_size ){
        for ( register int col = 0; col < _lastFrame.cols - square_col_size; col +=  square_col_size ){
            sub_roi = _lastFrame ( cv::Rect ( col, row, square_col_size, square_row_size ) );
            cv::split( sub_roi, channels );
            cv::minMaxLoc ( channels[ LASER_COLOR ], NULL, NULL, NULL, &pMaxCh );
            
            intensity = sub_roi.at < cv::Vec3b > ( pMaxCh.y, pMaxCh.x );
            *( c ) = static_cast < int > ( intensity.val [ 0 ] );
            *( c + 1 ) = static_cast < int > ( intensity.val [ 1 ] );
            *( c + 2 ) = static_cast < int > ( intensity.val [ 2 ] );
            
            if ( *(c + 2 ) > _red_thr &&  *( c ) < *( c + 2 ) - 20  && *( c + 1 ) < *( c +  2 ) - 20 ){   //230 -20 -20
                cv::cvtColor ( sub_roi, sub_roi, CV_BGR2GRAY );
                cv::GaussianBlur( sub_roi, sub_roi, cv::Size(9, 9), 1, 1 );
                cv::HoughCircles( sub_roi, vecCircles, CV_HOUGH_GRADIENT, 1, ceil (sub_roi.rows / 2.0 ) , 70, 3, 1, 10);
                
                if ( vecCircles.size() > 0 ){
                    cv::Point p ( col + pMaxCh.x, row + pMaxCh.y);
                    cv::circle ( _lastFrame, p, square_col_size, cv::Scalar ( 0, 255, 0 ) );
                    *x = (p.x) * SCALE / _lastFrame.cols;
                    *y = ( _lastFrame.rows - ( p.y )) * SCALE / _lastFrame.rows;
                }
            }
        }
    }
  if (*x == -1 || *y == -1)
    return false;
  return true;
}

#endif







#pragma mark -
/*
              ____   ___       _             __
             / ___| |_ _|_ __ | |_ ___ _ __ / _| __ _  ___ ___
            | |      | || '_ \| __/ _ \ '__| |_ / _` |/ __/ _ \
            | |___   | || | | | ||  __/ |  |  _| (_| | (_|  __/
             \____| |___|_| |_|\__\___|_|  |_|  \__,_|\___\___|

*/
#define RASPICAM_CLASS(o) reinterpret_cast<RaspicamLaser *>(o)

CRaspicamLaser newCRaspicamLaser(int width, int height) {
  return reinterpret_cast<void *>(new RaspicamLaser(width, height));
}

void delCRaspicamLaser(CRaspicamLaser rl) { delete RASPICAM_CLASS(rl); }

//
// Each public method. Takes an opaque reference to the object
// that was returned from the above constructor plus the methods parameters.
int CRaspicamLaserOpenCamera(CRaspicamLaser rl) {
  return RASPICAM_CLASS(rl)->openCamera() ? 0 : -1;
}

void CRaspicamLaserCloseCamera(CRaspicamLaser rl) {
  RASPICAM_CLASS(rl)->closeCamera();
}

void CRaspicamLaserSetRect(CRaspicamLaser rl, int x0, int y0, int x1, int y1) {
  RASPICAM_CLASS(rl)->set_rect(x0, y0, x1, y1);
}

void CRaspicamLaserResetRect(CRaspicamLaser rl) {
  RASPICAM_CLASS(rl)->reset_rect();
}

int CRaspicamLaserPosition(CRaspicamLaser rl, int *x, int *y) {
  return RASPICAM_CLASS(rl)->position(x, y) ? 0 : -1;
}

int CRaspicamLaserAvailable(CRaspicamLaser rl) {
  return RASPICAM_CLASS(rl)->available();
}

void CRaspicamLaserSetFrameSize(CRaspicamLaser rl, int width, int height) {
  RASPICAM_CLASS(rl)->setFrameSize(width, height);
}

unsigned int CRaspicamLaserRedThreshold(CRaspicamLaser rl) {
  return RASPICAM_CLASS(rl)->red_threshold();
}


void CRaspicamLaserSetRedThreshold(CRaspicamLaser rl, unsigned int thr) {
  RASPICAM_CLASS(rl)->set_red_threshold(thr);
}



int CRaspicamLaserSaveFrame(CRaspicamLaser rl, const char *cname, int slp) {
  // if (!RASPICAM_CLASS(rl)->available())
  //   return -1;
  // std::string name = cname;
  // RASPICAM_CLASS(rl)->saveFrame(name, slp);
  // return 0;
  std::string name = cname;
  int x = 0, y = 0;
  RASPICAM_CLASS(rl)->position(&x, &y, slp);
  return RASPICAM_CLASS(rl)->saveFrame(name) ? 0 : -1;
}
