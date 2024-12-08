#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include "cache.h"
#include <iomanip>
#include <cassert>
//#include "globals.h"
using namespace std;
unsigned long Total_execution_time = 0;
int main(int argc, char *argv[]) {
    int cache_size = 0;
    int cache_assoc = 0;
    int blk_size = 0;
    int num_processors = 0;
    int protocol = -1;
    char *fname = NULL;
    //unsigned long Total_execution_time = 0; 
    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--cache-size") == 0) {
            if (strcmp(argv[i + 1], "infinite") == 0) 
                cache_size = -1;  // Use -1 for "infinite"
            else 
                cache_size = atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "--assoc") == 0) {
            cache_assoc = atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "--block-size") == 0) {
            blk_size = atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "--num-proc") == 0) {
            num_processors = atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "--protocol") == 0) {
            protocol = atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "--trace") == 0) {
            fname = argv[i + 1];
            i++;
        }
    }

    // Check if all required arguments were provided
    if (cache_size == 0 || cache_assoc == 0 || blk_size == 0 || num_processors == 0 || protocol == -1 || fname == NULL) {
        cout << "Invalid arguments. Please check the input format." << endl;
        cout << "Usage: ./smp_cache --cache-size <size|infinite> --assoc <assoc> --block-size <size> "
             << "--num-proc <num_processors> --protocol <0|1> --trace <trace_file>" << endl;
        return 1;
    }

    // Print configuration details
  
    cout << "===== 506 SMP Simulator configuration =====" << endl;
    cout << "L1_SIZE:           " << (cache_size == -1 ? "infinite" : to_string(cache_size)) << endl;
    cout << "L1_BLOCKSIZE:      " << blk_size << endl;
    cout << "NUMBER OF PROCESSORS: " << num_processors << endl;
    cout << "COHERENCE PROTOCOL: " << (protocol == 0 ? "MESI" : "MOESI") << endl;
    cout << "TRACE FILE:        " << fname << endl;

    // Create an array of caches
    Cache **cache = (Cache **)malloc(num_processors * sizeof(Cache *));
    for (int i = 0; i < num_processors; i++) {
        cache[i] = new Cache(cache_size, cache_assoc, blk_size);
    }

    // Open the trace file
    ifstream trace_file(fname);
    if (!trace_file.is_open()) {
        cout << "Trace file problem" << endl;
        return 1;
    }

    string line;
    // int insCount = 1; // ADDED FOR DEBUG
    while (getline(trace_file, line)) {
        istringstream iss(line);
        int input_processor;
        char rw;
        unsigned int addr;
        int bus_signal = NO_OP;
        int action_time = 0;

        if (!(iss >> input_processor >> rw >> hex >> addr)) {
            cout << "Error reading trace file format." << endl;
            break;
        }

        // Determine if there is a copy in any other processor's cache
        // Creating share array to keep track of which processors have this block to avoid unnecessary snoops
        int* share_array = new int[num_processors]();
        int copy = 0;
        for (int i = 0; i < num_processors; i++) {
            if (cache[i]->findLine(addr) && (i != input_processor)) {
                copy = 1;
                share_array[i] = 1;
            }
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // ADDED FOR DEBUG
        // cout << "Address: " << hex << addr << endl;        
        // cout << "Input processor: " << input_processor << endl;
        // cout << "Tag: " << cache[input_processor]->calcTag(addr) << endl;
        // cout << "Instr: " << rw << endl;

        // cout << dec << insCount << ". " << "Processor: " << input_processor << setw(10) << "\t" << "Operation: " << rw << setw(10) << "\t" << "Address: " << hex << cache[input_processor]->calcTag(addr) << endl;
        // insCount ++;
        // cout << "Share array: ";
        // for (int i = 0; i < num_processors; ++i) {
        //     cout << share_array[i];
        // }
        // cout << endl;
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        // Process the request based on the coherence protocol
        if (protocol == 0) {  // MESI protocol

            // Dealing with Processor side of requests first
            cache[input_processor]->MESI_Processor_Access(addr, rw, copy, cache, input_processor, num_processors, bus_signal, action_time);
            Total_execution_time += action_time;
            // cout << dec << bus_signal << endl;
            
            // Dealing with snooped requests next
            // Snoop only from the processors that have share array slot as 1
            for (int i = 0; i < num_processors; ++i) {
                if (share_array[i]) {
                    assert(i != input_processor); // If this happens, then my share array logic is incorrect
                    // cout << "SNOOPING" << endl;
                    
                    // The switch case addresses the various snooped possibilities
                    switch (bus_signal)
                    {
                    case NO_OP:
                        break;

                    case READ:
                        cache[i]->MESI_Bus_Snoop(addr, 1, 0, 0);
                        break;
                    
                    case READX:
                        cache[i]->MESI_Bus_Snoop(addr, 0, 1, 0);
                        break;

                    case UPGR:
                        cache[i]->MESI_Bus_Snoop(addr, 0, 0 ,1);
                        break;
                    
                    default:
                        assert(0 && "Default case not possible");
                        break;
                    }                
                }
            }

            //   ulong state = cache[input_processor]->findLine(addr)->getFlags();
            //   std::cout << std::setw(10) << " " << rw  << ":" << std::hex << addr << "   :P" << (input_processor + 1) << "  ";
            // //   ulong state = cache[input_processor]->findLine(addr)->getFlags();
            //   cache[input_processor]->printCacheState(state);
            //   for (int i = 0; i < num_processors; i++) {
            //   if (i != input_processor) {
            //   cacheLine* line = cache[i]->findLine(addr);
            //   if (line) {  // If the line exists in the cache
            //   std::cout << " :P" << (i + 1) << " ";
            //   cache[i]->printCacheState(line->getFlags());}
            //   else
            //   {
            //     std::cout << " :P" << (i + 1) << " ";
            //     std::cout << "-";
            //   }
            
            //  }
            // }
            // std::cout<<std::endl;
            
           }
           
         
         else if (protocol == 1) {  // MOESI protocol
            // Dealing with processor side of request first
            cache[input_processor]->MOESI_Processor_Access(addr, rw, copy, cache, input_processor, num_processors, bus_signal, action_time);
            Total_execution_time += action_time;

            // Dealing with snooped requests next
            // Snoop only from the processors that have share array slot as 1
            for (int i = 0; i < num_processors; ++i) {
                if (share_array[i]) {
                    assert(i != input_processor); // If this happens, then my share array logic is incorrect
                    // cout << "SNOOPING" << endl;
                    
                    // The switch case addresses the various snooped possibilities
                    switch (bus_signal)
                    {
                    case NO_OP:
                        break;

                    case READ:
                        cache[i]->MOESI_Bus_Snoop(addr, 1, 0, 0);
                        break;
                    
                    case READX:
                        cache[i]->MOESI_Bus_Snoop(addr, 0, 1, 0);
                        break;

                    case UPGR:
                        cache[i]->MOESI_Bus_Snoop(addr, 0, 0 ,1);
                        break;
                    
                    default:
                        assert(0 && "Default case not possible");
                        break;
                    }                
                }
            }

            //   ulong state = cache[input_processor]->findLine(addr)->getFlags();
            //   std::cout << std::setw(10) << " " << rw  << ":" << std::hex << addr << "   :P" << (input_processor + 1) << "  ";
            // //   ulong state = cache[input_processor]->findLine(addr)->getFlags();
            //   cache[input_processor]->printCacheState(state);
            //   for (int i = 0; i < num_processors; i++) {
            //   if (i != input_processor) {
            //   cacheLine* line = cache[i]->findLine(addr);
            //   if (line) {  // If the line exists in the cache
            //   std::cout << " :P" << (i + 1) << " ";
            //   cache[i]->printCacheState(line->getFlags());}
            //   else
            //   {
            //     std::cout << " :P" << (i + 1) << " ";
            //     std::cout << "-";
            //   }
            
            //  }
            // }
            // std::cout<<std::endl;

        }
        delete[] share_array;
    }

    trace_file.close();
    //printf("Total Execution time: 				                %10lu\n", Total_execution_time);
    printf("Total Execution time:                                 %10lu\n", Total_execution_time);
    // Print out all caches' statistics
    for (int i = 0; i < num_processors; i++) {
        cout << "============ Simulation results (Cache " << i << ") ============" << endl;
        cache[i]->printStats();
    }

    // for (int i = 0; i < num_processors; i++) {
    //     cout << endl << "============ Simulation results (Cache " << i << ") ============" << endl;
    //     cache[i]->printCache();
    // }

    // Free allocated memory
    for (int i = 0; i < num_processors; i++) {
        delete cache[i];
    }
    free(cache);

    return 0;
}
