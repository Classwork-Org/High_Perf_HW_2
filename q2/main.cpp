#include <sys/signal.h>
#include <queue>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <iostream>
#include <bits/stdc++.h> 
#include <chrono> 

// Use auto keyword to avoid typing long 
// type definitions to get the timepoint 
// at this instant use function now() 

// C++ program to print print all primes smaller than 
// n using segmented sieve 
using std::cout; 
using std::endl;
using std::vector; 
using namespace std::chrono;

#define THREAD_COUNT 12
// #define L1_CACHE INT_MAX
#define L1_CACHE 32768
// #define PRINT_PRIMES
// #define DEBUG

// This functions finds all primes smaller than 'limit' 
// using simple sieve of eratosthenes. It also stores 
// found primes in vector prime[] 
void simpleSieve(int limit, vector<int> &prime) 
{ 
    // Create a boolean array "mark[0..n-1]" and initialize 
    // all entries of it as true. A value in mark[p] will 
    // finally be false if 'p' is Not a prime, else true. 
    bool *mark = new bool[limit+1]; 
    memset(mark, true, limit+1); 

    for (int p=2; p*p<limit; p++) 
    { 
        // If p is not changed, then it is a prime 
        if (mark[p] == true) 
        { 
            // Update all multiples of p 
            for (int i=p*2; i<limit; i+=p) 
                mark[i] = false; 
        } 
    } 
  
    // // Print all prime numbers and store them in prime 
    for (int p=2; p<limit; p++) 
    { 
        if (mark[p] == true) 
        { 
            prime.push_back(p); 
            // cout << p << " "; 
        } 
    } 
    // cout<<endl;
    delete[] mark;
} 

pthread_t threads[THREAD_COUNT];

struct ParSegArg {
    vector<int> *prime;
    int segGroupLower;
    int segGroupUpper;
    int limit;
    int threadId;
} threadArgs[THREAD_COUNT];

void* parallel_segment_process(void *arg)
{
    ParSegArg* structArg = ((ParSegArg*) arg);
    vector<int> &prime = *(structArg->prime);
    int segGroupLower = structArg->segGroupLower;
    int segGroupUpper = structArg->segGroupUpper;
    int limit = structArg->limit;
    int threadId = structArg->threadId;

    int low = segGroupLower; 
    int high;

    if(limit>L1_CACHE-1)
    {
        limit = L1_CACHE;
        high = segGroupLower+limit;
    }
    else
    {
        high = segGroupUpper;
    }

    bool *mark = new bool[limit]; 

    // While all segments of range [0..n-1] are not processed, 
    // process one segment at a time 
    while (low < segGroupUpper) 
    { 
        if (high >= segGroupUpper)
        {          
            high = segGroupUpper; 
        }
        #ifdef DEBUG
            printf("Thread(%d) is printing primes between [%d]->[%d]\n", threadId, low, high);
        #endif 
        // To mark primes in current range. A value in mark[i] 
        // will finally be false if 'i-low' is Not a prime, 
        // else true. 
        memset(mark, true, limit); 

        // Use the found primes by simpleSieve() to find 
        // primes in current range 
        for (long unsigned int i = 0; i < prime.size(); i++) 
        { 
             //Get closest to low (could be <= low) that is a multiple of of the current prime
            int loLim = floor(low/prime[i]) * prime[i];

            //If the number is lower than low adjust to the nearest multiple of
            //the prime greater than low. New multiples of a prime can be found
            //by taking other multiples and adding prime to them (basic
            //multiplication by addition)
            if (loLim < low)  
                loLim += prime[i]; 



            //We know that loLim is now a number in the segment low->high that
            //is a multiple of prime[i], to get all other multiples of that same
            //prime just add prime[i] tko lowLim. Mark multiples as not primes
            //but adjust for starting segment size low so that indicies are
            //within segment bounds.
            for (int j=loLim; j<high; j+=prime[i]) 
                mark[j-low] = false; 
        } 

        // Numbers which are not marked as false are prime 
        for (int i = low; i<high; i++) 
            if (mark[i - low] == true) 
            {
                #ifdef PRINT_PRIMES
                    printf("%d ",i);   
                #endif 
            }

        // Update low and high for next segment 
        low = low + limit; 
        high = high + limit; 
    } 
    delete[] mark;

    return 0;
}

// Prints all prime numbers smaller than 'n' 
void segmentedSieve(int n) 
{ 
    // Compute all primes smaller than or equal to square root of n using simple
    // sieve. square root is chosen because all prime numbers <= sqrt(n) will
    // have multiples inside other segments. If other segments have primes
    // (which will be found by the algorithm), THEIR multiples will only appear
    // in segments that exist beyond n. "Their multiples" is important here
    // because their multiples refers to multiples that begin at p*p for a given
    // prime p. All multiples found in segments beyond sqrt(n) are greater than
    // sqrt(n). Therfore since p>sqrt(n) then p*p>p*sqrt(n)>sqrt(n)^2 simplifying:
    // p*p>n.
    
    auto start = high_resolution_clock::now(); 

    int limit = floor(sqrt(n))+1; 
    vector<int> prime;
    simpleSieve(limit, prime);

    auto stop = high_resolution_clock::now(); 
    auto duration = duration_cast<milliseconds>(stop - start); 
    printf("Seq time: %lu\n", duration.count());

    #ifdef PRINT_PRIMES
    printf("Primes <= %d\n", n);
    for(long unsigned i = 0; i<prime.size(); i++)
    {
        printf("%d ",prime[i]);
    }
    #endif
    start = high_resolution_clock::now(); 

    int segGroupLower = limit;
    int plimit = floor((n-sqrt(n))/THREAD_COUNT);
    int segGroupUpper = segGroupLower+plimit;

    for (int threadIndex = 0; threadIndex < THREAD_COUNT; threadIndex++)
    {
        threadArgs[threadIndex].prime = &prime;
        threadArgs[threadIndex].segGroupLower = segGroupLower;
        threadArgs[threadIndex].segGroupUpper = segGroupUpper;
        threadArgs[threadIndex].limit = plimit;
        threadArgs[threadIndex].threadId = threadIndex;

        #ifdef DEBUG
        printf("Thread[%d] is processing segment from [%d]->[%d]\n", threadIndex, segGroupLower, segGroupUpper);
        #endif
        if(pthread_create(&threads[threadIndex], NULL, &parallel_segment_process, (void*)(&threadArgs[threadIndex])))
        {
            printf("Failed to dispatch thread... terminating....\n");
            exit(-1);
        }

        segGroupLower=segGroupUpper;
        if(threadIndex == THREAD_COUNT-2)
        {
            segGroupUpper=n+1;
            plimit = segGroupUpper-segGroupLower+1;
        }
        else
        {
            segGroupUpper+=plimit;
        }
    
    }

    for (int threadIndex = 0; threadIndex < THREAD_COUNT; threadIndex++)
    {
        pthread_join(threads[threadIndex], NULL);
    }

    stop = high_resolution_clock::now(); 
    duration = duration_cast<milliseconds>(stop - start); 
    printf("\nPar time: %lu\n", duration.count());

} 
  
// Driver program to test above function 
int main() 
{ 
    int n = INT_MAX/2; 
    // cout << "Primes smaller than " << n << endl; 
    segmentedSieve(n); 
    cout << endl;
    return 0; 
} 