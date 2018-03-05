#include <opencv2\opencv.hpp>
#include <iostream>
#include <math.h>

#define IMAGE_WIDTH  640
#define IMAGE_HEIGHT 360
//#define IMAGE_HEIGHT 480
#define S (IMAGE_WIDTH/8)
#define T (0.15f)

using namespace std;
using namespace cv;

Mat drawing;
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

int colineal(float ax, float ay, float bx, float by, float cx, float cy)
{
	float distAB = sqrt(pow(ax - bx, 2) + pow(ay - by, 2));
	float distBC = sqrt(pow(bx - cx, 2) + pow(by - cy, 2));
	float distAC = sqrt(pow(ax - cx, 2) + pow(ay - cy, 2));
	if ((distAB + distBC) == distAC) {
		return 1;
	}
	return 0;
}

void encontrarContorno()
{
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;

	// Encontrar contornos
	findContours(imgCanny, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));

	// Encontrar elipses rotados para cada contorno	
	vector<RotatedRect> minEllipse(contours.size());

	for (int i = 0; i < contours.size(); i++)
	{
		if (contours[i].size() > 5)
		{
			minEllipse[i] = fitEllipse(Mat(contours[i]));
		}
	}

	// Filtrar elipses y dibujarlos  
	drawing = Mat::zeros(imgCanny.size(), CV_8UC3);
	Scalar color = Scalar(255, 255, 255);
	float alto, ancho, dif, ax, ay, bx, by;
	float distAnt = 0, distSig = 0;
	int cont, conta;

	for (int i = 0; i< contours.size(); i++)
	{
		alto = minEllipse[i].size.height;
		ancho = minEllipse[i].size.width;
		dif = abs(alto - ancho);
		
		if (contours[i].size()>5 and contourArea(contours[i])>5 and dif < 10)
		{
			ax = minEllipse[i].center.x;
			ay = minEllipse[i].center.y;
			if (i > 0)
			{
				bx = minEllipse[i - 1].center.x;
				by = minEllipse[i - 1].center.y;
				distAnt = distancia(ax, ay, bx, by);
			}
			if (i < contours.size() - 1)
			{
				bx = minEllipse[i + 1].center.x;
				by = minEllipse[i + 1].center.y;
				distSig = distancia(ax, ay, bx, by);
			}
			if (distAnt > 2)
			{
				cont = 0;
				conta = 0;
				for (int j = i + 1; j < i + 4; j++) {
					bx = minEllipse[j].center.x;
					by = minEllipse[j].center.y;
					if (distancia(ax, ay, bx, by) < 2 and contours[j].size()>5 and contourArea(contours[j])>5)
						cont++;
					if (contourArea(contours[j])>1)
						conta++;
				}
				if (cont < 2 || conta<3)
					continue;
			}

			if (distSig > 2) {
				cont = 0;
				conta = 0;
				for (int j = i - 1; (j > 0 and j > i - 4); j--) {
					bx = minEllipse[j].center.x;
					by = minEllipse[j].center.y;
					if (distancia(ax, ay, bx, by) < 2 and contours[j].size()>5 and contourArea(contours[j])>5)
						cont++;
					if (contourArea(contours[j])>1)
						conta++;
				}
				if (cont < 2 || conta<3)
					continue;
			}

			ellipse(drawing, minEllipse[i], color, 2, 8);
			circle(drawing, minEllipse[i].center, 1, Scalar(0, 255, 0), 1);
			centroxy.push_back(Point2f(minEllipse[i].center.x, minEllipse[i].center.y));
			cout << "center " << minEllipse[i].center << " " << contourArea(contours[i]) << " " << distAnt << " " << distSig << " " << dif << " " << i << endl;
		}
	}

	cout << "centroxy: " << centroxy.size() << endl;

	//Hallar los centros 
	int cent, acum = 1;
	float promx, promy;
	for (int i = 0; i < centroxy.size(); i += acum)
	{
		acum = 1;
		cent = 0;
		promx = centroxy[i].x;
		promy = centroxy[i].y;
		for (int j = i + 1; j < centroxy.size(); j++)
		{
			if (distancia(centroxy[i].x, centroxy[i].y, centroxy[j].x, centroxy[j].y) <= 2)
			{
				promx = (promx + centroxy[j].x) / 2;
				promy = (promy + centroxy[j].y) / 2;
				cent++;
			}
		}
		acum += cent;
		//cout << "centros: " << promx << "   " << promy << endl;
		centros.push_back(Point2f(promx, promy));
	}
	cout << "centros: " << centros.size() << endl;

	//Otra Forma de trazado de líneas		
	vector<Point2f> centros;//findCirclesGrid guarda los puntos del tablero aqui
	Size patternsize = Size(6, 5); //Patron de anillos
	//bool found = findCirclesGrid(drawing, patternsize, centros, CALIB_CB_ASYMMETRIC_GRID);
	bool found = findCirclesGrid(drawing, patternsize, centros, CALIB_CB_SYMMETRIC_GRID + CALIB_CB_CLUSTERING);
	drawChessboardCorners(imagen, patternsize, Mat(centros), found);
	namedWindow("Imagen", WINDOW_AUTOSIZE);
	imshow("Imagen", imagen);
		
	namedWindow("contornos", CV_WINDOW_AUTOSIZE);
	imshow("contornos", drawing);
}

int main()
{
	VideoCapture video("PadronAnillos_02.avi");

	Mat gray, imgBlur, imgBin;
	IplImage* img, *imgGray, *iplImg;
	
	int cont = 0;
	while (true)
	{
		video >> imagen;
		centroxy.clear();
		centros.clear();
		if (cont == 10) {
			img = cvCloneImage(&(IplImage)imagen);		
			imgGray = cvCreateImage(cvSize(img->width, img->height), IPL_DEPTH_8U, 1);
			cvCvtColor(img, imgGray, COLOR_BGR2GRAY);  
		
			gray = cvarrToMat(imgGray);
			GaussianBlur(gray, imgBlur, Size(3, 3), 0, 0, 0); //Aplicando filtro gausiano
		
			iplImg = cvCloneImage(&(IplImage)imgBlur);		
			adaptiveThreshold((unsigned char*)iplImg->imageData, (unsigned char*)iplImg->imageData);
				
			imgBin = cvarrToMat(iplImg);
			Canny(imgBin, imgCanny, 10, 100, 3, true);
			namedWindow("canny", WINDOW_AUTOSIZE);
			imshow("canny", imgCanny);
	
			encontrarContorno();			
			cont = 0;
		}
		else {
			cont++;
		}		
		waitKey(1);
	}	
	return 0;
}
