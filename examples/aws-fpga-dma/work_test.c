// This is the main program for driving the work computation

#include <math.h>
#include <time.h>

#include "dma_msg_sw.c"
#include "work_sw.c"

// Helper to init an input data
float max_val = 100.0;
work_inputs_t work_inputs_init(int i)
{
	// Randomizeish float values to sum
	work_inputs_t inputs;
	for(int v=0;v<N_SUM;v++)
	{
		inputs.values[v] = (float)rand()/(float)(RAND_MAX/max_val); // rand[0..100]
	}
	return inputs;
}

// Helper to compare two output datas
void compare(int i, work_outputs_t cpu, work_outputs_t fpga)
{
	float ep = max_val / 1000.0; // 1/1000th of range;
	if(fabs(fpga.sum - cpu.sum) > ep)
	{
		printf("Output %d does not match! FPGA: %f, CPU: %f\n", i, fpga.sum, cpu.sum);
		printf("	FPGA: 0x%x\n",*(unsigned int*)&fpga.sum);
		printf("	CPU:  0x%x\n",*(unsigned int*)&cpu.sum);
	}
}

// Do work() using the FPGA hardware
work_outputs_t work_fpga(work_inputs_t inputs)
{
	// Convert input into bytes
	dma_msg_t write_msg;
	write_msg = inputs_to_bytes(inputs);
	// Write those DMA bytes to the FPGA
	dma_write(write_msg);
	// Read a DMA bytes back from FPGA
	dma_msg_t read_msg;
	read_msg = dma_read();
	// Convert bytes to outputs and return
	work_outputs_t work_outputs;
	work_outputs = bytes_to_outputs(read_msg);
	return work_outputs;
}

// Init + do some work + close
int main(int argc, char **argv) 
{
	// Init direct memory access to/from FPGA
	init_dma();
	
	// Do something using the function 'work_fpga()'
	// which uses the FPGA to compute the 'work()' function
	
	// Prepare N work inputs, and 2 output pairs (cpu vs fpga)
	int n = 100000;
	work_inputs_t* inputs = (work_inputs_t*)malloc(n*sizeof(work_inputs_t));
	work_outputs_t* cpu_outputs = (work_outputs_t*)malloc(n*sizeof(work_outputs_t));
	work_outputs_t* fpga_outputs = (work_outputs_t*)malloc(n*sizeof(work_outputs_t));
	for(int i = 0; i < n; i++)
	{
		inputs[i] = work_inputs_init(i);
	}
	
	// Time things
	clock_t t;
	double time_taken;
	
	// Start time
	t = clock(); 
	// Do the work on the cpu
	for(int i = 0; i < n; i++)
	{
		cpu_outputs[i] = work(inputs[i]);
	}
	// End time
	t = clock() - t; 
  time_taken = ((double)t)/CLOCKS_PER_SEC; // in seconds
  printf("CPU took %f seconds to execute \n", time_taken); 
	double cpu_time = time_taken;

	// Start time
	t = clock(); 
	// Do the work on the FPGA
	for(int i = 0; i < n; i++)
	{
		fpga_outputs[i] = work_fpga(inputs[i]);
	}
	// End time
	t = clock() - t; 
  time_taken = ((double)t)/CLOCKS_PER_SEC; // in seconds
  printf("FPGA took %f seconds to execute \n", time_taken);
  double fpga_time = time_taken;
  
  // Speedy?
  printf("Speedup: %f\n",cpu_time/fpga_time);  
	
	// Compare the outputs
	for(int i = 0; i < n; i++)
	{
		compare(i,cpu_outputs[i],fpga_outputs[i]);
	}

	// Close direct memory access to/from FPGA
	close_dma();    
}
