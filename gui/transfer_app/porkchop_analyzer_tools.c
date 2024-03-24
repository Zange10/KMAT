#include "porkchop_analyzer_tools.h"
#include <stdlib.h>



void swap_arr(double *a, double *b) {
	double temp = *a;
	*a = *b;
	*b = temp;
}

void swap_porkchop(double *a, double *b) {
	double temp[5];
	for(int i = 0; i < 5; i++) temp[i] = a[i];
	for(int i = 0; i < 5; i++) a[i] = b[i];
	for(int i = 0; i < 5; i++) b[i] = temp[i];
}

void swap_arrivals(struct ItinStep **a, struct ItinStep **b) {
	struct ItinStep *temp = *a;
	*a = *b;
	*b = temp;
}

void swap(double arr[], int a_ind, int b_ind, double *porkchop, struct ItinStep **arrivals) {
	swap_arr(&arr[a_ind], &arr[b_ind]);
	swap_porkchop(&porkchop[5*a_ind+1], &porkchop[5*b_ind+1]);
	swap_arrivals(&arrivals[a_ind], &arrivals[b_ind]);
}

int partition(double arr[], int low, int high, double *porkchop, struct ItinStep **arrivals) {
	double pivot = arr[high];
	int i = (low - 1);

	for (int j = low; j <= high - 1; j++) {
		if (arr[j] < pivot) {
			i++;
			swap(arr, i, j, porkchop, arrivals);
		}
	}
	swap(arr, i+1, high, porkchop, arrivals);
	return (i + 1);
}

void quicksort_porkchop_and_arrivals(double *arr, int low, int high, double *porkchop, struct ItinStep **arrivals) {
	if (low < high) {
		int pi = partition(arr, low, high, porkchop, arrivals);

		quicksort_porkchop_and_arrivals(arr, low, pi - 1, porkchop, arrivals);
		quicksort_porkchop_and_arrivals(arr, pi + 1, high, porkchop, arrivals);
	}
}



void reset_porkchop_and_arrivals(const double *all_porkchop, double *porkchop, struct ItinStep **all_arrivals, struct ItinStep **arrivals, int num_itins) {
	for(int i = 0; i <= all_porkchop[0]; i++) {
		porkchop[i] = all_porkchop[i];
	}
	for(int i = 0; i < num_itins; i++) {
		arrivals[i] = all_arrivals[i];
	}
}

void filter_porkchop_arrivals_depdate(double *porkchop, struct ItinStep **arrivals) {

}
