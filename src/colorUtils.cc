#include<iostream>
#include<cmath> // Needed for fmod()
using namespace std;

/*
 * H(Hue): 0 - 360 degree (integer)
 * S(Saturation): 0 - 1.00 (double)
 * V(Value): 0 - 1.00 (double)
 * 
 * output[3]: Output, array size 3, int
 */
class colorUtils
{
   public :
      static void hsvToRgb(int H, double S, double V, int* rgb);
} ;

void colorUtils::hsvToRgb(int H, double S, double V, int* rgb) {
	double C = S * V;
	double X = C * (1 - abs(fmod(H / 60.0, 2) - 1));
	double m = V - C;
	double Rs, Gs, Bs;

	if(H >= 0 && H < 60) {
		Rs = C;
		Gs = X;
		Bs = 0;	
	}
	else if(H >= 60 && H < 120) {	
		Rs = X;
		Gs = C;
		Bs = 0;	
	}
	else if(H >= 120 && H < 180) {
		Rs = 0;
		Gs = C;
		Bs = X;	
	}
	else if(H >= 180 && H < 240) {
		Rs = 0;
		Gs = X;
		Bs = C;	
	}
	else if(H >= 240 && H < 300) {
		Rs = X;
		Gs = 0;
		Bs = C;	
	}
	else {
		Rs = C;
		Gs = 0;
		Bs = X;	
	}
	rgb[0] = (Rs + m) * 255;
	rgb[1] = (Gs + m) * 255;
	rgb[2] = (Bs + m) * 255;
}