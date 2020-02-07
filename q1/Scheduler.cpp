#ifndef Scheduler_CPP
#define Scheduler_CPP

#include <vector>
#include <algorithm>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#define msec 1000

using std::vector;
using std::swap;
using std::find;

class Scheduler
{
private:
    vector<int> priorityVector;
    vector<bool> requestsVector;
    unsigned int shuffleResetVal;
    unsigned int shufflePriorityTrigger;
    int philosopherCount;
    bool* keepRunning;
    pthread_mutex_t requestsVectorLock;
    pthread_cond_t *dispatchSignals;
    unsigned int invocationFrequency;
    bool enableDynamicPriorityShuffle;
    bool has_made_an_eat_request(int);
    int find_philosopher_with_priority(int);
    void shift_priority_vector_left();
    void release_all_philosophers();

public:
    Scheduler(int _philosopherCount, bool* _keepRunning, unsigned int _invocationFrequency, int _shuffleResetVal, bool _enableDynamicPriorityShuffle);
    ~Scheduler();
    void run();
    void make_eat_request(int);
    void dispatch(int);
    void arbitrate();
    void shuffle_priorities();
    void update_philosopher_count(int);
};

Scheduler::Scheduler(int _philosopherCount, bool* _keepRunning, unsigned int _invocationFrequency, int _shuffleResetVal, bool _enableDynamicPriorityShuffle)
{
    
    this->philosopherCount = _philosopherCount;
    this->keepRunning = _keepRunning;
    this->invocationFrequency = _invocationFrequency;
    this->enableDynamicPriorityShuffle = _enableDynamicPriorityShuffle;
    this->shuffleResetVal = _shuffleResetVal;
        
    pthread_mutex_init(&requestsVectorLock, NULL);
    try
    {
        dispatchSignals = new pthread_cond_t[philosopherCount];
    }
    catch (const std::bad_alloc &e)
    {
        printf("Failed to get enough waiters! oh deer, financial ruin is imminent, ERROR %s\n", e.what());
    }
    priorityVector.resize(philosopherCount);
    requestsVector.resize(philosopherCount);

    for(int i = 0; i<philosopherCount; i++)
    {
        priorityVector[i] = i+1;
    }

    // Seperate adjacent philosopher in terms of priority
    for (int i = 1; i < philosopherCount-1; i+=2)
    {
        swap(priorityVector[i], priorityVector[i+1]);
    }

}

void Scheduler::update_philosopher_count(int updatedCount)
{
    this->philosopherCount = updatedCount;
}

/**
 * @brief Destroy the Scheduler:: Scheduler object by destroying the requestsVectorLock and all conditional variables. All vectors will be destroyed 
 * 
 */
Scheduler::~Scheduler()
{
    pthread_mutex_destroy(&requestsVectorLock);

    for (int i = 0; i < philosopherCount; i++)
    { 
        pthread_cond_destroy(&dispatchSignals[i]);
    }
}

bool Scheduler::has_made_an_eat_request(int philosopherIndex)
{
    return requestsVector[philosopherIndex];
}

void Scheduler::make_eat_request(int philosopherIndex)
{
    pthread_mutex_lock(&requestsVectorLock);
    requestsVector[philosopherIndex] = true;
    while(requestsVector[philosopherIndex] == true)
    {
        printf("Philosopher[%d] has made an eat request and is now waiting\n", philosopherIndex);
        pthread_cond_wait(&dispatchSignals[philosopherIndex], &requestsVectorLock);
    }
    printf("Philosopher[%d] was granted the ability to eat by scheduler\n", philosopherIndex);
    pthread_mutex_unlock(&requestsVectorLock);
}

void Scheduler::shuffle_priorities()
{
    int front = *(priorityVector.begin());
    for (long unsigned int i = 0; i < priorityVector.size()-1 ; i++)
    {
        priorityVector[i] = priorityVector[i+1];
    }
    priorityVector[priorityVector.size()-1] = front;
    
}

void Scheduler::release_all_philosophers()
{
    printf("Sched is releasing all philosophers\n");
    for(int i = 0; i < philosopherCount; i++)
    {
        if(requestsVector[i])
        {
            printf("Sched is releasing philosopher[%d]\n", i);
            dispatch(i);
        }
    }
}

void Scheduler::run()
{
    usleep(1000*500);
    while(*keepRunning)
    {
        printf("Sched about to aquire request lock\n");
        pthread_mutex_lock(&requestsVectorLock);
        printf("Sched about has acquired request lock\n");
        arbitrate();
        if(enableDynamicPriorityShuffle && shufflePriorityTrigger == 0)
        {
            printf("Sched shuffling priorities\n");
            shuffle_priorities();
            shufflePriorityTrigger = shuffleResetVal;
        }
        printf("Sched has unlocked requestVector\n");
        pthread_mutex_unlock(&requestsVectorLock);
        printf("Sched about to sleep\n");
        shufflePriorityTrigger--;
        usleep(invocationFrequency*msec);
    }
    release_all_philosophers();
}

int Scheduler::find_philosopher_with_priority(int priority)
{
    auto it = find(priorityVector.begin(), priorityVector.end(), priority);
    
    if(it == priorityVector.end())
    {
        printf("Invalid priority specified in scheduler... Exiting\n");
        exit(-1);
    }
    
    return it - priorityVector.begin();
}

void Scheduler::arbitrate()
{  

    
    for (int priority = 1; priority <= philosopherCount; priority++)
    {
        int philosopherIndex = find_philosopher_with_priority(priority);

        if(has_made_an_eat_request(philosopherIndex))
        {
            printf("Sched is allowing philosopher[%d] to eat\n", philosopherIndex);
            dispatch(philosopherIndex);
        }
    }
    
}

void Scheduler::dispatch(int index)
{
    requestsVector[index] = false;
    pthread_cond_signal(&dispatchSignals[index]);
}

#endif
