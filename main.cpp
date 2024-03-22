//g++ video.cpp -o app -lcurl  -lwiringPi  $(pkg-config --cflags --libs opencv)


#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>

#include <iostream>
#include <cmath>
#include <string>
#include <curl/curl.h>

using namespace cv;
using namespace std;

struct Circle {
	int x, y, r;
};

struct Pointc {
	double x, y;
};

// Callback function pour écrire les données reçues dans une chaîne de caractères
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp){
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}


bool sendHttp(){
  // Initialise la bibliothèque libcurl
  curl_global_init(CURL_GLOBAL_DEFAULT);
  
  // Crée une session curl
  CURL* curl = curl_easy_init();
  
  // Définit l'URL de la requête HTTP GET
  std::string url = "http://192.168.1.48/cnt";
  
  // Définit le callback pour écrire les données reçues dans une chaîne de caractères
  std::string response;
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
  
  // Définit l'URL de la requête HTTP GET
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  
  // Envoie la requête HTTP GET et reçoit la réponse
  CURLcode res = curl_easy_perform(curl);
  
  // Vérifie si la requête a réussi
  if(res != CURLE_OK)
  {
	  std::cerr << "Échec de la requête HTTP GET : " << curl_easy_strerror(res) << std::endl;
  }
  else
  {
	  // Affiche la réponse reçue
	  std::cout << "Réponse reçue : " << response << std::endl;
  }
  
  // Nettoie la session curl
  curl_easy_cleanup(curl);
  
  // Termine la bibliothèque libcurl
  curl_global_cleanup();
  
  return true;
}

Pointc getCollisionPoint(const Circle& c1, const Circle& c2) {
	double d = std::sqrt(std::pow(c1.x - c2.x, 2) + std::pow(c1.y - c2.y, 2));
	if (d > c1.r + c2.r) {
		//throw std::runtime_error("Les cercles ne sont pas en collision");
	}
	double a = (c1.r * c1.r - c2.r * c2.r + d * d) / (2 * d);
	double h = std::sqrt(c1.r * c1.r - a * a);
	double x3 = c1.x + a * (c2.x - c1.x) / d;
	double y3 = c1.y + a * (c2.y - c1.y) / d;
	double x4 = x3 + h * (c2.y - c1.y) / d;
	double y4 = y3 - h * (c2.x - c1.x) / d;
	return {x4, y4};
}


int main(int argc,char ** argv)
{

  Circle outer  = {320, 240, 170};
  Circle outer2 = {320, 240, 230};
  
  int score=1;
  string filename = "video.mp4";
  VideoCapture cap(0);
  if (!cap.isOpened()) {
	cerr << "ERROR: Unable to open the camera" << endl;
	return 0;
  }

  Mat frame;
  cout << "Start grabbing, press a key on Live window to terminate" << endl;
  while(1) {
	cap >> frame;
	
	Mat gray;
	Mat src = frame;
	cvtColor(src, gray, COLOR_BGR2GRAY);
	medianBlur(gray, gray, 5);
	vector<Vec3f> circles;
	HoughCircles(gray, circles, HOUGH_GRADIENT, 1,
				 gray.rows/16,  // change this value to detect circles with different distances to each other
				 100, 30, 20, 40 // change the last two parameters
			// (min_radius & max_radius) to detect larger circles
	);
	for( size_t i = 0; i < circles.size(); i++ )
	{
		Vec3i c = circles[i];
		Point center = Point(c[0], c[1]);
		// circle outline
		int radius = c[2];
		Circle inner = {c[0], c[1], c[2]};
		try {
			Pointc p = getCollisionPoint(outer2, inner);
			if (std::isnan(p.x)) {
				//std::cout << "x est NaN" << std::endl;
				try {
					Pointc pp = getCollisionPoint(outer, inner);
					if (std::isnan(pp.x)) {
						//std::cout << "pp x est NaN" << std::endl;
					} else {
						std::cout << "PP Le point de collision est (" << pp.x << ", " << pp.y << ")" << std::endl;
						std::cout << "count : "<< score++ << std::endl;
						sendHttp();
					}
				} catch (const std::runtime_error& e) {
					//std::cout << e.what() << std::endl;
				}
				
				
			} else {
				std::cout << "P Le point de collision est (" << p.x << ", " << p.y << ")" << std::endl;
			}	
		} catch (const std::runtime_error& e) {
			//std::cout << e.what() << std::endl;
		}
		circle( src, center, 1, Scalar(0,100,100), 3, LINE_AA);
		circle( src, center, radius, Scalar(255,0,255), 3, LINE_AA);
	}
	Point center = Point(320, 240);
	circle( src, center, 170, Scalar(255,0,255), 3, LINE_AA);
	circle( src, center, 230, Scalar(255,0,255), 3, LINE_AA);
	imshow("detected circles", src);
	if (frame.empty()) {
		cerr << "ERROR: Unable to grab from the camera" << endl;
		break;
	}
	//imshow("Live",frame);
	int key = cv::waitKey(20);
	key = (key==255) ? -1 : key;
	if (key>=0)
	  break;
  }

  cout << "Closing the camera" << endl;
  cap.release();
  destroyAllWindows();
  cout << "bye!" <<endl;
  return 0;
}