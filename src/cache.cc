
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "cache.h"
using namespace std;

Cache::Cache(int s,int a,int b )
{
   
   ulong i, j;

 lineSize = (ulong)(b);
        sets = 1;               // Only one set for fully associative
        assoc = 10000;        // Large associativity to simulate "infinite" cache
        numLines = assoc;       // The number of lines is equal to associativity
  
   
 
cache = new cacheLine*[sets];
    for (i = 0; i < sets; i++) {
        cache[i] = new cacheLine[assoc];
        for (j = 0; j < assoc; j++) {
            cache[i][j].invalidate();
        }
    }    
   
}


void Cache::MESI_Processor_Access(ulong addr,uchar rw, int copy , Cache **cache, int processor, int num_processors, int& bus_signal, int& time)
{
    cacheLine* line;
    line = findLine(addr);
    // cout << copy << endl;

    // Dealing with processor reads first
    if (rw == 'r') {
        incReads();                 // Increment total reads

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // If line doesn't exist in cache: 
        // 1. Increment read misses
        // 2. Fill the cache line
        // 3. If copy exists, update state to SHARED, else update it to EXCLUSIVE
        // 4. Send out BusRd in case copies exist
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        if (!line) {
            incReadMisses();
            // cout << "READ MISS" << endl;
            line = fillLine(addr);
            if (copy) {
                bus_signal = READ;
                line->setFlags(SHARED);
                // incFlushes();
                // cout << "Read miss + copy - " << hex << addr << endl;
                // Total time += flush
                time = flush_transfer;
                return;

            }
            else {
                bus_signal = NO_OP;
                line->setFlags(EXCLUSIVE);
                incMemAccess();
                // cout << "Read miss + no copy - " << hex << addr << endl;
                // Total time += mem_access
                time = memory_latency;
                return;
            } 
            assert(0);
        }

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // If line exists in cache:
        // 1. Increment read hits
        // 2. No state changes happen within the processor based on a read and so nothing sent to be snooped
        // 3. Time is just hit time
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        else {
            incReadHits();
            bus_signal = NO_OP;
            // cout << "Read hit - " << hex << addr << endl;
            //  Total time +=  read hit time
            time = read_hit_latency;
            // cout << "READ HIT" << endl;
            return;
        }
        assert(0);
    }

    // Dealing with processor writes
    else if (rw == 'w') {
        incWrites();              // Increment total writes

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // If line doesn't exist in cache:
        // 1. Increment write misses
        // 2. Fill cache line
        // 3. Change state to MODIFIED
        // 4. If other copies exist, send out a busrdx, otherwise it is not needed
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        if (!line) {
            incWriteMisses();
            // cout << "WRITE MISS" << endl;
            line = fillLine(addr);
            line->setFlags(MODIFIED);
            if (copy) {
                bus_signal = READX;
                // incFlushes();
                // cout << "Write miss + copy - " << hex << addr << endl;
                // Total time += flush
                time = flush_transfer;
                return;
            }
            else {
                bus_signal = NO_OP;
                incMemAccess();
                // cout << "Write miss + no copy - " << hex << addr << endl;
                // Total time += mem_access
                time = memory_latency;
                return;
            } 
            assert(0);
        }

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // If line exists in cache:
        // 1. Increment write hits
        // 2. Based on current state there are different rules:
        //    a. If modified, no change in state and time is hit time
        //    b. If exclusive, change to modified and time 
        //    c. If shared, change to modified and send out bus upgr. Time is 
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        else {
            incWriteHits();
            // cout << "WRITE HIT" << endl;
            // Total time +=  read hit time
            time = write_hit_latency;
            switch (line->getFlags()) {
                case MODIFIED:
                    bus_signal = NO_OP;
                    return;
                
                case EXCLUSIVE:
                    line->setFlags(MODIFIED);
                    bus_signal = NO_OP;
                    return;

                case SHARED:
                    line->setFlags(MODIFIED);
                    bus_signal = UPGR;
                    return;

                default:
                    assert(0 && "Default case not possible");
                    return;
            }
        }
        assert(0);
    }
}

void Cache::MESI_Bus_Snoop(ulong addr , int busread,int busreadx, int busupgrade)
{
    cacheLine* line;
    line = findLine(addr);
    assert(line);               // In this version, we snoop only if we know for sure that the line exists in the processor

    // Split based on the snooped signal. Then if needed split based on current state. for eg: busupgr always means invalidate
    
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // First deal with busreads:
    // 1. If we are in SHARED state, stay SHARED
    // 2. If we are in EXCLUSIVE state, change to SHARED 
    // 3. If we are in MODIFIED state, change to SHARED
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if (busread) {
        assert(!busreadx && !busupgrade);
        incFlushes();
        switch (line->getFlags()) {
            case SHARED:
                return;

            case EXCLUSIVE:
                line->setFlags(SHARED);
                // cout << "INTERVENTION" << endl;
                return;
            
            case MODIFIED:
                line->setFlags(SHARED);
                // cout << "INTERVENTION" << endl;
                return;
            
            default:
                assert(0 && "Default case not possible");
                return;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Next we deal with busreadx:
    // 1. Regardless of our state, we invalidate and it's on us to send the copy via flush
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    else if (busreadx) {
        assert(!busread && !busupgrade);
        incInvalidates();
        incFlushes();
        line->invalidate();
        // cout << "INVALIDATION " << endl;
        return;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Finally we deal with busupgrade:
    // 1. If line exists as modified or exclusive, we can be sure there is an error
    // 2. Other than this, all others invalidate
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    else if (busupgrade) {
        assert(!busread && !busreadx);
        assert((line->getFlags() != MODIFIED) && (line->getFlags() != EXCLUSIVE));
        incInvalidates();
        line->invalidate();
        // cout << "INVALIDATION " << endl;
        return;
    }
}

void Cache::MOESI_Processor_Access(ulong addr,uchar rw, int copy, Cache **cache, int processor, int num_processors, int& bus_signal, int& time)
{
    cacheLine* line;
    line = findLine(addr);
    // cout << copy << endl;

    // Dealing with processor reads first
    if (rw == 'r') {
        incReads();                 // Increment total reads

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // If line doesn't exist in cache: 
        // 1. Increment read misses
        // 2. Fill the cache line
        // 3. If copy exists, update state to SHARED, else update it to EXCLUSIVE
        // 4. Send out BusRd in case copies exist
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        if (!line) {
            incReadMisses();
            // cout << "READ MISS" << endl;
            line = fillLine(addr);
            if (copy) {
                bus_signal = READ;
                line->setFlags(SHARED);
                // incFlushes();
                // cout << "Read miss + copy - " << hex << addr << endl;
                // Total time += flush
                time = flush_transfer;
                return;

            }
            else {
                bus_signal = NO_OP;
                line->setFlags(EXCLUSIVE);
                incMemAccess();
                // cout << "Read miss + no copy - " << hex << addr << endl;
                // Total time += mem_access
                time = memory_latency;
                return;
            } 
            assert(0);
        }

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // If line exists in cache:
        // 1. Increment read hits
        // 2. No state changes happen within the processor based on a read and so nothing sent to be snooped
        // 3. Time is just hit time
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        else {
            incReadHits();
            bus_signal = NO_OP;
            // cout << "Read hit - " << hex << addr << endl;
            //  Total time +=  read hit time
            time = read_hit_latency;
            // cout << "READ HIT" << endl;
            return;
        }
        assert(0);
    }

    // Dealing with processor writes
    else if (rw == 'w') {
        incWrites();              // Increment total writes

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // If line doesn't exist in cache:
        // 1. Increment write misses
        // 2. Fill cache line
        // 3. Change state to MODIFIED
        // 4. If other copies exist, send out a busrdx, otherwise it is not needed
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        if (!line) {
            incWriteMisses();
            // cout << "WRITE MISS" << endl;
            line = fillLine(addr);
            line->setFlags(MODIFIED);
            if (copy) {
                bus_signal = READX;
                // incFlushes();
                // cout << "Write miss + copy - " << hex << addr << endl;
                // Total time += flush
                time = flush_transfer;
                return;
            }
            else {
                bus_signal = NO_OP;
                incMemAccess();
                // cout << "Write miss + no copy - " << hex << addr << endl;
                // Total time += mem_access
                time = memory_latency;
                return;
            } 
            assert(0);
        }

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // If line exists in cache:
        // 1. Increment write hits
        // 2. Based on current state there are different rules:
        //    a. If modified, no change in state and time is hit time
        //    b. If exclusive, change to modified and time 
        //    c. If shared, change to modified and send out bus upgr. Time is 
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        else {
            incWriteHits();
            // cout << "WRITE HIT" << endl;
            // Total time +=  read hit time
            time = write_hit_latency;
            switch (line->getFlags()) {
                case MODIFIED:
                    bus_signal = NO_OP;
                    return;
                
                case EXCLUSIVE:
                    line->setFlags(MODIFIED);
                    bus_signal = NO_OP;
                    return;

                case SHARED:
                    line->setFlags(MODIFIED);
                    bus_signal = UPGR;
                    return;

                case OWNED:
                    line->setFlags(MODIFIED);
                    bus_signal = UPGR;
                    return;

                default:
                    assert(0 && "Default case not possible");
                    return;
            }
        }
        assert(0);
    }
}

void Cache::MOESI_Bus_Snoop(ulong addr , int busread,int busreadx, int busupgrade)
{
    cacheLine* line;
    line = findLine(addr);
    assert(line);               // In this version, we snoop only if we know for sure that the line exists in the processor

    // Split based on the snooped signal. Then if needed split based on current state. for eg: busupgr always means invalidate
    
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // First deal with busreads:
    // 1. If we are in SHARED state, stay SHARED
    // 2. If we are in EXCLUSIVE state, change to SHARED 
    // 3. If we are in MODIFIED state, change to SHARED
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if (busread) {
        assert(!busreadx && !busupgrade);
        // incFlushes();
        switch (line->getFlags()) {
            case SHARED:
                return;

            case EXCLUSIVE:
                line->setFlags(SHARED);
                // incFlushes();
                // cout << "INTERVENTION" << endl;
                return;
            
            case MODIFIED:
                incFlushes();
                line->setFlags(OWNED);
                // cout << "INTERVENTION" << endl;
                return;

            case OWNED:
                incFlushes();
                return;
            
            default:
                assert(0 && "Default case not possible");
                return;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Next we deal with busreadx:
    // 1. Regardless of our state, we invalidate and it's on us to send the copy via flush
    // 2. If in MODIFIED or OWNED state, increment Flush
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    else if (busreadx) {
        assert(!busread && !busupgrade);
        if ((line->getFlags() == OWNED) || (line->getFlags() == MODIFIED)) incFlushes();
        line->invalidate();
        incInvalidates();
        // cout << "INVALIDATION " << endl;
        return;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Finally we deal with busupgrade:
    // 1. If line exists as modified or exclusive, we can be sure there is an error
    // 2. Other than this, all others invalidate
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    else if (busupgrade) {
        assert(!busread && !busreadx);
        assert((line->getFlags() != MODIFIED) && (line->getFlags() != EXCLUSIVE));
        line->invalidate();
        incInvalidates();
        // cout << "INVALIDATION " << endl;
        return;
    }	
}

/*look up line*/
cacheLine * Cache::findLine(ulong addr)
{
   ulong i, j, tag, pos;
   
   pos = assoc;
   tag = calcTag(addr);
   i   = calcIndex(addr);
//    cout << "TAG AFT FINDLINE: " << hex << tag << endl;
  
   for(j=0; j<assoc; j++)
	if(cache[i][j].isValid())
	        if(cache[i][j].getTag() == tag)
		{
		     pos = j; break; 
		}
   if(pos == assoc)
	return NULL;
   else
	return &(cache[i][pos]); 
}


/*Most of these functions are redundant so you can use/change it if you want to*/

/*upgrade LRU line to be MRU line*/
void Cache::updateLRU(cacheLine *line)
{
  line->setSeq(currentCycle);  
}

/*return an invalid line as LRU, if any, otherwise return LRU line*/
cacheLine * Cache::getLRU(ulong addr)
{
   ulong i, j, victim, min;

   victim = assoc;
   min    = currentCycle;
   i      = calcIndex(addr);
   
   for(j=0;j<assoc;j++)
   {
      if(cache[i][j].isValid() == 0) 
	  return &(cache[i][j]);     
   }   
   for(j=0;j<assoc;j++)
   {
	 if(cache[i][j].getSeq() <= min) { victim = j; min = cache[i][j].getSeq();}
   } 
   assert(victim != assoc);
   std::cout << "victim" << victim << std::endl;
   return &(cache[i][victim]);
}

/*find a victim, move it to MRU position*/
cacheLine *Cache::findLineToReplace(ulong addr)
{
   cacheLine * victim = getLRU(addr);
   updateLRU(victim);
  
   return (victim);
}

/*allocate a new line*/
cacheLine *Cache::fillLine(ulong addr)
{ 
   ulong tag;
  
   cacheLine *victim = findLineToReplace(addr);

   assert(victim != 0);
   if ((victim->getFlags() == MODIFIED))
   {
	   writeBack(addr);
   }
   victim->setFlags(SHARED);	
   tag = calcTag(addr);   
   victim->setTag(tag);
       
 

   return victim;
}

void Cache::printStats()
{ 
	//printf("===== Simulation results      =====\n");
	float miss_rate = (float)(getRM() + getWM()) * 100 / (getReads() + getWrites());
	
printf("01. number of reads:                                 %10lu\n", getReads());
printf("02. number of read misses:                           %10lu\n", getRM());
printf("03. number of writes:                                %10lu\n", getWrites());
printf("04. number of write misses:                          %10lu\n", getWM());
printf("05. number of write hits:                            %10lu\n", getWH());
printf("06. number of read hits:                             %10lu\n", getRH()); // Changed from getRM() to getRH()
printf("07. total miss rate:                                 %10.2f%%\n", miss_rate);
printf("08. number of memory accesses (exclude writebacks):  %10lu\n", mem_trans);
printf("09. number of invalidations:                         %10lu\n", Invalidations());
printf("10. number of flushes:                               %10lu\n", Flushes());

	
}

void Cache::printCacheState(ulong state) {
    switch (state) {
        case INVALID:
            std::cout << "I";
            break;
        case SHARED:
            std::cout << "S";
            break;
        case MODIFIED:
            std::cout << "M";
            break;
        case EXCLUSIVE:
            std::cout << "E";
            break;
        case OWNED:
            std::cout << "O";
            break;
        default:
            std::cout << "-";
            break;
    }
}

void Cache::printCache() {
    sets = getSets();
    assoc = getAssoc();
    for (ulong i = 0; i < sets; i++) {
        // cout << "SET: " << i << endl;
        for (ulong j = 0; j < assoc; j++) {
            if (cache[i][j].getTag() != 0) {
                cout << "LINE: " << dec << j << "\t" << cache[i][j].getTag() << "\t" << cache[i][j].isValid() << endl;
            }
        }
    } 
}
