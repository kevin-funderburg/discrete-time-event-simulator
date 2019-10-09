/***
 * CPU Scheduler Simulation
 * @file sim.cpp
 * @authors: Kevin Funderburg, Rob Murray
 */

/////////////////////////////////////////////////
// NOTES
// avgnumprocesses 1 / lambda * num_proceesses i think
// Use 105 ms for the preemptive _SRTF
/////////////////////////////////////////////////
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cmath>
#include <list>
#include <queue>
//#include "event.h"
#include "header.h"
using namespace std;
/////////////////////////////////////////////////
void parseArgs(int argc, char *argv[])
{
    scheduler = static_cast<Scheduler>(stoi(argv[1]));  // set scheduler
    lambda = stoi(argv[2]);
//    lambda = (float)1.0 / stof(argv[2]);
    avgServiceTime = stof(argv[3]);
    if (argc == 5)
        float quantum = stof(argv[4]);
}


static void show_usage()
{
    cerr << "Usage: sim [123] [average arrival rate] [average service time] [quantum interval]\n\n"
         << "Options:\n"
         << "\t1 : First-Come First-Served (_FCFS)\n"
         << "\t2 : Shortest Remaining Time First (_SRTF)\n"
         << "\t3 : Round Robin, with different quantum values (_RR) (requires 4 arguments)\n";
}


void init()
{
//    mu = (float)1.0 / avgServiceTime;
    quantumClock = 0.0;
    lastid = 0;

    avgArrivalTime = 1.0/(float)lambda;

    cpu_head = new cpuNode;
    cpu_head->clock = 0.0;
    cpu_head->busy = false;
    cpu_head->p_link = NULL;

    pl_head = new procListNode;
    pl_head->pid = lastid;
    pl_head->arrivalTime = genexp(avgArrivalTime);
    pl_head->startTime = 0.0;
    pl_head->reStartTime = 0.0;
    pl_head->finishTime = 0.0;
//    pl_head->burst = genexp(mu);
    pl_head->burst = genexp(avgServiceTime);
    pl_head->remainingTime = pl_head->burst;
    pl_head->pl_next = NULL;

    eq_head = new eventQNode;
    eq_head->time = pl_head->arrivalTime;
    eq_head->type = ARRIVE;
    eq_head->eq_next = NULL;
    eq_head->p_link = pl_head;
}


////////////////////////////////////////////////////////////////
void generate_report()
{
    clog << "outputting stats\n";
    ofstream data("data.txt",  ios::out | ios::app);
    data << "Average Turnaround Time\n";// << lambda << "\t\t" << getAvgTurnaround();
//    if (data.is_open())
//    {
//    }
//    else cout << "Unable to open file";
//        data.close();
}


int genID() { return ++lastid; }


//void addToEventQ(event *newEvent)
//{
//    debugging(newEvent);
////    eventQ.push(newEvent);
//}


//void debugging(event *newEvent)
//{
//    clog << "DEBUG [" << cpu_head->clock << "] - adding event: ";
//    switch (newEvent->type) {
//        case ARRIVE: clog << "ARRIVE"; break;
//        case DEPARTURE: clog << "DEPARTURE"; break;
//        case DISPATCH: clog << "DISPATCH"; break;
//        default: cerr << "invalid type";
//    }
//    clog << "\n\twill happen at time: [" << newEvent->time << "]\n"
//         << "\tattached to process id: [" << newEvent->pid << "]\n";
//
////    clog << "\n\tQUEUE SIZES:"
////         << "\tevent: [" << eventQ.size()
////         << "]\tready: [" << readyQ.size()
////         << "] process list: [" << pList.size() << "]\n";
//}


////////////////////////////////////////////////////////////////
/// @return a random number between 0 and 1
float urand() { return( (float) rand()/RAND_MAX ); }


/////////////////////////////////////////////////////////////
/// @return a random number that follows an exp distribution
float genexp(float lambda)
{
    float u,x;
    x = 0;
    while (x == 0)
    {
        u = urand();
        x = (-1/lambda)*log(u);
    }
    return(x);
}


void scheduleArrival()
{

    procListNode *pl_cursor = pl_head;
    while (pl_cursor->pl_next != NULL)
        pl_cursor = pl_cursor->pl_next;

    pl_cursor->pl_next = new procListNode;
    pl_cursor->pl_next->pid = pl_cursor->pid + 1;
    pl_cursor->pl_next->arrivalTime = pl_cursor->arrivalTime + genexp(avgArrivalTime);
    pl_cursor->pl_next->startTime = 0.0;
    pl_cursor->pl_next->reStartTime = 0.0;
    pl_cursor->pl_next->finishTime = 0.0;
    pl_cursor->pl_next->burst = genexp(avgServiceTime);
//    pl_cursor->pl_next->burst = genexp(mu);
    pl_cursor->pl_next->remainingTime = pl_cursor->pl_next->burst;
    pl_cursor->pl_next->pl_next = NULL;

    eventQNode *arrival = new eventQNode;
    arrival->time = pl_cursor->pl_next->arrivalTime;
    arrival->type = ARRIVE;
    arrival->p_link = pl_cursor->pl_next;
    arrival->eq_next = NULL;

    insertIntoEventQ(arrival);
}

//Push process into ready queue and remove event from event queue
void handleArrival()
{
    readyQNode *ready = new readyQNode;
    ready->p_link = eq_head->p_link;
    ready->rq_next = NULL;

    if (rq_head == NULL)  //empty queue
        rq_head = ready;
    else
    {
        readyQNode *rq_cursor = rq_head;
        while (rq_cursor->rq_next != NULL)
            rq_cursor = rq_cursor->rq_next;

        rq_cursor->rq_next = ready;
    }

    popEventQHead();
}


void scheduleDispatch()
{

    eventQNode *dispatch = new eventQNode;

    procListNode *nextProc;
    if (scheduler == _FCFS)
        nextProc = rq_head->p_link;
    else if (scheduler == _SRTF)
    {
        if (cpu_head->clock > rq_head->p_link->arrivalTime)
        {
//            nextProc = getSRTProcess();
        }
        else
        {
            nextProc = rq_head->p_link;
        }
    }
    else if (scheduler == _RR)
    {
//        nextProc = getHRRProcess();
    }

    if (cpu_head->clock < nextProc->arrivalTime)
        dispatch->time = nextProc->arrivalTime;
    else
        dispatch->time = cpu_head->clock;

    dispatch->type = DISPATCH;
    dispatch->eq_next = NULL;
    dispatch->p_link = nextProc;

    insertIntoEventQ(dispatch);
}

void handleDispatch()
{
    cpu_head->p_link = eq_head->p_link;  //assign process to CPU

    if (scheduler == _SRTF || scheduler == _RR)
    {
        readyQNode *rq_cursor = rq_head->rq_next;
        readyQNode *rq_precursor = rq_head;
        if (rq_precursor->p_link->arrivalTime != eq_head->p_link->arrivalTime)
        {
            while (rq_cursor != NULL)
            {
                if (rq_cursor->p_link->arrivalTime == eq_head->p_link->arrivalTime)
                {
                    rq_precursor->rq_next = rq_cursor->rq_next;
                    rq_cursor->rq_next = rq_head;
                    rq_head = rq_cursor;
                    break;
                }
                rq_cursor = rq_cursor->rq_next;
                rq_precursor = rq_precursor->rq_next;
            }
        }
    }

    popReadyQHead();
    popEventQHead();

    cpu_head->busy = true;    //CPU now busy executing process

    //set clock to time of process arrival
    if (cpu_head->clock < cpu_head->p_link->arrivalTime)
        cpu_head->clock = cpu_head->p_link->arrivalTime;

    if (cpu_head->p_link->startTime == 0)
        cpu_head->p_link->startTime = cpu_head->clock;
    else
        cpu_head->p_link->reStartTime = cpu_head->clock;
}

void scheduleDeparture()
{
    eventQNode *departure = new eventQNode;
    departure->type = DEPARTURE;
    departure->eq_next = NULL;
    departure->p_link = cpu_head->p_link;

    if (scheduler == _FCFS || scheduler == _RR)
        departure->time = cpu_head->p_link->startTime + cpu_head->p_link->remainingTime;  //FCFS will process to completion

    else if (scheduler == _SRTF)
    {
        if (cpu_head->p_link->reStartTime == 0)
            departure->time = cpu_head->p_link->startTime + cpu_head->p_link->remainingTime;
        else
            departure->time = cpu_head->p_link->reStartTime + cpu_head->p_link->remainingTime;
    }

    insertIntoEventQ(departure);
}

void handleDeparture()
{
    cpu_head->clock = eq_head->time;   //set clock to head of event queue

    cpu_head->p_link->finishTime = cpu_head->clock;    //log finishTime
    pl_head->finishTime = cpu_head->p_link->finishTime;

    cpu_head->p_link->remainingTime = 0.0;    //clear remainingTime
    cpu_head->p_link = NULL;

    cpu_head->busy = false;   //CPU ready for next process

    popEventQHead();    //remove departure event
}

void schedulePreemption()
{
    //TODO
}


////////////////////////////////////////////////////////////
int run_sim()
{
    switch (scheduler)
    {
        case _FCFS: cout << "Scheduling as FCFS\n\n";
            FCFS();
            break;
        case _SRTF: cout << "Scheduling as SRTF\n\n";
            SRTF();
            break;
        case _RR: cout << "Scheduling as RR\n\n";
            RR();
            break;
        default: cerr << "invalid scheduler\n"; return 1;
    }
    return 0;
}


void FCFS()
{
    int arrivalCount = 0;
    int departureCount = 0;
    int dispatchCount = 0;

    while (departureCount < MAX_PROCESSES)
    {
        if (!cpu_head->busy)
        {
            scheduleArrival();
            if (rq_head != NULL)
                scheduleDispatch();
        }
        else
            scheduleDeparture();

        switch (eq_head->type)
        {
            case ARRIVE:
                handleArrival();
                arrivalCount++;
                break;
            case DISPATCH:
                handleDispatch();
                dispatchCount++;
                break;
            case DEPARTURE:
                handleDeparture();
                departureCount++;
                break;
            default:
                cerr << "invalid event type\n";
        }
    }
    cout << "Arrival Count: " << arrivalCount << endl;
    cout << "Dispatch Count: " << dispatchCount << endl;
    cout << "Departure Count: " << departureCount << endl;
}

// The event queue is treated as a priority queue by sorting
// the events based on time.
void insertIntoEventQ(eventQNode *newEvent)
{
    if (eq_head == NULL)  //empty list
        eq_head = newEvent;
    else if (eq_head->time > newEvent->time)   //add to front
    {
        newEvent->eq_next = eq_head;
        eq_head = newEvent;
    }
    else
    {
        eventQNode *eq_cursor = eq_head;
        while (eq_cursor != NULL)
        {
            if ((eq_cursor->time < newEvent->time) && (eq_cursor->eq_next == NULL))  //add to tail
            {
                eq_cursor->eq_next = newEvent;
                break;
            }
            else if ((eq_cursor->time < newEvent->time) && (eq_cursor->eq_next->time > newEvent->time))   //add inside
            {
                newEvent->eq_next = eq_cursor->eq_next;
                eq_cursor->eq_next = newEvent;
                break;
            }
            else
                eq_cursor = eq_cursor->eq_next;
        }
    }
}
// Helper Function
void popEventQHead()
{
    eventQNode *tempPtr = eq_head;
    eq_head = eq_head->eq_next;
    delete tempPtr;
}

// Helper Function
void popReadyQHead()
{
    readyQNode *tempPtr = rq_head;
    rq_head = rq_head->rq_next;
    delete tempPtr;
}

void SRTF()
{
    //TODO
}


void RR()
{
    //TODO
}

float getAvgTurnaround()
{
    float totalTurnaround = 0.0,
            avgTurnaround = 0.0;

    if (pl_head == NULL)  //empty queue
        cerr << "empty queue";
    else
    {
        procListNode *pl_cursor = pl_head;
        while (pl_cursor->pl_next != NULL)
        {
            if (pl_cursor->finishTime != 0)
            {
                float tmp = pl_cursor->finishTime - pl_cursor->arrivalTime;
                cout << pl_cursor->pid << ": " << "pl_cursor->finishTime - pl_cursor->arrivalTime: "
                    << pl_cursor->finishTime << " - " << pl_cursor->arrivalTime << " = " << tmp << endl;
                totalTurnaround += (pl_cursor->finishTime - pl_cursor->arrivalTime);
                //            cout << "totalTurnaround: " << totalTurnaround << endl;

            }
            pl_cursor = pl_cursor->pl_next;
        }
    }
//    cout << "totalTurnaround: " << totalTurnaround << endl;
    avgTurnaround = totalTurnaround / MAX_PROCESSES;
    return avgTurnaround;
}

int outPut()
{

}
////////////////////////////////////////////////////////////////
int main(int argc, char *argv[] )
{
    clog << "hello\n";

    if (argc < 3)
    {
        show_usage();
        return 1;
    }
    if (*argv[2] == 3 && argc < 5)      // _RR requires the quantum arg
    {
        cerr << "Expected 4 arguments and got " << argc << endl;
        show_usage();
        return 1;
    }
    parseArgs(argc, argv);
    init();
    run_sim();
    generate_report();
    return 0;
}

