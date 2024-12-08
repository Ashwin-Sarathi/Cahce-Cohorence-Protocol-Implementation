/*******************************************************
                          cache.h
********************************************************/
#ifndef CACHE_H
#define CACHE_H

#include <cmath>
#include <iostream>
typedef unsigned long ulong;
typedef unsigned char uchar;
typedef unsigned int uint;
extern unsigned long Total_execution_time;
/****add new states, based on the protocol****/
enum blockState{
	INVALID,
	SHARED,     
	MODIFIED,     
	EXCLUSIVE,     
	OWNED 
};

enum busSignals {
   NO_OP,
   READ,
   READX,
   UPGR
};
 
class cacheLine 
{
protected:
   ulong tag;
   ulong Flags;    
   ulong seq;     
 
public:
   cacheLine()            { tag = 0; Flags = 0; }
   ulong getTag()         { return tag; }
   ulong getFlags()			{ return Flags;}
   ulong getSeq()         { return seq; }
   void setSeq(ulong Seq)			{ seq = Seq;}
   void setFlags(ulong flags)			{  Flags = flags;}

   void setTag(ulong a)   { tag = a; }
   void invalidate()      { tag = 0; Flags = INVALID; }
   bool isValid()         { return ((Flags) != INVALID); }
};

class Cache
{
protected:
   ulong size, lineSize, assoc, sets, log2Sets, tagMask, numLines;
   ulong reads;
   ulong log2Blk = 0;

   ulong readMisses=0;
   ulong writes=0;
   ulong writeMisses=0;
   ulong writeBacks=0;
   ulong Readhits=0;
   ulong Writehits=0;

   ulong interventions=0;
   ulong invalidations=0;
   ulong mem_trans=0; 
   ulong c_to_c_trans=0; 
   ulong flushes=0;
   ulong busrdxes=0;

   cacheLine **cache;
   // ulong calcTag(ulong addr)     { return (addr >> (log2Blk) );}
   ulong calcIndex(ulong addr)  { return ((addr >> log2Blk) & tagMask);}
   ulong calcAddr4Tag(ulong tag)   { return (tag << (log2Blk));}
   
public:
    ulong currentCycle;  

   
   int busrd, busrdx, busupgr, busupd;
   
  
    Cache(int,int,int);
   ~Cache() { delete cache;}
   
   cacheLine *findLineToReplace(ulong addr);
   cacheLine *fillLine(ulong addr);
   cacheLine * findLine(ulong addr);
   cacheLine * getLRU(ulong);

   ulong calcTag(ulong addr)     { return (addr >> (log2Blk) );}
   
   ulong getRM(){return readMisses;} 
   ulong getWM(){return writeMisses;} 
   ulong getReads(){return reads;}
   ulong getWrites(){return writes;}
   ulong Invalidations(){ return invalidations; }
   ulong Flushes(){ return flushes; }
   ulong BusRdX(){ return busrdxes; }
   ulong getRH(){ return Readhits; }
   ulong getWH(){ return Writehits; }
   ulong getTotalExecution() {return Total_execution_time;}
   ulong getSets() {return sets;}
   ulong getAssoc() {return assoc;}
   int read_hit_latency=1;
   int write_hit_latency=3;
   int memory_latency=100;
   int flush_transfer=15;

   void writeBack(ulong)   { writeBacks++; mem_trans++; }
   void MOESI_Processor_Access(ulong addr,uchar rw, int copy, Cache **cache, int current_processor, int total_processor, int& bus_signal, int& time);
   void MESI_Processor_Access(ulong addr,uchar rw, int copy, Cache **cache,int current_processor, int total_processor, int& bus_signal, int& time);
   void printStats();
   void printCacheState(ulong state);
   void printCache();
   void updateLRU(cacheLine *);
   void MOESI_Bus_Snoop(ulong addr,int busread,int busreadx, int busupgrade);
   void MESI_Bus_Snoop(ulong addr,int busread,int busreadx,int busupgrade);

   int* placeBlock(ulong addr, uchar rw, int copy);
   void setState(uchar rw, int copy);
  
   void incReads() {reads ++;}
   void incWrites() {writes ++;}
   void incReadMisses() {readMisses ++;}
   void incWriteMisses() {writeMisses ++;}
   void incReadHits() {Readhits ++;}
   void incWriteHits() {Writehits ++;}
   void incInvalidates() {invalidations ++;}
   void incFlushes() {flushes ++;}
   void incMemAccess() {mem_trans ++;}
};

#endif
