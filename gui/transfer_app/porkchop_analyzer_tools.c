#include "porkchop_analyzer_tools.h"
#include <stdlib.h>



void swap_arr(double *a, double *b) {
	double temp = *a;
	*a = *b;
	*b = temp;
}

void swap_porkchop(struct PorkchopAnalyzerPoint *porkchop, int a_ind, int b_ind) {
	struct PorkchopAnalyzerPoint temp;
	temp = porkchop[a_ind];
	porkchop[a_ind] = porkchop[b_ind];
	porkchop[b_ind] = temp;
}

void swap(double arr[], int a_ind, int b_ind, struct PorkchopAnalyzerPoint *porkchop) {
	swap_arr(&arr[a_ind], &arr[b_ind]);
	swap_porkchop(porkchop, a_ind, b_ind);
}

int partition(double arr[], int low, int high, struct PorkchopAnalyzerPoint *porkchop) {
	int pivot_index = (low + high) / 2;	// choosing pivot in middle reduces time for already sorted list dramatically
	double pivot = arr[pivot_index];
	int i = (low - 1);

	// Move pivot to end of array
	swap(arr, pivot_index, high, porkchop);

	for (int j = low; j <= high - 1; j++) {
		if (arr[j] < pivot) {
			i++;
			swap(arr, i, j, porkchop);
		}
	}
	swap(arr, i+1, high, porkchop);
	return (i + 1);
}

void quicksort_porkchop(double *arr, int low, int high, struct PorkchopAnalyzerPoint *pps) {
	if (low < high) {
		int pi = partition(arr, low, high, pps);

		quicksort_porkchop(arr, low, pi - 1, pps);
		quicksort_porkchop(arr, pi + 1, high, pps);
	}
}

void sort_porkchop(struct PorkchopAnalyzerPoint *pp, int num_itins, enum LastTransferType last_transfer_type) {
	double *dvs = (double*) malloc(num_itins*sizeof(double));
	for(int i = 0; i < num_itins; i++)  {
		dvs[i] = pp[i].data.dv_dep + pp[i].data.dv_dsm;
		if(last_transfer_type == TF_CAPTURE) dvs[i] += pp[i].data.dv_arr_cap;
		if(last_transfer_type == TF_CIRC) dvs[i] += pp[i].data.dv_arr_circ;
	}

	quicksort_porkchop(dvs, 0, num_itins - 1, pp);
	free(dvs);
}

void get_min_max_dep_dur_range_from_mouse_rect(double *dep0, double *dep1, double *dur0, double *dur1, double min_dep, double max_dep, double min_dur, double max_dur, double screen_width, double screen_height) {
	double x0 = *dep0, x1 = *dep1, y0 = *dur0, y1 = *dur1;

	if(x0 < 45) x0 = 45;
	if(x1 < 45) x1 = 45;
	if(x0 > screen_width) x0 = screen_width;
	if(x1 > screen_width) x1 = screen_width;

	if(y0 < 0) y0 = 0;
	if(y1 < 0) y1 = 0;
	if(y0 > screen_height-40) y0 = screen_height-40;
	if(y1 > screen_height-40) y1 = screen_height-40;

	if(x0 > x1) { double temp = x0; x0 = x1; x1 = temp;	}
	if(y0 > y1) { double temp = y0; y0 = y1; y1 = temp; }

	x0 -= 45;
	x1 -= 45;
	x0 /= (screen_width-45);
	x1 /= (screen_width-45);
	y0 /= (screen_height-40);
	y1 /= (screen_height-40);

	double ddate = max_dep-min_dep;
	double ddur = max_dur-min_dur;

	// below from porkchop drawing...

	double margin = 0.05;

	min_dep = min_dep-ddate*margin;
	max_dep = max_dep+ddate*margin;
	min_dur = min_dur - ddur * margin;
	max_dur = max_dur + ddur * margin;

	x0 = x0*(max_dep-min_dep)+min_dep;
	x1 = x1*(max_dep-min_dep)+min_dep;
	y0 = (1-y0)*(max_dur-min_dur)+min_dur;
	y1 = (1-y1)*(max_dur-min_dur)+min_dur;

	*dep0 = x0, *dep1 = x1, *dur0 = y0, *dur1 = y1;
}
