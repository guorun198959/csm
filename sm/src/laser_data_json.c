#include <errno.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include "logging.h"
#include "math_utils.h"
#include "laser_data_json.h"

JO matrix_to_json(gsl_matrix*m) {
	JO jo = jo_new_array();
	if(m->size1>1) {
		size_t i,j;
		for(i=0;i<m->size1;i++) {
			JO row  = json_object_new_array();
			for(j=0;j<m->size2;j++) {
				double v = gsl_matrix_get(m,i,j);
				json_object_array_add(row, jo_double_or_null(v));
			}
			json_object_array_add(jo, row);
		}
	} else {
		size_t i = 1, j;
		JO row  = jo;
		for(j=0;j<m->size2;j++) {
			double v = gsl_matrix_get(m,i,j);
			json_object_array_add(row, jo_double_or_null(v));
		}
	}
	return jo;
}

int json_to_corr(JO array, struct correspondence*corr, int n) {
	/** XXX : check it's an array and its size is good */
	int i;
	for(i=0;i<n;i++) {
		JO element = json_object_array_get_idx(array, i);
		if(element == NULL) {
			corr[i].valid = 0;
			corr[i].j1 = -1;
			corr[i].j2 = -1;
		} else {
			corr[i].valid = 1;
			jo_read_int(element, "j1", &(corr[i].j1));
			jo_read_int(element, "j2", &(corr[i].j2));
		}
	}
	return 1;
}

JO corr_to_json(struct correspondence*corr, int n) {
	JO jo = jo_new_array();
	int i;
	for(i=0;i<n;i++) {
		if(corr[i].valid) {
			JO c = jo_new();
			jo_add_int(c, "j1", corr[i].j1);
			jo_add_int(c, "j2", corr[i].j2);
			jo_array_add(jo, c);
		} else {
			jo_array_add(jo, jo_new_null());
		}
	}
	return jo;
}


JO vector_to_json(gsl_vector*vec) {
	JO jo = json_object_new_array();
	size_t j;
	for(j=0;j<vec->size;j++) {
		double v = gsl_vector_get(vec,j);
		jo_array_add(jo, jo_double_or_null(v));
	}
	return jo;
}

JO result_to_json(struct sm_params*p, struct sm_result *r) {
	JO jo = json_object_new_object();
	jo_add_double_array(jo, "x",  r->x, 3);
	
	if(p->do_compute_covariance) {
		jo_add(jo, "cov_x",   matrix_to_json(r->cov_x_m ) );
		jo_add(jo, "dx_dy1",  matrix_to_json(r->dx_dy1_m) );
		jo_add(jo, "dx_dy2",  matrix_to_json(r->dx_dy2_m) );
	}
	jo_add_int(jo, "iterations", r->iterations);
	jo_add_int(jo, "nvalid", r->nvalid);
	jo_add_double(jo, "error", r->error);
	return jo;
}

int is_all_nan(const double *v, int n ) {
	int i; for(i=0;i<n;i++) if(v[i]==v[i]) return 0;
	return 1;
}

/** Adds unless it's all NAN */
void jo_add_double_array_if_not_nan(JO jo, const char*name, const double *v, int n) {
	if(is_all_nan(v,n)) return;
	jo_add_double_array(jo, name, v, n);
}

/** true if all values are equal to v */
int all_is(int *a, int n, int v) {
	int i; for(i=0;i<n;i++) if(a[i]!=v) return 0;
	return 1;
}

JO ld_to_json(LDP ld) {
	JO jo = json_object_new_object();
	int n = ld->nrays;

	jo_add_int(jo, "nrays",    ld->nrays);
	jo_add_double(jo, "min_theta", ld->min_theta);
	jo_add_double(jo, "max_theta", ld->max_theta);

	jo_add_double_array(jo, "odometry",   ld->odometry,  3);
	jo_add_double_array(jo, "estimate",   ld->estimate,  3);
	jo_add_double_array(jo, "true_pose",  ld->true_pose, 3);

	jo_add_double_array(jo, "theta",     ld->theta, n);
	jo_add_double_array(jo, "readings",   ld->readings, n);

	jo_add_int_array(jo, "valid",   ld->valid, n);

	if(!all_is(ld->cluster, n, -1))
		jo_add_int_array(jo, "cluster",  ld->cluster, n);

	jo_add_double_array_if_not_nan(jo, "alpha",        ld->alpha, n);
	jo_add_double_array_if_not_nan(jo, "cov_alpha",    ld->cov_alpha, n);

	if(!all_is(ld->alpha_valid, n, 0))
		jo_add_int_array(jo, "alpha_valid", ld->alpha_valid, n);
	
	jo_add_double_array_if_not_nan(jo, "true_alpha",      ld->true_alpha, n);
	

	return jo;
/*	int *up_bigger, *up_smaller, *down_bigger, *down_smaller;

	gsl_vector**p;
	
	struct correspondence* corr;
*/
}


LDP json_to_ld(JO jo) {
	int n;
	if(!jo_read_int(jo, "nrays", &n)) {
		sm_error("Could not read nrays.\n");
		return 0;
	}
	
	LDP ld = ld_alloc_new(n);
	jo_read_double(jo, "min_theta", &ld->min_theta);
	jo_read_double(jo, "max_theta", &ld->max_theta);	
	jo_read_double_array(jo, "theta", ld->theta, n, NAN);	
	jo_read_double_array(jo, "readings", ld->readings, n, NAN);	

	jo_read_int_array(jo, "valid",     ld->valid, n, 0);
	jo_read_int_array(jo, "cluster",   ld->cluster, n, -1);

	jo_read_double_array(jo, "alpha",     ld->alpha, n, NAN);
	jo_read_double_array(jo, "cov_alpha", ld->cov_alpha, n, NAN);
	jo_read_int_array(jo, "alpha_valid",   ld->alpha_valid, n, 0);

	jo_read_double_array(jo, "true_alpha",     ld->true_alpha, n, NAN);
	
	jo_read_double_array(jo, "odometry", ld->odometry, 3, NAN);
	jo_read_double_array(jo, "estimate", ld->estimate, 3, NAN);	
	jo_read_double_array(jo, "true_pose", ld->true_pose, 3, NAN);	
	
	return ld;
}

LDP ld_from_json_stream(FILE*file) {
	JO jo; /* the monkey */
	LDP ld;
	
	jo = json_read_stream(file);
	if(!jo) return 0;

	ld = json_to_ld(jo);
	if(!ld) {
		sm_error("Could not read laser_data:\n\n%s\n", json_object_to_json_string(jo));
		return 0;
	}
	jo_free(jo);
	return ld;
}

/** 
	Tries to read a laser scan from file. If error or EOF, it returns 0.
	Whitespace is skipped. If first valid char is '{', it tries to read 
	it as JSON. If next char is 'F' (first character of "FLASER"),
	it tries to read in Carmen format. Else, 0 is returned. 
*/
LDP ld_read_smart(FILE*f) {
	int c;
	while(1) {
		c = fgetc(f);
		if(feof(f)) { 
			/* sm_debug("eof\n"); */
			return 0;
		}
		if(!isspace(c)) break;
	}
	ungetc(c, f);

	switch(c) {
		case '{': {
/*			sm_debug("Reading JSON\n"); */
			return ld_from_json_stream(f);
		}
		case 'F': {
/*			sm_debug("Reading Carmen\n");  */
			LDP ld = (LDP) malloc(sizeof(struct laser_data));
			if(ld_read_next_laser_carmen(f, ld)) {
				sm_debug("bad carmen\n");
				free(ld);
				return 0;
			}
			return ld;
		}
		default:
			sm_error("Could not read ld. First char is '%c'. \n", c);
			
		return 0;
	}
}



