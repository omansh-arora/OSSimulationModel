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
    List *readyQueue;
    List *incomingMessages; // Change to pointer to List if List is a struct
    char currMessage[40];
    bool messageReceived;
} PCB;

typedef struct Message_s
{
    int senderPID;
    int receiverPID;
    char message[40];

} Message;

static List *allProcesses;
static List *readyZero;
static List *readyOne;
static List *readyTwo;
static List *blockedSendersQueue;
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
void setNextRunningProcess();
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
    blockedSendersQueue = List_create();
    blockedReceivers = List_create();

    initProcess = (PCB *)malloc(sizeof(PCB));
    initProcess->processState = RUNNING;
    initProcess->PID = -1;
    initProcess->incomingMessages = List_create();
    runningProcess = initProcess;
}

void putProcessOnQueue(PCB *process)
{
    List_append(allProcesses, (void *)process);
    List_prepend(process->readyQueue, (void *)process);
    process->processState = READY;
}

void createProcess(int priority)
{
    PCB *newProcess = (PCB *)malloc(sizeof(PCB));
    newProcess->incomingMessages = List_create();
    newProcess->PID = PIDCounter;
    newProcess->processState = READY;
    newProcess->priority = priority;
    newProcess->messageReceived = false;

    if (priority == 0)
    {
        newProcess->readyQueue = readyZero;
    }
    else if (priority == 1)
    {
        newProcess->readyQueue = readyOne;
    }
    else if (priority == 2)
    {
        newProcess->readyQueue = readyTwo;
    }
    else
    {
        perror("Error: priority has to be: 0, 1, 2\n");
        free(newProcess);
        PIDCounter--;
        return;
    }
    PIDCounter++;
    putProcessOnQueue(newProcess);
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
    newProcess->readyQueue = runningProcess->readyQueue;
    newProcess->messageReceived = runningProcess->messageReceived;
    strcpy(runningProcess->currMessage, runningProcess->currMessage);
    PIDCounter++;
    putProcessOnQueue(newProcess);
}

bool pComparator(void *pItem, void *pComparisonArg)
{
    return ((PCB *)pItem)->PID == *((int *)pComparisonArg);
}

/**
 * @brief Kills a process with a given PID.
 *
 * @param delPID The PID of the process to kill.
 *
 * @return None.
 */
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
        List *readyQ = foundItem->readyQueue;
        List_first(readyQ);
        List_search(readyQ, pComparator, &delPID);
        List_remove(readyQ);
        List_remove(allProcesses);
    }
    else
    {
        perror("Process PID not found");
    }
}

void exitRunningProcess()
{
    if (runningProcess->PID != initProcess->PID)
    {
        List_first(allProcesses);
        List_search(allProcesses, pComparator, &(runningProcess->PID));
        List_remove(allProcesses);
        List *readyQ = runningProcess->readyQueue;
        List_first(readyQ);
        List_search(readyQ, pComparator, &(runningProcess->PID));
        List_remove(readyQ);
        setNextRunningProcess();
    }
}

void setNextRunningProcess()
{
    if (List_count(readyZero) > 0)
    {
        runningProcess = (PCB *)List_trim(readyZero);
    }
    else if (List_count(readyOne) > 0)
    {
        runningProcess = (PCB *)List_trim(readyOne);
    }
    else if (List_count(readyTwo) > 0)
    {
        runningProcess = (PCB *)List_trim(readyTwo);
    }
    else
    {
        runningProcess = initProcess;
    }
    runningProcess->processState = RUNNING;
}
void returnProcessToQueue(PCB *process)
{
    List *readyQ = process->readyQueue;
    List_prepend(readyQ, (void *)process);
    process->processState = READY;
}

void quantumExpire()
{
    PCB *oldRunningProcess = runningProcess;
    setNextRunningProcess();
    if (oldRunningProcess->PID != initProcess->PID)
        returnProcessToQueue(oldRunningProcess);
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
    printf("\n");

    printf("Blocked Senders: ");
    printListPIDs(blockedSendersQueue);
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
    PCB *blockedProcess = process;
    List *readyQ = blockedProcess->readyQueue;
    blockedProcess->processState = BLOCKED;
    List_first(readyQ);
    List_search(readyQ, pComparator, &process->PID);
    List_remove(readyQ);

    if (blockType == 0)
    {
        List_prepend(blockedSendersQueue, (void *)blockedProcess);
    }
    else if (blockType == 1)
    {
        List_prepend(blockedReceivers, (void *)blockedProcess);
    }
}

void sendMessage(int sendPID, Message *msg)
{
    List_first(allProcesses);
    PCB *foundItem = List_search(allProcesses, pComparator, &sendPID);

    if (foundItem != NULL)
    {
        msg->senderPID = runningProcess->PID;
        List_first(blockedReceivers);
        PCB *blockedReceiver = List_search(blockedReceivers, pComparator, &sendPID);

        if (blockedReceiver != NULL)
        {
            List_remove(blockedReceivers);
            putProcessOnQueue(blockedReceiver);
        }
        PCB *sender = runningProcess;
        List_prepend(foundItem->incomingMessages, (void *)msg->message);
        setNextRunningProcess();
        blockProcess(sender, 0);
    }
    else
    {
        perror("Process PID not found");
    }
}

void unblockSender(PCB* process){
    List_first(blockedSendersQueue);
    PCB* blockedSender = List_search(blockedSendersQueue, pComparator, process->PID);
    
}

void receive()
{
    Message *msg = List_trim(runningProcess->incomingMessages);
    if (msg != NULL)
    {
        List_first(allProcesses);
        unblockSender(List_search(allProcesses, msg->senderPID));
        runningProcess->messageReceived = true;
        strcpy(runningProcess->currMessage, msg->message);
    }
    else
    {
        PCB *blockedProcess = runningProcess;
        setNextRunningProcess();
        blockProcess(blockedProcess, 1);
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

        // Clear input buffer before reading the message
        while (getchar() != '\n')
            ; // Clear input buffer

        Message *msg = (Message *)malloc(sizeof(Message));
        printf("Type message: \n");
        scanf("%[^\n]%*c", msg->message);

        sendMessage(sendPID, msg);
        break;
    case 'R':
        receive();
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