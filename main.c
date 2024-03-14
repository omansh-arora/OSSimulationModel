#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"

enum ProcessState
{
    RUNNING,
    BLOCKED,
    READY
};

typedef struct PCB_s
{
    int PID;
    enum ProcessState processState;
    int priority;
    struct List *incomingMessages; // Change to pointer to List if List is a struct
} PCB;

static List *allProcesses;
static List *readyZero;
static List *readyOne;
static List *readyTwo;
static List *waitingForReply;
static List *waitingForReceive;

PCB *initProcess;

static int PIDCounter;

static PCB *runningProcess;

void initializeOS();
void createProcess();
void forkProcess();
void static printListPIDs(List *pLlist);
void totalInfo();

void static printListPIDs(List *pList)
{
    List_first(pList);

    for (int i = 0; i < List_count(pList); i++)
    {
        PCB *currProcess = (PCB *)List_curr(pList);
        printf("{PID: %d, State: %d, Priority: %d}, ", currProcess->PID, currProcess->processState, currProcess->priority);
        List_next(pList);
    }
}

void initializeOS()
{
    allProcesses = List_create();
    readyZero = List_create();
    readyOne = List_create();
    readyTwo = List_create();
    waitingForReply = List_create();
    waitingForReceive = List_create();

    initProcess = (PCB *)malloc(sizeof(PCB));
    initProcess->processState = RUNNING;
    initProcess->PID = -1;
    initProcess->incomingMessages = List_create();
    runningProcess = initProcess;
}

void putProcessOnQueue(PCB *process, int priority)
{
    switch (priority)
    {
    case 0:
        List_append(allProcesses, (void *)process);
        List_prepend(readyZero, (void *)process);
        break;
    case 1:
        List_append(allProcesses, (void *)process);
        List_prepend(readyOne, (void *)process);
        break;
    case 2:
        List_append(allProcesses, (void *)process);
        List_prepend(readyTwo, (void *)process);
        break;
    default:
        perror("Error: priority has to be: 0, 1, 2\n");
        free(process);
        PIDCounter--;
        break;
    }
}

void createProcess(int priority)
{
    PCB *newProcess = (PCB *)malloc(sizeof(PCB));
    newProcess->incomingMessages = List_create();
    newProcess->PID = PIDCounter;
    newProcess->processState = READY;
    newProcess->priority = priority;
    PIDCounter++;
    putProcessOnQueue(newProcess, priority);
}

void forkProcess()
{

    if (runningProcess == initProcess)
    {
        perror("Cannot fork init process.");
        return;
    }
    PCB *newProcess = (PCB *)malloc(sizeof(PCB));
    newProcess->incomingMessages = runningProcess->incomingMessages;
    newProcess->PID = PIDCounter;
    newProcess->priority = runningProcess->priority;
    newProcess->processState = READY;
    PIDCounter++;
    putProcessOnQueue(newProcess, newProcess->priority);
}

bool pComparator(void *pItem, void *pComparisonArg)
{
    return (((PCB *)pItem)->PID) == (int *)pComparisonArg;
}

void killProcess(int delPID)
{
    List_first(allProcesses);
    PCB *foundItem = List_search(allProcesses, pComparator, delPID);
    if (foundItem != NULL)
    {
        switch (foundItem->priority)
        {
        case 0:
            List_first(readyZero);
            List_search(readyZero, pComparator, delPID);
            List_remove(readyZero);
            break;
        case 1:
            List_first(readyOne);
            List_search(readyOne, pComparator, delPID);
            List_remove(readyOne);
            break;
        case 2:
            List_first(readyTwo);
            List_search(readyTwo, pComparator, delPID);
            List_remove(readyTwo);
            break;
        default:
            break;
        }
    }
    perror("Process PID not found");
}

void exitRunningProcess()
{
    if (runningProcess != initProcess)
    {
        PCB *oldRunningProcess = runningProcess;
        quantumExpire();
        killProcess(oldRunningProcess->PID);
    }
}

void quantumExpire()
{
    runningProcess->processState = READY;
    putProcessOnQueue(runningProcess, runningProcess->priority);
    if (List_count(readyZero) > 0)
    {
        setProcessAsRunning(readyZero);
    }
    else if (List_count(readyOne) > 0)
    {
        setProcessAsRunning(readyOne);
    }
    else if (List_count(readyTwo) > 0)
    {
        setProcessAsRunning(readyTwo);
    }
    else
    {
    }
}

void totalInfo()
{
    printf("Here are all process queues and contents:\n\n");
    printf("Priority 0: ");
    printListPIDs(readyZero);
    printf("Priority 1: ");
    printListPIDs(readyOne);
    printf("Priority 2: ");
    printListPIDs(readyTwo);
}

void handleInput(char input)
{
    switch (input)
    {
    case 'C':
        int priority;
        printf("Type process priority: \n");
        scanf("%d", &priority);
        createProcess(priority);
        break;
    case 'T':
        totalInfo();
        break;
    default:
        break;
    }
}

int main()
{
    initializeOS();
    while (1)
    {
        char command;
        printf("Type command: \n");
        scanf(" %c", &command);
        handleInput(command);
    }
}