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
	double pivot = arr[high];
	int i = (low - 1);

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
