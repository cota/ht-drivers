#include <stdio.h>

int main(int argc, char *argv[])
{
	int i, j, k;
	int dvco, d1, d2;
	double fvco = 2700000;
	double freq =   10000;
	double diff;
	int mindiff = 100000;

	if (argc > 1)
		freq = atoi(argv[1]);
	for (i = 1; i <= 6; i++) {
		for (j = i; j <= 32; j++) {
			for (k = j; k <= 32; k++) {
				diff = fvco / i / j / k - freq;
				if (abs(diff) <= 0.1)
					goto found;
				if (abs(diff) < abs(mindiff)) {
					mindiff = (int)diff;
					dvco = i;
					d1 = j;
					d2 = k;
				}
			}
		}
	}
	goto notfound;
found:
	printf("found it: dvco=%d, d1=%d, d2=%d\n", i, j, k);
	return 0;
notfound:
	printf("notfound: mindiff=%d, dvco=%d, d1=%d, d2=%d\n",
		mindiff, dvco, d1, d2);
	return 1;
}
