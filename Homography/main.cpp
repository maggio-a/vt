#include <opencv2/opencv.hpp>

#include <vector>
#include <iostream>

using cv::Mat;

cv::Point2f transformPoint(cv::Point2f src, Mat transform) {
	std::vector<double> tmp(3);
	tmp[0] = src.x; tmp[1] = src.y; tmp[2] = 1.0;

	Mat pt(tmp);
	Mat dst = transform * pt;

	return cv::Point2f(dst.at<double>(0,0) / dst.at<double>(0,2),
	                   dst.at<double>(0,1) / dst.at<double>(0,2));
}

int main(int argc, char *argv[]) {

	Mat image = cv::imread("../floor.jpg");

	std::vector<cv::Point2f> img_quad;
	img_quad.push_back(cv::Point2f(205.0, 539.0));
	//img_quad.push_back(cv::Point2f(520.0, 576.0));
	//img_quad.push_back(cv::Point2f(563.0, 320.0));
	//img_quad.push_back(cv::Point2f(289.0, 289.0));
	img_quad.push_back(cv::Point2f(834.0, 612.0));
	img_quad.push_back(cv::Point2f(836.0, 153.0));
	img_quad.push_back(cv::Point2f(355.0, 96.0));

	for(unsigned int i=0; i<img_quad.size(); ++i) {
        cv::circle(image, img_quad[i], 5, cv::Scalar(0,0,50*i));
    }

	std::vector<cv::Point2f> world_quad;
	world_quad.push_back(cv::Point2f(0.0, 0.0));
	world_quad.push_back(cv::Point2f(648.0, 0.0));
	world_quad.push_back(cv::Point2f(648.0, 648.0 ));
	world_quad.push_back(cv::Point2f(0.0, 648.0));

	Mat img2world = cv::getPerspectiveTransform(img_quad, world_quad);
	Mat world2img = img2world.inv();

	cv::Point2f tpoint = transformPoint(cv::Point2f(324.0, 324.0), world2img);
    cv::circle(image, tpoint, 5, cv::Scalar(255,255,0));

    cv::Point2f tpoint2 = transformPoint(cv::Point2f(197.0, 172.0), img2world);
    std::cout << tpoint2 << std::endl;

    cv::imwrite("../floor_points.png", image);

	return 0;
}