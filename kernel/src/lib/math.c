#include <math.h>

// double pow(double n1, double n2){
// 	double ret = n1;
// 	for(uint16_t i = 0;i<n2-1;i++) ret = ret*n2;
// 	return ret;
// }

uint64_t powi(uint64_t num, uint16_t count){
	count--;
	if(count == 0) return 1;
	for(uint16_t i = 0;i<count;i++) num*=num;
	return num;
}