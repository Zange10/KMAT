//
// Created by niklas on 01.03.24.
//

#include "data_tool.h"

void insert_new_data_point(struct Vector2D data[], double x, double y) {
	for (int i = 1; i <= data[0].x; i++) {
		if (x < data[i].x) {
			
			for (int j = (int) data[0].x; j >= i; j--) data[j + 1] = data[j];
			
			data[i].x = x;
			data[i].y = y;
			data[0].x++;
			return;
		}
	}
	
	data[(int)data[0].x+1].x = x;
	data[(int)data[0].x+1].y = y;
	data[0].x++;
}


struct Vector2D get_xn(struct Vector2D d0, struct Vector2D d1, double m_l, double m_r) {
	struct Vector2D xn;
	xn.x = (d1.y-d0.y + m_l*d0.x - m_r*d1.x) / (m_l-m_r);
	xn.y = m_l * (xn.x-d0.x) + d0.y;
	return xn;
}

double root_finder_monot_deriv_next_x(struct Vector2D *data, int branch) {
	// branch = 0 for left branch, 1 for right branch
	if(data[0].x == 2) return (data[1].x + data[2].x)/2;
	int num_data = (int) data[0].x;
	int index;
	
	/*for(int i = 1; i <= num_data; i++) {
		printf("[%f %f]", data[i].x, data[i].y);
	}
	printf("\n");*/
	
	// left branch
	if(branch == 0) {
		index = 1;
		for(int i = 2; i <= num_data; i++) {
			if(data[i].y < 0)			{ index = i; break; }
			if(data[i].y > data[i-1].y)	{ break; }
			else 						{ index = i; }
		}
		
		// right branch
	} else {
		index = num_data;
		for(int i = num_data-1; i >= 1; i--) {
			if(data[i].y < 0)			{ index =   i; break; }
			if(data[i].y > data[i+1].y)	{ index = 1+i; break; }
			else 						{ index =   i; }
		}
	}
	
	if(data[index].y < 0) {
		if(branch == 0) return (data[index].x + data[index-1].x)/2;
		else 			return (data[index].x + data[index+1].x)/2;
	}
	
	if(index <= 2)			return (data[1].x + data[2].x)/2;
	if(index >= num_data-1)	return (data[num_data-1].x + data[num_data].x)/2;
	
	
	double gradient_l 	= (data[index  ].y - data[index - 1].y) / (data[index  ].x - data[index - 1].x);
	double gradient_r 	= (data[index  ].y - data[index + 1].y) / (data[index  ].x - data[index + 1].x);
	double gradient_ll 	= (data[index-1].y - data[index - 2].y) / (data[index-1].x - data[index - 2].x);
	double gradient_rr 	= (data[index+1].y - data[index + 2].y) / (data[index+1].x - data[index + 2].x);
	
	struct Vector2D xn_l = get_xn(data[index-1], data[index  ], gradient_ll, gradient_r);
	struct Vector2D xn_r = get_xn(data[index  ], data[index+1], gradient_l, gradient_rr);
	
	return (xn_l.y < xn_r.y) ? xn_l.x : xn_r.x;
}

double root_finder_monot_func_next_x(struct Vector2D *data, int increasing) {
	// branch = 0 for decreasing monotonously, 1 for increasing monotonously
	if(data[0].x == 2) return (data[1].x + data[2].x)/2;
	int num_data = (int) data[0].x;
	int i_n = 2, i_p = 2;	// negative and positive index
	
	if(increasing == 0) {
		for(int i = 2; i <= num_data; i++) {
			if(data[i].y < 0)			{ i_n = i; break; }
		}
		i_p = i_n-1;
	} else {
		for(int i = 2; i <= num_data; i++) {
			if(data[i].y > 0)			{ i_p = i; break; }
		}
		i_n = i_p-1;
	}
	if(i_p == 1 || i_p == data[0].x ||
	   i_n == 1 || i_n == data[0].x) return (data[i_p].x+data[i_n].x) / 2;

	
	double m = (data[i_n].y - data[i_p].y) / (data[i_n].x - data[i_p].x);
	double n = data[i_p].y - data[i_p].x*m;
	
	return -n/m;
}

int can_be_negative_monot_deriv(struct Vector2D *data) {
	int num_data = (int) data[0].x;
	if(num_data < 3) return 1;
	
	int mind = 1;
	double min = data[1].y;
	for(int i = 2; i <= num_data; i++) {
		if(data[i].y < min) { mind = i; min = data[i].y; }
		if(min < 0) return 1;
	}
	
	if(mind == 1) mind++;
	if(mind == num_data) mind--;
	
	// gradient on left side and see whether it can get negetive on right side
	double gradient = (data[mind].y - data[mind - 1].y) / (data[mind].x - data[mind - 1].x);
	double dx = data[mind+1].x - data[mind].x;
	if(gradient*dx + data[mind].y < 0) return 1;
	
	// gradient on right side and see whether it can get negetive on left side
	gradient = (data[mind].y - data[mind + 1].y) / (data[mind].x - data[mind + 1].x);
	dx = data[mind-1].x - data[mind].x;
	if(gradient*dx + data[mind].y < 0) return 1;
	
	return 0;
}