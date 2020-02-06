#ifndef ARBITER_CPP
#define ARBITER_CPP

#include <vector>
#include <pthread.h>

using std::vector;

class Arbiter
{
private:
    vector<int> priorityVector;
    vector<bool> requestsVector;
    const unsigned int shuffleResetVal = 10;
    unsigned int shuffleTrigger;
    pthread_mutex_t readerCountLock; // Controls access to the reader count
    pthread_mutex_t queueLock;       // Controls access to the queue
    pthread_cond_t headConditional;  // Conditional variable that waits on signal
    int readerCount;                 // The number of reading processes accessing the data
    int read(int (*helper)());
    void write(int (*helper)(int), int);
    void pushHelper(int);
    void popHelper(int);
    int getHeadHelper();
    int getTailHelper();

public:
    Arbiter(/* args */);
    ~Arbiter();
    void push(int);
    void pop();
    int getHead();
    int getTail();
    void waitUntilHeadEqualTo(int);
};

Arbiter::Arbiter(/* args */)
{
    pthread_mutex_init(&readerCountLock, NULL);
    pthread_mutex_init(&queueLock, NULL);
    readerCount = 0;
}

Arbiter::~Arbiter()
{
    pthread_mutex_destroy(&readerCountLock);
    pthread_mutex_destroy(&queueLock);
    pthread_cond_destroy(&headConditional);
}

int Arbiter::read(int (*helper)())
{
    int requestedValue;
    pthread_mutex_lock(&readerCountLock); // lock access to readerCount (to prevent accidental changes)
    readerCount = readerCount + 1;
    if (readerCount == 1)
    {
        pthread_mutex_lock(&queueLock); // if this is the first process to read the queue lock out writer
                                        // This will no be called again because all other readers will
                                        // not be first readers
    }
    pthread_mutex_unlock(&readerCountLock); // Done modifying the readerCount, can release it

    requestedValue = (*helper)(); // call requested helper and read requested data

    pthread_mutex_lock(&readerCountLock); // gain access to readerCount
    readerCount = readerCount - 1;        // decrement readerCount
    if (readerCount == 0)
    {
        pthread_mutex_unlock(&queueLock); // if this is the last reader unlock queue for writers
    }
    pthread_mutex_unlock(&readerCountLock); // release reader lock so that other readers can read
}

void Arbiter::write(int (*helper)(int), int arg)
{
    int returnVal;
    pthread_mutex_lock(&queueLock); // gain access to the database
    (*helper)(arg);                 // write information to the database
    if (helper == this->popHelper)  // Someone was removed from the queue, signal to waiting
    {
        pthread_cond_signal(&headConditional);
    } 
    pthread_mutex_unlock(&queueLock); // release exclusive access to the database
}

/**
 * @brief // ! 
 *
 * 
 * @param val 
 */
void Arbiter::waitUntilHeadEqualTo(int val)
{
    int currentHead = getHead();
    while (currentHead != val)
    {
        //block until a change is detected
        pthread_cond_wait()
        currentHead = getHead();
    }
}

void Arbiter::push(int value)
{
    write(&(this->pushHelper), value);
}

void Arbiter::pop()
{
    return write(&(this->popHelper), 0);
}

int Arbiter::getHead()
{
    int returnValue = read(&(this->getHeadHelper));
    return returnValue;
}

int Arbiter::getTail()
{
    int returnValue = read(&(this->getTailHelper));
    return returnValue;
}

void Arbiter::pushHelper(int value)
{
    this->.push(value);
}

void Arbiter::popHelper(int value)
{
    this->.pop();
}

int Arbiter::getHeadHelper()
{
    return this->.front();
}

int Arbiter::getTailHelper()
{
    return this->.back();
}

#endif
