#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/// Maximum number of processes that can be scheduled.
#define MAX_PROCESSES 20

/// A process read from the input file
typedef struct {
    int pid;
    int arrivalTime;
    int burstTime;
    int index;
} Process;

/// Timing information calculated by a scheduling algorithm.
typedef struct {
    int pid;
    int arrivalTime;
    int startTime;
    int endTime;
    int runningTime;
    int waitingTime;
} Statistics;

/// Circular queue used by the Round Robin scheduler
typedef struct {
    Process processes[MAX_PROCESSES];
    int front;
    int rear;
    int size;
} Queue;

/// Initialize an empty process queue.
static void initQueue(Queue *queue)
{
    queue->front = 0;
    queue->rear = 0;
    queue->size = 0;
}

/// Add a process to the queue.
static void enqueue(Queue *queue, Process process)
{
    queue->processes[queue->rear] = process;
    queue->rear = (queue->rear + 1) % MAX_PROCESSES;
    queue->size++;
}

/// Remove and return the process at the front of the queue.
static Process dequeue(Queue *queue)
{
    Process process = queue->processes[queue->front];
    queue->front = (queue->front + 1) % MAX_PROCESSES;
    queue->size--;
    return process;
}

/// Return nonzero when the queue contains no processes.
static int isEmpty(const Queue *queue)
{
    return queue->size == 0;
}

/// Print the completed statistics table and average waiting time.
static void printStatistics(const char *title, const Statistics *stats, int count)
{
    double totalWaitingTime = 0.0;

    printf("%s\n", title);
    printf("PID\tArrival Time\tStart Time\tEnd Time\tRunning Time\tWaiting Time\n");

    for (int i = 0; i < count; i++) {
        totalWaitingTime += stats[i].waitingTime;
        printf("%d\t%d\t\t%d\t\t%d\t\t%d\t\t%d\n",
               stats[i].pid,
               stats[i].arrivalTime,
               stats[i].startTime,
               stats[i].endTime,
               stats[i].runningTime,
               stats[i].waitingTime);
    }

    printf("Average Waiting Time: %.2f\n\n", totalWaitingTime / count);
}

/// Schedule processes in first-come, first-served order.
static void firstComeFirstServe(const Process *processes, int count)
{
    int currentTime = 0;
    Statistics stats[count];

    for (int i = 0; i < count; i++) {
        if (currentTime < processes[i].arrivalTime) {
            currentTime = processes[i].arrivalTime;
        }

        stats[i].pid = processes[i].pid;
        stats[i].arrivalTime = processes[i].arrivalTime;
        stats[i].startTime = currentTime;
        currentTime += processes[i].burstTime;
        stats[i].endTime = currentTime;
        stats[i].runningTime = stats[i].endTime - stats[i].startTime;
        stats[i].waitingTime = stats[i].startTime - stats[i].arrivalTime;
    }

    printStatistics("FCFS:", stats, count);
}

/// Schedule processes using Round Robin.
static void roundRobin(const Process *processes, int count, int timeQuantum)
{
    /// Set up need variables for the scheduling loop, statistics tracking, and the ready queue.
    int currentTime = 0;
    int completedProcesses = 0;
    int nextProcessIndex = 0;
    Queue readyQueue;
    Statistics stats[MAX_PROCESSES] = {0};

    /// Initialize the ready queue and set all start times to -1 to indicate they haven't started yet.
    initQueue(&readyQueue);
    for (int i = 0; i < count; i++) {
        stats[i].startTime = -1;
    }

    printf("PID\tStart Time\tEnd Time\tRunning Time\n");

    /// Main scheduling loop continues until all processes have completed.
    while (completedProcesses < count) {
        /// Enqueue all processes that have arrived by the current time.
        while (nextProcessIndex < count &&
               processes[nextProcessIndex].arrivalTime <= currentTime) {
            enqueue(&readyQueue, processes[nextProcessIndex]);
            nextProcessIndex++;
        }

        /// If the ready queue is empty, advance time to the next process arrival.
        if (isEmpty(&readyQueue)) {
            currentTime = processes[nextProcessIndex].arrivalTime;
            continue;
        }

        /// Dequeue the next process and determine how long it will run in this time slice.
        Process currentProcess = dequeue(&readyQueue);
        int timeSlice = currentProcess.burstTime < timeQuantum ? currentProcess.burstTime : timeQuantum;

        /// If the current time is less than the arrival time of the current process, advance the current time to the arrival time.
        if (currentTime < currentProcess.arrivalTime) {
            currentTime = currentProcess.arrivalTime;
        }

        /// Update the statistics for the current process.
        Statistics *currentStats = &stats[currentProcess.index];
        currentStats->pid = currentProcess.pid;
        currentStats->arrivalTime = currentProcess.arrivalTime;
        if (currentStats->startTime == -1) {
            currentStats->startTime = currentTime;
        }

        int sliceStartTime = currentTime;
        currentTime += timeSlice;
        currentStats->endTime = currentTime;
        currentStats->runningTime += timeSlice;
        currentProcess.burstTime -= timeSlice;

        /// Print the scheduling information for the current time slice.
        printf("%d\t%d\t\t%d\t\t%d\n", currentProcess.pid, sliceStartTime, currentStats->endTime, timeSlice);

        /// Enqueue all processes that have arrived by the current time.
        while (nextProcessIndex < count &&
               processes[nextProcessIndex].arrivalTime <= currentTime) {
            enqueue(&readyQueue, processes[nextProcessIndex]);
            nextProcessIndex++;
        }

        /// If the current process still has burst time remaining, enqueue it back into the ready queue.
        if (currentProcess.burstTime > 0) {
            enqueue(&readyQueue, currentProcess);
        } else {
            /// Otherwise, update its waiting time and increment the completed process count.
            currentStats->waitingTime = currentStats->endTime - currentStats->arrivalTime - currentStats->runningTime;
            completedProcesses++;
        }
    }

    /// Print the scheduling statistics.
    char title[80];
    snprintf(title, sizeof(title),
             "RR (Time Quantum: %d):",
             timeQuantum);
    printStatistics(title, stats, count);
}


/// Schedule processes using Shortest Job First (SJF) scheduling, non-preemptive.
static void shortestJobFirst(Process *processes, int count)
{
    /// Set up variables for the scheduling loop and statistics tracking.
    int currentTime = 0;
    Statistics stats[count];

    /// Main scheduling loop continues until all processes have been scheduled.
    for (int i = 0; i < count; i++) {
        int shortestIndex = -1;

        /// Find the index of the next process to schedule based on the shortest burst time among those that have arrived.
        for (int j = i; j < count; j++) {
            if (processes[j].arrivalTime <= currentTime &&
                (shortestIndex == -1 ||
                 processes[j].burstTime < processes[shortestIndex].burstTime)) {
                shortestIndex = j;
            }
        }

        /// If no process has arrived yet, advance time to the next arrival.
        if (shortestIndex == -1) {
            currentTime = processes[i].arrivalTime;
            shortestIndex = i;

            /// find the next process with the shortest burst time among those that have arrived.
            for (int j = i + 1; j < count; j++) {
                if (processes[j].arrivalTime <= currentTime &&
                    processes[j].burstTime < processes[shortestIndex].burstTime) {
                    shortestIndex = j;
                }
            }
        }

        /// Swap the selected process into the current position.
        if (shortestIndex != i) {
            Process temp = processes[i];
            processes[i] = processes[shortestIndex];
            processes[shortestIndex] = temp;
        }

        /// Update the statistics for the selected process.
        stats[i].pid = processes[i].pid;
        stats[i].arrivalTime = processes[i].arrivalTime;
        stats[i].startTime = currentTime;
        currentTime += processes[i].burstTime;
        stats[i].endTime = currentTime;
        stats[i].runningTime = stats[i].endTime - stats[i].startTime;
        stats[i].waitingTime = stats[i].startTime - stats[i].arrivalTime;
    }

    /// Print the scheduling statistics.
    printStatistics("SJF:", stats, count);
}

/** Sort processes by arrival time, then by PID for ties. */
static void sortProcesses(Process *processes, int count)
{
    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            /// Sort by arrival time, then by PID for ties.
            if (processes[i].arrivalTime > processes[j].arrivalTime || (processes[i].arrivalTime == processes[j].arrivalTime && processes[i].pid > processes[j].pid)) {
                Process temp = processes[i];
                processes[i] = processes[j];
                processes[j] = temp;
            }
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <input_file> [FCFS|RR|SJF] <time_quantum>\n", argv[0]);
        return EXIT_FAILURE;
    }

    /// Open the input file.
    FILE *file = fopen(argv[1], "r");

    /// Validate that the input file was opened successfully.
    if (file == NULL) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    Process processes[MAX_PROCESSES];
    int count;
    /// Read the number of processes from the input file, making sure it is a positive integer.
    if (fscanf(file, "%d", &count) != 1 || count <= 0) {
        fprintf(stderr, "Error: invalid process count.\n");
        fclose(file);
        return EXIT_FAILURE;
    }

    /// Read the process data.
    for (int i = 0; i < count; i++) {
        if (fscanf(file, "%d %d %d", &processes[i].pid, &processes[i].arrivalTime, &processes[i].burstTime) != 3) {
            fprintf(stderr, "Error: invalid process data.\n");
            fclose(file);
            return EXIT_FAILURE;
        }
        processes[i].index = i;
    }

    /// Close the input file after reading all process data.
    fclose(file);

    /// Sort the processes by arrival time with PID as the tie breaker.
    sortProcesses(processes, count);

    if (strcmp(argv[2], "FCFS") == 0) 
    {
        /// Run FCFS scheduling algorithm.
        firstComeFirstServe(processes, count);
    } else if (strcmp(argv[2], "RR") == 0) 
    {   
        int timeQuantum = atoi(argv[3]);

        /// Validate the time quantum for Round Robin scheduling.
        if (timeQuantum <= 0) {
            fprintf(stderr, "Error: time quantum must be positive.\n");
            return EXIT_FAILURE;
        }
        /// Run RR scheduling algorithm.
        roundRobin(processes, count, timeQuantum);

    } else if (strcmp(argv[2], "SJF") == 0) 
    {
        /// Run SJF scheduling algorithm.
        shortestJobFirst(processes, count);
    } else {
        /// Invalid scheduling algorithm specified.
        fprintf(stderr, "Invalid scheduling algorithm specified. Use FCFS, RR, or SJF.\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
