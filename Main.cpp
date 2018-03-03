#include <opencv2\opencv.hpp>
#include <iostream>
#include <math.h>

#define IMAGE_WIDTH  640
#define IMAGE_HEIGHT 360
#define S (IMAGE_WIDTH/8)
#define T (0.15f)

using namespace std;
using namespace cv;

Mat imgCanny, imagen;
vector<Point2f> centroxy, centros, centros2, centros3;

void adaptiveThreshold(unsigned char* input, unsigned char* bin)
{
	unsigned long* integralImg = 0;
	int i, j;
	long sum = 0;
	int count = 0;
	int index;
	int x1, y1, x2, y2;
	int s2 = S / 2;

	// create the integral image
	integralImg = (unsigned long*)malloc(IMAGE_WIDTH*IMAGE_HEIGHT * sizeof(unsigned long*));

	for (i = 0; i<IMAGE_WIDTH; i++)
	{
		// reset this column sum
		sum = 0;

		for (j = 0; j<IMAGE_HEIGHT; j++)
		{
			index = j * IMAGE_WIDTH + i;

			sum += input[index];
			if (i == 0)
				integralImg[index] = sum;
			else
				integralImg[index] = integralImg[index - 1] + sum;
		}
	}

	// perform thresholding
	for (i = 0; i<IMAGE_WIDTH; i++)
	{
		for (j = 0; j<IMAGE_HEIGHT; j++)
		{
			index = j * IMAGE_WIDTH + i;

			// set the SxS region
			x1 = i - s2; x2 = i + s2;
			y1 = j - s2; y2 = j + s2;

			// check the border
			if (x1 < 0) x1 = 0;
			if (x2 >= IMAGE_WIDTH) x2 = IMAGE_WIDTH - 1;
			if (y1 < 0) y1 = 0;
			if (y2 >= IMAGE_HEIGHT) y2 = IMAGE_HEIGHT - 1;

			count = (x2 - x1)*(y2 - y1);

			// I(x,y)=s(x2,y2)-s(x1,y2)-s(x2,y1)+s(x1,x1)
			sum = integralImg[y2*IMAGE_WIDTH + x2] -
				integralImg[y1*IMAGE_WIDTH + x2] -
				integralImg[y2*IMAGE_WIDTH + x1] +
				integralImg[y1*IMAGE_WIDTH + x1];

			if ((long)(input[index] * count) < (long)(sum*(1.0 - T)))
				bin[index] = 0;
			else
				bin[index] = 255;
		}
	}

	free(integralImg);
}

float distancia(float ax, float ay, float bx, float by)
{
	float dist = sqrt(pow(ax - bx, 2) + pow(ay - by, 2));
	return dist;
}

void encontrarContorno() {
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;
	
	// Encontrar contornos
	findContours(imgCanny, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));
	
	int c2 = 0;
	// Encontrar elipses rotados para cada contorno	
	vector<RotatedRect> minEllipse(contours.size());	
			
	for (int i = 0; i < contours.size(); i++)
	{
		if (contours[i].size() > 5)
		{
			minEllipse[i] = fitEllipse(Mat(contours[i]));
			c2++;
		}
	}	
	cout << "anillos ant: " << c2 << endl;

	int c = 0;

	// Filtrar elipses y dibujarlos  
	Mat drawing = Mat::zeros(imgCanny.size(), CV_8UC3);
	for (int i = 0; i< contours.size(); i++)
	{
		float alto = minEllipse[i].size.height;
		float ancho = minEllipse[i].size.width;
		float dif = abs(alto - ancho);
		float dist = 0, dist2 = 0;
		float prop = minEllipse[i].size.width / minEllipse[i].size.height;
		float dif_antx = 0, dif_anty = 0, dif_sigx = 0, dif_sigy = 0;		
		if (i > 0) {
			float ax = minEllipse[i].center.x;
			float ay = minEllipse[i].center.y;
			float bx = minEllipse[i-1].center.x;
			float by = minEllipse[i-1].center.y;
			dist = distancia(ax, ay, bx, by);
			dif_antx = abs(minEllipse[i].center.x - minEllipse[i - 1].center.x);
			dif_anty = abs(minEllipse[i].center.y - minEllipse[i - 1].center.y);
		} 
		if (i < contours.size()-1) {
			float ax = minEllipse[i].center.x;
			float ay = minEllipse[i].center.y;
			float bx = minEllipse[i+1].center.x;
			float by = minEllipse[i+1].center.y;
			dist2 = distancia(ax, ay, bx, by);
			dif_sigx = abs(minEllipse[i].center.x - minEllipse[i + 1].center.x);
			dif_sigy = abs(minEllipse[i].center.y - minEllipse[i + 1].center.y);
		}

		if (dist > 2 and dist2 > 2)
			continue;
		
		//if (contours[i].size()>5 and contourArea(contours[i])>5 and dif<10 and prop >= 0.6)
		if (contours[i].size()>5 and contourArea(contours[i])>35 and dif<10 and prop >= 0.6)
		//if (contours[i].size()>5 and contourArea(contours[i])>35 and dif < 3 and prop >= 0.8)
		{				
			Scalar color = Scalar(255, 255, 255);			
			ellipse(drawing, minEllipse[i], color, 2, 8);	
			circle(drawing, minEllipse[i].center, 1, Scalar(0, 255, 0), 1);
			centroxy.push_back(Point2f(minEllipse[i].center.x, minEllipse[i].center.y));
			cout << "center " << minEllipse[i].center << " " << contourArea(contours[i]) << " " << dif << " " << contours[i].size() << "  " << prop <<endl;
			c++;		
		}
	}
	cout << "anillos: " << c << endl;
	cout << "centroxy[i].x: " << centroxy[0].x << endl;
	cout << "centroxy: " << centroxy.size() << endl;
	
	//Hallar los centros 
	int cent = 0;
	for (int i = 0; i < centroxy.size()-3; i+=4) {
		cout << "centroxy[i].x: " << centroxy[i].x << "   " << centroxy[i].y << endl;
		float promx = (centroxy[i].x + centroxy[i+1].x + centroxy[i+2].x + centroxy[i+3].x) / 4;
		float promy = (centroxy[i].y + centroxy[i+1].y + centroxy[i+2].y + centroxy[i+3].y) / 4;				
		centros.push_back(Point2f(promx, promy));				
		cent++;	
	}
	cout << "cent: " << cent << endl;
	
	//Ordenar los centros Forma1
	int acum = 0;
	for (int i = 0; i < 5; i++) {
		for (int j = 5; j >= 0; j--) {
			centros2.push_back(Point2f(centros[j+acum]));
			cout << "centros2: " << centros[j + acum].x << "   " << centros[j + acum].y << endl;
		}
		acum += 6;
	}
	
	//Ordenar los centros Forma2	
	
	for (int i = 29; i >= 0; i--) {
		centros3.push_back(Point2f(centros[i]));
		cout << "centros3: " << centros[i].x << "   " << centros[i].y << endl;		
	}
	
	namedWindow("contornos", CV_WINDOW_AUTOSIZE);
	imshow("contornos", drawing);
}

int main()
{
	IplImage* img = cvLoadImage("Frame0.jpg", 1);
	IplImage* imgGray;
	Mat imgBlur;
	imagen = cvarrToMat(img);
		
	imgGray = cvCreateImage(cvSize(img->width, img->height), IPL_DEPTH_8U, 1);
	cvCvtColor(img, imgGray, CV_BGR2GRAY);
	
	Mat gray = cvarrToMat(imgGray);
	GaussianBlur(gray, imgBlur, Size(3, 3), 0, 0, 0); //Aplicando filtro gausiano
	
	IplImage* iplImg = cvCloneImage(&(IplImage)imgBlur);
	adaptiveThreshold((unsigned char*)iplImg->imageData, (unsigned char*)iplImg->imageData);
		
	Mat imgBin = cvarrToMat(iplImg);
	Canny(imgBin, imgCanny, 10, 100, 3, true);
	namedWindow("canny", WINDOW_AUTOSIZE);
	imshow("canny", imgCanny);

	encontrarContorno();
	
	//Trazado de líneas en forma manual
	/*
	for (int i = 0; i < 30; i++) {		
		if (i>=0 and i<6){
			circle(imagen, centros3[i], 3, Scalar(255, 0, 0), 2, LINE_AA);
			line(imagen, centros3[i], centros3[i + 1], Scalar(255, 0, 0));
		}
		else if (i >= 6 and i < 12) {
			circle(imagen, centros3[i], 3, Scalar(0, 255, 0), 2, LINE_AA);
			line(imagen, centros3[i], centros3[i + 1], Scalar(0, 255, 0));
		}
		else if (i >= 12 and i < 18) {
			circle(imagen, centros3[i], 3, Scalar(255, 255, 0), 2, LINE_AA);
			line(imagen, centros3[i], centros3[i + 1], Scalar(255, 255, 0));
		}
		else if (i >= 18 and i < 24) {
			circle(imagen, centros3[i], 3, Scalar(0, 255, 255), 2, LINE_AA);
			line(imagen, centros3[i], centros3[i + 1], Scalar(0, 255, 255));
		}
		else if (i >= 24 and i < 29) {
			circle(imagen, centros3[i], 3, Scalar(0, 0, 255), 2, LINE_AA);
			line(imagen, centros3[i], centros3[i + 1], Scalar(0, 0, 255));
		}
		if (i == 29)
			circle(imagen, centros3[i], 3, Scalar(0, 0, 255), 1);
	}
	*/
		
	int ind = 0;
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 6; j++) {
			ostringstream s1;
			s1 << "(" << i << "," << j << ")";
			putText(imagen, s1.str(), centros3[ind], FONT_HERSHEY_SIMPLEX, 0.4, Scalar(0, 255, 0), 1.0);
			ind++;
		}
	}
			
	//Otra Forma		
	Size patternsize = Size(6, 5); //Patron de anillos
	drawChessboardCorners(imagen, patternsize, Mat(centros3), 1);
		
	namedWindow("Imagen", WINDOW_AUTOSIZE);
	imshow("Imagen", imagen);

	waitKey(0);
	cvDestroyAllWindows();
	cvReleaseImage(&img);
	cvReleaseImage(&imgGray);
	cvReleaseImage(&iplImg);

	return 0;
}

