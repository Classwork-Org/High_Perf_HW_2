#include <sys/signal.h>
#include <queue>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <iostream>
#include "Scheduler.cpp"


#define SUCCESS true
#define FAIL false
#define MSEC 1000

#define GREEDY
#define GET_FORKS_IN_ORDER
// #define PROLETARIAT
#define RUN_TIME_IN_MSEC 5000
#define MAX_SLEEP_PERIOD 100 
#define SCHED_UPDATE_FREQ 50
#define SCHED_SHUFFLE_TRIGGER 4
#define SCHED_ENABLE_SHUFFLE true

using std::queue;
using std::swap;

unsigned int philosopherCount = 5;
unsigned int forksCount = 5;
bool stillSitting = true;
enum PhilosopherState {THINKING, PICKING_UP_FORK, EATING, PUTTING_DOWN_FORKS};
pthread_t *philosophers;
pthread_t terminatorThread;
pthread_t schedulerThread;
pthread_mutex_t *forks;
unsigned int *threadArgs;
Scheduler *sched;

void* runScheduler(void *args)
{
    sched->run();
    return 0;
}

/**
 * @brief Sends termination signal through bool stillSitting to threads. Since
 * some threads may be sleeping (THINKING/ EATING) they will not respond
 * immediately and will have to finish up their usage of the resource in order
 * to leave it in a consistent state
 * 
 * @param args time that terminator waits before terminating, cast to unsigned int
 * @return void* 
 */
void* terminator(void *args)
{
    unsigned int timeTillTermination = *((unsigned int*)args);
    usleep(MSEC*timeTillTermination);    
    stillSitting = false;
    printf("Terminator has just told everyone to leave and cleanup... this may take a while...\n");
    return 0;
}

/**
 * @brief seeds srand, static variable prevents reseeding 
 * 
 */
void seed_random_generator()
{
    static bool hasSeeded = false;
    if(!hasSeeded)
    {
        srand(time(NULL));
        hasSeeded = true;
    }
}

unsigned int get_sleep_period()
{
    #ifdef PROLETARIAT
        return MAX_SLEEP_PERIOD;
    #else
        return rand()%MAX_SLEEP_PERIOD;
    #endif
}

void sort(unsigned int *first, unsigned int *second)
{
    unsigned int temp; 
    if(*first > *second)
    {
        temp = *first;
        *first = *second;
        *second = temp;
    }
}

bool pickup_fork(unsigned int forkNum)
{
    #ifdef GREEDY
    pthread_mutex_lock(&forks[forkNum]);
    #else
    if(!(pthread_mutex_trylock(&forks[forkNum])))
    {
        return false;
    }
    #endif

    return true;
}

std::string convert_enum(PhilosopherState state)
{
    switch(state)
    {
        case THINKING:
            return "THINKING";
        case EATING:
            return "EATING";
        case PUTTING_DOWN_FORKS:
            return "PUTTING_DOWN_FORKS";
        case PICKING_UP_FORK:
            return "PICKING_UP_FORKS";
        default:
            return "INVALID STATE!";
    }
}

void putdown_fork(unsigned int fork)
{
    pthread_mutex_unlock(&forks[fork]);
}

/**
 * @brief This function is basically the main for each philosopher. It
 * identifies the philosopher by his thread number, and defines which forks he
 * should take first. If the GET_FORKS_IN_ORDER flag is raised she will pickup
 * the forks depending their ascending index order. This is done via a sort
 * call. This guarantees that on any given table there will always be a left
 * philosopher (philosopher that will pickup the left fork first before the
 * right one). Otherwise the order will always be left fork then right fork with
 * no righty philosophers. To guarantee no starvation a scheduler has been
 * implemented with dynamic and fixed priority for philosophers
 *
 * @param args thread number based on dispatch order, becomes philosoper num
 * @return void* 
 */
void* sit_down_on_table(void* args)
{
    unsigned int philosopherNumber =  *((unsigned int*)args);
    unsigned int firstFork = philosopherNumber;
    unsigned int secondFork = (philosopherNumber+1)%forksCount;
    bool tryToPickUpSecondForks = false;
    bool tryToPickUpFirstFork = false;


    PhilosopherState state = THINKING;
    #ifdef GET_FORKS_IN_ORDER
    sort(&firstFork, &secondFork);
    #endif
    while(stillSitting)
    {
        switch (state)
        {
        case THINKING:
        {
            unsigned int thinkingPeriod = get_sleep_period();
            printf("Philosopher[%d] is about to THINK for %d milliseconds\n", philosopherNumber, thinkingPeriod);
            usleep(MSEC*thinkingPeriod);
            state = PICKING_UP_FORK;
            break;
        }
            
        case PICKING_UP_FORK:
        {
            /**
             * @brief will work with both greedy and non greedy philosophers
             * greedy will block until has both, non greedy will fail to get 
             * both and back off (will relinquish fork that was successfully 
             * acquired if one was acquired).  
             */
            printf("Philosopher[%d] is asking scheduler to eat\n", philosopherNumber);
            sched->make_eat_request(philosopherNumber);
            printf("Philosopher[%d] has been granted the ability to eat by scheduler\n", philosopherNumber);

            printf("Philosopher[%d] is trying to pickup firstFork[%d]\n", philosopherNumber, firstFork);
            tryToPickUpFirstFork = pickup_fork(firstFork);
            printf("Philosopher[%d] has picked-up firstFork[%d]!!\n", philosopherNumber, firstFork);
            printf("Philosopher[%d] is trying to pickup secondFork[%d]\n", philosopherNumber, secondFork);
            tryToPickUpSecondForks = pickup_fork(secondFork);
            printf("Philosopher[%d] has picked-up secondFork[%d]!!\n", philosopherNumber, secondFork);
            if(tryToPickUpFirstFork == SUCCESS && tryToPickUpSecondForks == SUCCESS)
            {
                state = EATING;
            }
            else
            {
                printf("Philosopher[%d] couldn't pickup both forks\n", philosopherNumber);
                if(tryToPickUpFirstFork)
                {
                    printf("Philosopher[%d] was holding on to firstFork[%d] and has now released it\n", philosopherNumber, firstFork);
                    putdown_fork(firstFork);
                }
                if(tryToPickUpSecondForks)
                {
                    printf("Philosopher[%d] was holding on to secondFork[%d] and has now released it\n", philosopherNumber, secondFork);
                    putdown_fork(secondFork);
                }
                state = THINKING;
            }
            
            break;
        }
            
        case EATING:
        {
            unsigned int eatingPeriod = get_sleep_period();
            printf("Philosopher[%d] is about to EAT for %d milliseconds\n", philosopherNumber, eatingPeriod);
            usleep(MSEC*eatingPeriod);
            state = PUTTING_DOWN_FORKS;
            break;
        }
            
        case PUTTING_DOWN_FORKS:
        {
            printf("Philosopher[%d] is putting down fork %d\n", philosopherNumber, secondFork);
            putdown_fork(secondFork);
            printf("Philosopher[%d] is putting down fork %d\n", philosopherNumber, firstFork);
            putdown_fork(firstFork);
            state = THINKING;
            break;
        }
            
        default:
            break;
        }
    }
    printf("Philosopher[%d] is done exploring the mysteries of the universe, he was last seen %s\n", philosopherNumber, (convert_enum(state)).c_str());

    if(tryToPickUpFirstFork)
    {
        printf("Philosopher[%d] was holding fork number %d and is now putting it down\n", philosopherNumber, firstFork);
        putdown_fork(firstFork);
    }
    if(tryToPickUpSecondForks)
    {
        printf("Philosopher[%d] was holding fork number %d and is now putting it down\n", philosopherNumber, secondFork);
        putdown_fork(secondFork);
    }
    return 0;
}

int parse_args(int argc, char const *argv[])
{
    if(argc != 3)
    {
        
        printf("Invalid number of arguments received, need number of philosophers and forks!\n");
        return -1;
    }
    else
    {
        philosopherCount = atoi(argv[1]);
        forksCount = atoi(argv[2]);
        if(forksCount < philosopherCount)
        {
            printf("Forks cannot be less than philosophers!\n");
            return -1;
        }
        printf("Received philosopher count %d\n", philosopherCount);
        printf("Received forks count %d\n", forksCount);
    }
    return 0;
}

int init_sim()
{
	try 
    {
        forks = new pthread_mutex_t[forksCount];
        philosophers = new pthread_t[philosopherCount];
        threadArgs = new unsigned int[philosopherCount];
        sched = new Scheduler(philosopherCount, &stillSitting, SCHED_UPDATE_FREQ, SCHED_SHUFFLE_TRIGGER, SCHED_ENABLE_SHUFFLE);
	}
	catch (const std::bad_alloc& e) 
    {
        printf("Failed to get enough forks and seats, and that waiter! oh deer, financial ruin is imminent\n");
		std::cout << "Allocation failed: " << e.what() << '\n';
        return -1;
	}		

    for (unsigned int mutexIndex = 0; mutexIndex < forksCount; mutexIndex++)
    {
        pthread_mutex_init(&forks[mutexIndex], NULL);
    }

    for (unsigned int threadNumber = 0; threadNumber < philosopherCount; threadNumber++)
    {
        threadArgs[threadNumber] = threadNumber;
        if(pthread_create(&philosophers[threadNumber], NULL, &sit_down_on_table, (void*)(&threadArgs[threadNumber])) != 0)
        {
            printf("Failed to dispatch philosopher thread, terminating....\n");
            return -1;
        }
    }
    
    printf("Main is dispatching terminator thread (spooky...)\n");

    unsigned int terminationTime = RUN_TIME_IN_MSEC;
    if(pthread_create(&terminatorThread, NULL, &terminator, (void*)(&terminationTime)))
    {
        printf("Failed to dispatch terminator... terminating (ironically)....\n");
        return -1;
    }

    printf("Main is dispatching scheduler thread\n");

    if(pthread_create(&schedulerThread, NULL, &runScheduler, NULL))
    {
        printf("Failed to dispatch terminator... terminating (ironically)....\n");
        return -1;
    }
    return 0;
}

void wait_until_sim_termination()
{
    for (unsigned int threadNumber = 0; threadNumber < philosopherCount; threadNumber++)
    {
        printf("main is waiting for thread [%d] to join\n", threadNumber);
        pthread_join(philosophers[threadNumber], NULL);
    }
}

void cleanup_sim()
{
    for (unsigned int mutexIndex = 0; mutexIndex < forksCount; mutexIndex++)
    {
        printf("Main destroying fork[%d]\n", mutexIndex);
        pthread_mutex_destroy(&forks[mutexIndex]);
    }
    delete sched;
}

int main(int argc, char const *argv[])
{
    printf("Simulation starting...\n");

    if(parse_args(argc, argv))
    {
        exit(-1);
    }
    
    if(init_sim())
    {
        exit(-1);
    }

    wait_until_sim_termination();

    cleanup_sim();

    printf("Simulation has concluded successfully\n");
    
    return 0;
}
