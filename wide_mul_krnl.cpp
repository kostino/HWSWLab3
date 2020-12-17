
//Including to use ap_uint<> datatype
#include <ap_int.h>
#include <stdio.h>
#include <string.h>

#define BUFFER_SIZE 64
#define DATAWIDTH 512
#define VECTOR_SIZE (DATAWIDTH / 32) // vector size is 16 (512/32 = 16)
typedef ap_uint<DATAWIDTH> uint512_dt;
#define lm 5
#define ln 5
#define lp 5
#define M (1<<lm)
#define N (1<<ln)
#define P (1<<lp)

//TRIPCOUNT identifier
const unsigned int c_chunk_sz = BUFFER_SIZE;
const unsigned int c_size     = VECTOR_SIZE;

/*
    Vector Addition Kernel Implementation using uint512_dt datatype
    Arguments:
        in1   (input)     --> Input Vector1
        in2   (input)     --> Input Vector2
        out   (output)    --> Output Vector
        size  (input)     --> Size of Vector in Integer
   */
extern "C"
{
    void wide_vadd(
        const uint512_dt *in1, // Read-Only Vector 1
        const uint512_dt *in2, // Read-Only Vector 2
        uint512_dt *out,       // Output Result
        int size               // Size in integer
    )
    {
#pragma HLS INTERFACE m_axi port = in1 max_write_burst_length = 32 max_read_burst_length = 32 offset = slave bundle = gmem
#pragma HLS INTERFACE m_axi port = in2 max_read_burst_length = 32 offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = out max_write_burst_length = 32 max_read_burst_length = 32 offset = slave bundle = gmem2
#pragma HLS INTERFACE s_axilite port = in1 bundle = control
#pragma HLS INTERFACE s_axilite port = in2 bundle = control
#pragma HLS INTERFACE s_axilite port = out bundle = control
#pragma HLS INTERFACE s_axilite port = size bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

        uint512_dt v1_local[N*M/VECTOR_SIZE]; // Local memory to store vector1 ROW MAJOR
        uint512_dt v2_local[M*P/VECTOR_SIZE]; // COL MAJOR
        ap_uint<32> result_local[N*P]; // Local Memory to store result


        // Input vector size for integer vectors. However kernel is directly
        // accessing 512bit data (total 16 elements). So total number of read
        // from global memory is calculated here:
        int size_in16 = (size - 1) / VECTOR_SIZE + 1;

        //Per iteration of this loop perform BUFFER_SIZE vector addition
        //for (int i = 0; i < size_in16; i += BUFFER_SIZE) {
//#pragma HLS PIPELINE
//#pragma HLS DATAFLOW
//#pragma HLS stream variable = v1_local depth = 64
//#pragma HLS stream variable = v2_local depth = 64

           // int chunk_size = BUFFER_SIZE;

            //boundary checks
           // if ((i + BUFFER_SIZE) > size_in16)
           //     chunk_size = size_in16 - i;

        //burst read first vector from global memory to local memory
        v1_rd:
            for (int j = 0; j < N*M/VECTOR_SIZE; j++) {
#pragma HLS pipeline
//#pragma HLS LOOP_TRIPCOUNT min = 1 max = 64
                v1_local[j] = in1[j];
            }
            for (int j = 0; j < M*P/VECTOR_SIZE; j++) {
            #pragma HLS pipeline
            //#pragma HLS LOOP_TRIPCOUNT min = 1 max = 64
                v2_local[j] = in2[j];
            }

        //burst read second vector and perform vector addition
        v2_rd_add:
		for (int i = 0; i < N; i++){
		for (int j = 0; j < P; j++) {
			ap_uint<32> temp=0;
		for (int k = 0; k < M/VECTOR_SIZE; k++){
		#pragma HLS pipeline
		//#pragma HLS LOOP_TRIPCOUNT min = 1 max = 64
		                uint512_dt tmpV1 = v1_local[i*(M/VECTOR_SIZE)+k];
		                uint512_dt tmpV2 = v2_local[j*(M/VECTOR_SIZE)+k];


						for (int vector = 0; vector < VECTOR_SIZE; vector++) {
						   #pragma HLS UNROLL
							ap_uint<32> tmp1 = tmpV1.range(32 * (vector + 1) - 1, vector * 32);
							ap_uint<32> tmp2 = tmpV2.range(32 * (vector + 1) - 1, vector * 32);
							temp += tmp1 * tmp2;
						}
					}
				result_local[i*P+j]=temp;
		      }
		}
        //}
		for(int i=0; i < N*P/VECTOR_SIZE;i++){
			uint512_dt tmpout;
			for(int vector = 0; vector < VECTOR_SIZE; vector++){

				tmpout.range(32 * (vector + 1) - 1, vector * 32)=result_local[i*(VECTOR_SIZE)+vector];
			}
			out[i]=tmpout;
		}
    }
}
