// https://github.com/doonny/PipeCNN

#define USE_ROM

// The following macros are used for debug
//#define DEBUG_MEMRD
//#define DEBUG_CONV
//#define DEBUG_BN
//#define DEBUG_POOL
//#define DEBUG_MEMWR
//#define DEBUG_LRN
//#define DEBUG_LRN_OUT

#ifndef _HW_PARAM_H
#define _HW_PARAM_H

//choose net
#define RESNET
// #define ALEXNET
// #define VGG16


// Macro architecture parameters
// General
#define VEC_SIZE            16              // larger than 4, i.e., 4, 8, 16, ...
#define LANE_NUM            16            // larger than 1, for alexnet: 2, 3, 4, 8, 12, 15, 16, 22, 28, 32, 34, 48, 50, 51, 64, ...
#define CHN_DEPTH           0
//MemRD Kernel
#define CONV_GP_SIZE_X      7
#define CONV_GP_SIZE_Y      1              // In this version, CONV_GP_SIZE_Y must be 1
#ifdef ALEXNET
#define WIN_BUF_SIZE        9216/VEC_SIZE  // for AlexNet  batch=1
	#define WEIGHT_BUF_SIZE     9216/VEC_SIZE  // for AlexNet  batch=1
#endif
#ifdef VGG16
#define WIN_BUF_SIZE        25088/VEC_SIZE // for VGG-16  batch=1
	#define WEIGHT_BUF_SIZE     25088/VEC_SIZE // for VGG-16  batch=1
#endif
//#define WIN_BUF_SIZE        CONV_GP_SIZE_X*9216/VEC_SIZE  // for AlexNet  batch>=4
//#define WEIGHT_BUF_SIZE     9216/VEC_SIZE                 // for AlexNet  batch>=4
#ifdef RESNET
#define WIN_BUF_SIZE        16384/VEC_SIZE  // for ResNet-50  batch=1
#define WEIGHT_BUF_SIZE     4608/VEC_SIZE  // for ResNet-50  batch=1
#endif
// Conv Kernel
#define PIPE_DEPTH          6
// Pooling Kernel
#define POOL_GP_SIZE_X      4
#define POOL_MAX_SIZE       3
// Lrn Kernel
#define LRN_WIN_SIZE        5
#define LRN_MAX_LOCAL_SIZE  (256/VEC_SIZE) // For alexnet the max dim3 size is 256
#define MAN_BITS            23             // Floating point format setting
#define EXP_MASK            0xFF           // Floating point format setting
#define MAN_MASK            0x7FFFFF       // Floating point format setting
#define EXP_STEP_MIN        13             // PWLF table setting
#define EXP_STEP_LOG        0              // PWLF table setting
#define MAN_INDEX_BITS      2              // PWLF table setting
#define MAN_INDEX_MASK      0x03           // PWLF table setting

//ResNet-50,Eltwise Kernel
#ifdef RESNET
#define AVGPOOL_SIZE       49			   //7*7
#define ELT_PIPE_DEPTH     8
#endif
// Parameters for fixed-point design
#define CZERO       0x00     // constant zero
#define MASK8B      0xFF     // used for final rounding
#define MASK9B      0x1FE    // used for final rounding
#define MASKSIGN    0x80     // used for final rounding
// #define MASK_ACCUM  0x1FFFFFFF // not used for reducing mac pipeline logic cost (when PIPE_DEPTH=6, MASK_ACCUM has 16+13=29 bits)
//#define MASK_MULT   0x1FFFFF   // not used for reducing mac pipeline logic cost (four parallel mac, max VEC_SIZE is 32, MASK_MULT has 16+5=21 bits)
#define MASK_ACCUM  0xFFFFFFFF // use this value
#define MASK_MULT   0xFFFFFFFF // use this value


#ifdef USE_ROM
// Coefficients lookup table for lrn computation
constant float coef0[46] = {9.98312401e-01,8.92383765e-01,8.69534866e-01,8.48001507e-01,8.27672857e-01,8.08269896e-01,7.72814246e-01,7.40785193e-01,7.11686616e-01,6.84743320e-01,6.38046300e-01,5.98139529e-01,5.63585746e-01,5.32842946e-01,4.82570938e-01,4.42066574e-01,4.08721176e-01,3.80120836e-01,3.35733988e-01,3.01782553e-01,2.74896454e-01,2.52503409e-01,2.19044754e-01,1.94367577e-01,1.75328514e-01,1.59766323e-01,1.37073713e-01,1.20695464e-01,1.08253750e-01,9.81965345e-02,8.37272488e-02,7.34111523e-02,6.56398695e-02,5.93964327e-02,5.04776032e-02,4.41593533e-02,3.94211944e-02,3.56262849e-02,3.02252062e-02,2.64117530e-02,2.35583854e-02,2.12767794e-02,1.80355644e-02,1.57509127e-02,1.40434261e-02};
constant float coef1[46] = {-1.07542919e-01,-2.28535953e-02,-2.15331066e-02,-2.03286855e-02,-1.92268508e-02,-3.55023570e-02,-3.20657642e-02,-2.91245494e-02,-2.65861837e-02,-4.68257134e-02,-3.99817597e-02,-3.45887189e-02,-3.02571264e-02,-5.05149626e-02,-4.06040782e-02,-3.34413514e-02,-2.80826706e-02,-4.46757687e-02,-3.40991637e-02,-2.69894342e-02,-2.19616650e-02,-3.37238519e-02,-2.48195600e-02,-1.91265576e-02,-1.52482883e-02,-2.29016249e-02,-1.64847560e-02,-1.25042597e-02,-9.85141038e-03,-1.46114169e-02,-1.03881575e-02,-7.81187564e-03,-6.11526810e-03,-9.00946183e-03,-6.36361270e-03,-4.76376961e-03,-3.71675305e-03,-5.45684726e-03,-3.84135330e-03,-2.86894660e-03,-2.23458481e-03,-3.27498492e-03,-2.30149338e-03,-1.71686994e-03,-1.33609904e-03};
constant float h_inv[46] = {1.22085215e-04,4.88281250e-04,4.88281250e-04,4.88281250e-04,4.88281250e-04,2.44140625e-04,2.44140625e-04,2.44140625e-04,2.44140625e-04,1.22070313e-04,1.22070313e-04,1.22070313e-04,1.22070313e-04,6.10351563e-05,6.10351563e-05,6.10351563e-05,6.10351563e-05,3.05175781e-05,3.05175781e-05,3.05175781e-05,3.05175781e-05,1.52587891e-05,1.52587891e-05,1.52587891e-05,1.52587891e-05,7.62939453e-06,7.62939453e-06,7.62939453e-06,7.62939453e-06,3.81469727e-06,3.81469727e-06,3.81469727e-06,3.81469727e-06,1.90734863e-06,1.90734863e-06,1.90734863e-06,1.90734863e-06,9.53674316e-07,9.53674316e-07,9.53674316e-07,9.53674316e-07,4.76837158e-07,4.76837158e-07,4.76837158e-07,4.76837158e-07};
constant float x_sample[46] = {1.00000000e+00,8.19200000e+03,1.02400000e+04,1.22880000e+04,1.43360000e+04,1.63840000e+04,2.04800000e+04,2.45760000e+04,2.86720000e+04,3.27680000e+04,4.09600000e+04,4.91520000e+04,5.73440000e+04,6.55360000e+04,8.19200000e+04,9.83040000e+04,1.14688000e+05,1.31072000e+05,1.63840000e+05,1.96608000e+05,2.29376000e+05,2.62144000e+05,3.27680000e+05,3.93216000e+05,4.58752000e+05,5.24288000e+05,6.55360000e+05,7.86432000e+05,9.17504000e+05,1.04857600e+06,1.31072000e+06,1.57286400e+06,1.83500800e+06,2.09715200e+06,2.62144000e+06,3.14572800e+06,3.67001600e+06,4.19430400e+06,5.24288000e+06,6.29145600e+06,7.34003200e+06,8.38860800e+06,1.04857600e+07,1.25829120e+07,1.46800640e+07,1.67772160e+07};
#endif

#endif

int   mult_add_fix8bx4 (char a0_in, char b0_in, char a1_in, char b1_in, char a2_in, char b2_in, char a3_in, char b3_in);

#pragma OPENCL EXTENSION cl_intel_channels : enable

// Define the precision of the data-path
typedef char DPTYPE;
typedef int  MACTYPE;

// Vectorized data type
typedef struct {
   DPTYPE data[VEC_SIZE];
} lane_data;

// Combined vec-data type from multiple lane
typedef struct {
   lane_data lane[LANE_NUM];
} channel_vec;

// Combined scalar data type from multiple lane
typedef struct {
   DPTYPE lane[LANE_NUM];
} channel_scal;
#ifdef RESNET
typedef struct {
   float lane[LANE_NUM];
} channel_scal_float;
#endif


__kernel
__attribute__((max_work_group_size(1,1,LRN_MAX_LOCAL_SIZE))) // (x,y,z)
void lrn(
			// Params Ports
			uchar data_dim1,
			uchar data_dim2,
			char  frac_dout,
			// Data Ports
			__global lane_data *restrict bottom,
			__global lane_data *restrict top
		)
{
	uchar  global_x = get_global_id(0); // max value 256
	uchar  global_y = get_global_id(1); // max value 256
	ushort global_z = get_global_id(2); // max value 4096

	#ifdef DEBUG_LRN
	int local_x = get_local_id(0);
	int local_y = get_local_id(1);
	int local_z = get_local_id(2);
	int block_x = get_group_id(0);
	int block_y = get_group_id(1);
	int block_z = get_group_id(2);
	#endif

	__local DPTYPE z_buffer[VEC_SIZE*LRN_MAX_LOCAL_SIZE+LRN_WIN_SIZE]; // allocate two more points for padding
	__local DPTYPE lrn_buffer[VEC_SIZE*LRN_MAX_LOCAL_SIZE];
	channel_scal data_in;
	channel_scal data_pad_left;
	channel_scal data_pad_right;
	channel_scal data_out;
	lane_data    data_in_partial;
	lane_data    data_left_partial;
	lane_data    data_right_partial;
	lane_data    data_out_partial;
	int          *convert_ptr;
	int          expo;
	uint         manti;
	uint         addr_1, addr_2, addr;
	float        lrn_reg1, lrn_reg2, lrn_tmp, lrn_out;
	short        lrn_cnvt, lrn_cnvt2;

	// Load the all data in one line along dim3 into local line buffer
	#pragma unroll
	for(unsigned char ll=0; ll<VEC_SIZE; ll++){
		z_buffer[global_z*VEC_SIZE+ll+LRN_WIN_SIZE/2] = bottom[global_z*data_dim2*data_dim1 + global_y*data_dim1+ global_x].data[ll];
	}

	//Padding left
	if(global_z==0){
		#pragma unroll
		for(unsigned char ll=0; ll<LRN_WIN_SIZE/2; ll++){
			z_buffer[ll] = CZERO;
		}
	}

	// Padding right
	if(global_z==(get_global_size(2)-1)){
		#pragma unroll
		for(unsigned char ll=0; ll<LRN_WIN_SIZE/2; ll++){
			z_buffer[VEC_SIZE*get_local_size(2)+ll+LRN_WIN_SIZE/2] = CZERO;
		}
	}

	#ifdef DEBUG_LRN
	if(global_z==0&&global_x==0&&global_y==0)
	printf("Kernel LRN: work-item x=%d, y=%d, z=%d(z_local=%d)\n", global_x, global_y, global_z, local_z);
	#endif
	barrier(CLK_LOCAL_MEM_FENCE); // fill all values of the line bufer before reading it

	// Piecewise interpolation pipeline for lrn operation (i.e., y=pwlf(x'))
	for(unsigned char ll=0; ll<VEC_SIZE; ll++){
		// First Step: Coefficients table looking-up
		// Calculate x'=sum(x(k)^2) for the pwlf function, x(k)s are from adjacent featuremaps
		lrn_reg2 = CZERO;
		#pragma unroll
		for(char k=-LRN_WIN_SIZE/2; k<=LRN_WIN_SIZE/2; k++){
			// Convert DPTYPE fixed-point to float
			// Input data has "frac_dout" fractional bits
			// Note: current version only support frac_dout<0
			lrn_cnvt = z_buffer[global_z*VEC_SIZE+ll+k+LRN_WIN_SIZE/2]<<(-frac_dout);
			lrn_reg1 = convert_float(lrn_cnvt);
			lrn_reg2 += lrn_reg1 * lrn_reg1;
			#ifdef DEBUG_LRN
			if(global_z==0&&global_x==0&&global_y==0)
			printf("x=%f(k=%d), ", lrn_reg1, k);
			#endif
		}
		// Get the exponent and mantissa of x' (assuming x'>=0)
		convert_ptr = (int*) (&lrn_reg2);
		// substract the bias 127 to get exponent
		expo = (EXP_MASK & (*convert_ptr >> MAN_BITS)) - 127;
		// manti = ((*convert_ptr) & MAN_MASK); //does not include the implicit 1 // todo uncomment

		// Get level-1 table item (segment) index from exponent
		addr_1 = ((expo-EXP_STEP_MIN)>>EXP_STEP_LOG)<<MAN_INDEX_BITS;
		// Get level-2 table item (segment) index from mantissa
		addr_2 = (manti>>(MAN_BITS-MAN_INDEX_BITS) & MAN_INDEX_MASK)+1;
		// Combine level-1 and level-2 table index to get true table address
		if(expo<EXP_STEP_MIN)
			addr = 0; // use the first segment
		else
			addr = addr_1+addr_2;

		// Sencond Step: Perform piecewise linear interpolation
		lrn_tmp = ((lrn_reg2-x_sample[addr])*h_inv[addr])*coef1[addr] + coef0[addr];

		// Third Step: Perform lrn operation
		lrn_cnvt2 = z_buffer[global_z*VEC_SIZE+ll+LRN_WIN_SIZE/2]<<(-frac_dout);
		lrn_out = lrn_tmp*convert_float(lrn_cnvt2);

		// Convert float to DPTYPE fixed-point
		// Note: current version only support frac_din=0 for next layer
		lrn_buffer[global_z*VEC_SIZE+ll] = convert_char_rte(lrn_out);

		#ifdef DEBUG_LRN
		if(global_z==0&&global_x==0&&global_y==0)
		printf("\nKernel LRN (ll=%d): pwlf_x=%f, expo=%d, addr=%d, pwlf_y=%f, lrn=%f\n", ll, lrn_reg2, expo, addr, lrn_tmp, lrn_out);
		#endif
		barrier(CLK_LOCAL_MEM_FENCE);
	}

	// Store the results back to global mem
	#pragma unroll
	for(unsigned char vv=0; vv<VEC_SIZE; vv++){
		data_out_partial.data[vv]=lrn_buffer[global_z*VEC_SIZE+vv];
	}
	//top[global_z*data_dim2*data_dim1 + global_y*data_dim1 + global_x] = data_out_partial;

	#ifdef DEBUG_LRN_OUT
	if(global_z==0&&global_x==0&&global_y==0)
	printf("\nKernel LRN OUT: x=%d, y=%d, z=%d, result=%f\n", global_x, global_y, global_z, (float)data_out_partial.data[0]);
	#endif

}

