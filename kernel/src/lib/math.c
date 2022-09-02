#include <math.h>

double pow(double n1, double n2){
	n2--;
	if(n2 <= 0) return 1;
	for(uint16_t i = 0;i<n2;i++) n1*=n1;
	return n1;
}

uint64_t powi(uint64_t num, uint16_t count){
	count--;
	if(count == 0) return 1;
	for(uint16_t i = 0;i<count;i++) num*=num;
	return num;
}

double abs(double Val) {
	if(Val < 0) Val *= -1;

	return Val;
}