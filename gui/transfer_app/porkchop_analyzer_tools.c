#include "porkchop_analyzer_tools.h"
#include <stdlib.h>
#include "celestial_bodies.h"



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



void reset_porkchop_and_arrivals(const double *all_porkchop, double *porkchop, struct ItinStep **all_arrivals, struct ItinStep **arrivals) {
	for(int i = 0; i <= all_porkchop[0]; i++) {
		porkchop[i] = all_porkchop[i];
	}
	for(int i = 0; i < all_porkchop[0]/5; i++) {
		arrivals[i] = all_arrivals[i];
	}
}

int filter_porkchop_arrivals_depdate(double *porkchop, struct ItinStep **arrivals, double min_date, double max_date) {
	int new_num_itins = 0;
	int num_itins = (int) (porkchop[0]/5);

	if(min_date > max_date) {
		double temp = min_date;
		min_date = max_date;
		max_date = temp;
	}

	int index;
	double date;

	for(int i = 0; i < num_itins; i++) {
		index = 1+i*5;
		date = porkchop[index+0];
		if(date >= min_date && date <= max_date) {
			if(new_num_itins != i) {
				arrivals[new_num_itins] = arrivals[i];
				swap_porkchop(&porkchop[index], &porkchop[5*new_num_itins+1]);
			}
			new_num_itins++;
		}
	}

	porkchop[0] = new_num_itins*5;

	return new_num_itins;
}

int filter_porkchop_arrivals_dur(double *porkchop, struct ItinStep **arrivals, double min_dur, double max_dur) {
	int new_num_itins = 0;
	int num_itins = (int) (porkchop[0]/5);

	if(min_dur > max_dur) {
		double temp = min_dur;
		min_dur = max_dur;
		max_dur = temp;
	}

	int index;
	double dur;

	for(int i = 0; i < num_itins; i++) {
		index = 1+i*5;
		dur = porkchop[index+1];
		if(dur >= min_dur && dur <= max_dur) {
			if(new_num_itins != i) {
				arrivals[new_num_itins] = arrivals[i];
				swap_porkchop(&porkchop[index], &porkchop[5*new_num_itins+1]);
			}
			new_num_itins++;
		}
	}

	porkchop[0] = new_num_itins*5;

	return new_num_itins;
}

int filter_porkchop_arrivals_totdv(double *porkchop, struct ItinStep **arrivals, double min_dv, double max_dv, int fb0_pow1) {
	int new_num_itins = 0;
	int num_itins = (int) (porkchop[0]/5);

	if(min_dv > max_dv) {
		double temp = min_dv;
		min_dv = max_dv;
		max_dv = temp;
	}

	int index;
	double dv;

	for(int i = 0; i < num_itins; i++) {
		index = 1+i*5;
		dv = porkchop[index+2]+porkchop[index+3]+porkchop[index+4]*fb0_pow1;
		if(dv >= min_dv && dv <= max_dv) {
			if(new_num_itins != i) {
				arrivals[new_num_itins] = arrivals[i];
				swap_porkchop(&porkchop[index], &porkchop[5*new_num_itins+1]);
			}
			new_num_itins++;
		}
	}

	porkchop[0] = new_num_itins*5;

	return new_num_itins;
}

int filter_porkchop_arrivals_depdv(double *porkchop, struct ItinStep **arrivals, double min_dv, double max_dv) {
	int new_num_itins = 0;
	int num_itins = (int) (porkchop[0]/5);

	if(min_dv > max_dv) {
		double temp = min_dv;
		min_dv = max_dv;
		max_dv = temp;
	}

	int index;
	double dv;

	for(int i = 0; i < num_itins; i++) {
		index = 1+i*5;
		dv = porkchop[index+2];
		if(dv >= min_dv && dv <= max_dv) {
			if(new_num_itins != i) {
				arrivals[new_num_itins] = arrivals[i];
				swap_porkchop(&porkchop[index], &porkchop[5*new_num_itins+1]);
			}
			new_num_itins++;
		}
	}

	porkchop[0] = new_num_itins*5;

	return new_num_itins;
}

int filter_porkchop_arrivals_satdv(double *porkchop, struct ItinStep **arrivals, double min_dv, double max_dv, int fb0_pow1) {
	int new_num_itins = 0;
	int num_itins = (int) (porkchop[0]/5);

	if(min_dv > max_dv) {
		double temp = min_dv;
		min_dv = max_dv;
		max_dv = temp;
	}

	int index;
	double dv;

	for(int i = 0; i < num_itins; i++) {
		index = 1+i*5;
		dv = porkchop[index+3]+porkchop[index+4]*fb0_pow1;
		if(dv >= min_dv && dv <= max_dv) {
			if(new_num_itins != i) {
				arrivals[new_num_itins] = arrivals[i];
				swap_porkchop(&porkchop[index], &porkchop[5*new_num_itins+1]);
			}
			new_num_itins++;
		}
	}

	porkchop[0] = new_num_itins*5;

	return new_num_itins;
}

int filter_porkchop_arrivals_groups(double *porkchop, struct ItinStep **arrivals, struct Group *groups, int num_groups) {
	int new_num_itins = 0;
	int num_itins = (int) (porkchop[0]/5);

	int index;

	for(int i = 0; i < num_itins; i++) {
		index = 1+i*5;
		if(groups[get_itinerary_group_index(arrivals[i], groups, num_groups)].show_group) {
			if(new_num_itins != i) {
				arrivals[new_num_itins] = arrivals[i];
				swap_porkchop(&porkchop[index], &porkchop[5*new_num_itins+1]);
			}
			new_num_itins++;
		}
	}

	porkchop[0] = new_num_itins*5;

	return new_num_itins;
}



int get_itinerary_group_index(struct ItinStep *arrival_step, struct Group *groups, int num_groups) {
	struct ItinStep *ptr, *group_ptr;
	for(int group_idx = 0; group_idx < num_groups; group_idx++) {
		ptr = arrival_step;
		group_ptr = groups[group_idx].sample_arrival_node;
		while(group_ptr != NULL) {
			if(ptr == NULL) break;
			if(ptr->body != group_ptr->body) break;
			else {
				if(ptr->prev == NULL && group_ptr->prev == NULL) {
					return group_idx;
				}
			}
			group_ptr = group_ptr->prev;
			ptr = ptr->prev;
		}
	}
	return -1;
}

