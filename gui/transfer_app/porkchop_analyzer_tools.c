#include "porkchop_analyzer_tools.h"
#include "gui/drawing.h"
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

void get_min_max_dep_arr_dur_range_from_mouse_rect(double *p_x0, double *p_x1, double *p_y0, double *p_y1, double min_x_val, double max_x_val, double min_y_val, double max_y_val, double screen_width, double screen_height, int dur0arrdate1) {
	double x0 = *p_x0, x1 = *p_x1, y0 = *p_y0, y1 = *p_y1;

	int min_x = dur0arrdate1 ? get_porkchop_arrdate_yaxis_x() : get_porkchop_dur_yaxis_x();
	int min_y = get_porkchop_xaxis_y();
	
	if(x0 < min_x) x0 = min_x;
	if(x1 < min_x) x1 = min_x;
	if(x0 > screen_width) x0 = screen_width;
	if(x1 > screen_width) x1 = screen_width;

	if(y0 < 0) y0 = 0;
	if(y1 < 0) y1 = 0;
	if(y0 > screen_height-min_y) y0 = screen_height-min_y;
	if(y1 > screen_height-min_y) y1 = screen_height-min_y;

	if(x0 > x1) { double temp = x0; x0 = x1; x1 = temp;	}
	if(y0 > y1) { double temp = y0; y0 = y1; y1 = temp; }

	x0 -= min_x;
	x1 -= min_x;
	x0 /= (screen_width-min_x);
	x1 /= (screen_width-min_x);
	y0 /= (screen_height-min_y);
	y1 /= (screen_height-min_y);

	double ddate = max_x_val - min_x_val;
	double ddur = max_y_val - min_y_val;

	// below from porkchop drawing...

	double margin = 0.05;
	
	min_x_val = min_x_val - ddate*margin;
	max_x_val = max_x_val + ddate*margin;
	min_y_val = min_y_val - ddur*margin;
	max_y_val = max_y_val + ddur*margin;

	x0 = x0*(max_x_val - min_x_val) + min_x_val;
	x1 = x1*(max_x_val - min_x_val) + min_x_val;
	y0 = (1-y0)*(max_y_val - min_y_val) + min_y_val;
	y1 = (1-y1)*(max_y_val - min_y_val) + min_y_val;
	
	*p_x0 = x0, *p_x1 = x1, *p_y0 = y0, *p_y1 = y1;
}
