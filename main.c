#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
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
    List* readyQueue;
    List *incomingMessages; // Change to pointer to List if List is a struct
    
} PCB;

static List *allProcesses;
static List *readyZero;
static List *readyOne;
static List *readyTwo;
static List *blockedSenders;
static List *blockedReceivers;

PCB *initProcess;

static int PIDCounter;

static PCB *runningProcess;

void initializeOS();
void createProcess();
void forkProcess();
void static printListPIDs(List *pLlist);
void totalInfo();
void quantumExpire();
void setProcessAsRunning(List *queue);
void exitRunningProcess();

void static printListPIDs(List *pList)
{
    List_last(pList);

    for (int i = 0; i < List_count(pList); i++)
    {
        PCB *currProcess = (PCB *)List_curr(pList);
        printf("{PID: %d, State: %d, Priority: %d}, ", currProcess->PID, currProcess->processState, currProcess->priority);
        List_prev(pList);
    }
}

void initializeOS()
{
    allProcesses = List_create();
    readyZero = List_create();
    readyOne = List_create();
    readyTwo = List_create();
    blockedSenders = List_create();
    blockedReceivers = List_create();

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
    return ((PCB *)pItem)->PID == *((int *)pComparisonArg);
}

void killProcess(int delPID)
{
    if (runningProcess->PID == delPID)
    {
        exitRunningProcess();
        return;
    }

    List_first(allProcesses);
    PCB *foundItem = List_search(allProcesses, pComparator, &delPID);
    if (foundItem != NULL)
    {
        switch (foundItem->priority)
        {
        case 0:
            List_first(readyZero);
            List_search(readyZero, pComparator, &delPID);
            List_remove(readyZero);
            List_remove(allProcesses);
            break;
        case 1:
            List_first(readyOne);
            List_search(readyOne, pComparator, &delPID);
            List_remove(readyOne);
            List_remove(allProcesses);
            break;
        case 2:
            List_first(readyTwo);
            List_search(readyTwo, pComparator, &delPID);
            List_remove(readyTwo);
            List_remove(allProcesses);
            break;
        default:
            break;
        }
    }
    else
    {
        perror("Process PID not found");
    }
}

void exitRunningProcess()
{
    if (runningProcess != initProcess)
    {
        List_first(allProcesses);
        List_search(allProcesses, pComparator, &(runningProcess->PID));
        List_remove(allProcesses);
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
            runningProcess->processState = READY;
            runningProcess = initProcess;
            runningProcess->processState = RUNNING;
        }
    }
}

void setProcessAsRunning(List *queue)
{
    runningProcess = (PCB *)List_trim(queue);
    runningProcess->processState = RUNNING;
}

void quantumExpire()
{
    runningProcess->processState = READY;
    if (runningProcess != initProcess)
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
        runningProcess->processState = READY;
        if (runningProcess != initProcess)
            putProcessOnQueue(runningProcess, runningProcess->priority);
        runningProcess = initProcess;
        runningProcess->processState = RUNNING;
    }
}

void totalInfo()
{
    printf("\n*****************************************");
    printf("\nHere are all process queues and contents:\n\n");
    printf("All processes: ");
    printListPIDs(allProcesses);
    printf("\n");

    printf("Priority 0: ");
    printListPIDs(readyZero);
    printf("\n");

    printf("Priority 1: ");
    printListPIDs(readyOne);
    printf("\n");

    printf("Priority 2: ");
    printListPIDs(readyTwo);
    printf("\n\n");

    printf("The currently running process is: {PID: %d, State: %d, Priority: %d}\n", runningProcess->PID, runningProcess->processState, runningProcess->priority);
    printf("*****************************************\n");
}

/**
 * @brief Blocks a process from running.
 * 
 * @param process The process to block.
 * @param blockType Indicates whether the process is blocked as a sender(0) or a receiver(1).
 * 
 * @return None.
 */
void blockProcess(PCB *process, bool blockType)
{
    PCB* blockedProcess = process;
    switch (process->priority)
    {
    case 0:
        List_first(readyZero);
        List_search(readyZero, pComparator, &process->PID);
        List_remove(readyZero);
        break;
    case 1:
        List_first(readyOne);
        List_search(readyOne, pComparator, &process->PID);
        List_remove(readyOne);
        break;
    case 2:
        List_first(readyTwo);
        List_search(readyTwo, pComparator, &process->PID);
        List_remove(readyTwo);
        break;
    default:
        break;
    }
    if (blockType == 0){
       List_prepend(blockedSenders, (void *)blockedProcess);
    } else if (blockType == 1){
       List_prepend(blockedReceivers, (void *)blockedProcess);
    }
}

void sendMessage(int sendPID, const char *msg)
{
    List_first(allProcesses);
    PCB *foundItem = List_search(allProcesses, pComparator, &sendPID);
    if (foundItem != NULL)
    {
        List_prepend(foundItem->incomingMessages, (void *)msg);
        quantumExpire();
        blockProcess(foundItem, 0);
    }
    else
    {
        perror("Process PID not found");
    }
}

void handleInput(char input)
{
    switch (toupper(input))
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
    case 'K':
        int pid;
        printf("Type process PID: \n");
        scanf("%d", &pid);
        killProcess(pid);
        break;
    case 'E':
        exitRunningProcess();
        break;
    case 'Q':
        quantumExpire();
        break;
    case 'S':
        int sendPID;
        printf("Type receiver PID: \n");
        scanf("%d", &sendPID);
        char msg[40];
        printf("Type message: \n");
        scanf("%s", msg);
        sendMessage(pid, msg);
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