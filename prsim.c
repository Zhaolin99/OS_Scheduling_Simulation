#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

// you do not generate how long you stay on the i/o until you really move to i/o queue
//overall tick (the one get from job) to set if it is running or not

//Struct
struct job{
    char name[11];
    int runtime; //Its total CPU time (as read);
    float prob_blocking;
    int time_run;
    int time_remain;
    bool initial_inCPU; // first sec in CPU
    bool initial_inIO; // first sec in IO
    bool just_run; //first time send to io after run 1 second

    int end_walltime; // The wall clock time at which it was completed
    int cpu_dispatch; // How many times the process was given the CPU;
    int io_block; // How many times the process blocked for I/O;
    int TimeInIO; // How much time the process spent doing I/O.
    bool should_block;
    int quantum;
};

struct CPU{
    bool busy;
    int time_to_stop; //runtime For RR
    int time_busy; //Total time spent busy;
    int time_idle; //Total time spent idle;
    float cpu_utilization; //calculate at end; (= busy time / total time);
    int num_dispatches; //number of times a process is moved onto the CPU
    int total_time;//total time (should be equal to walltime)
};

struct IO{
    bool busy;
    //int time_to_stop; //how long one job will be blocked
    int time_busy; //Total time spent busy;
    int time_idle; //Total time spent idle;
    float device_utilization;  //calculate at end; (= busy time / total time);
    int num_io_starts; //Number of times I/O was started
    int total_time;
};

//cpu_queue
//IO queue

typedef struct job job;
typedef struct CPU cpu;
typedef struct IO IO;

void display(struct job *j) {
    printf("\nDisplaying information\n");
    printf("Name: %s", (*j).name);
    printf("\nruntime: %d", (*j).runtime);
    printf("\nprob_blocking: %f.2", (*j).prob_blocking);
    printf("\ntim_run: %d", (*j).time_run);
    printf("\nTime remain: %d\n", (*j).time_remain);
}
job* init_job_struct(char* name, int runtime, float prob){
    job* new_job = (job*)malloc(sizeof(job));

    strcpy(new_job->name ,name);
    new_job->runtime=runtime;
    new_job->prob_blocking = prob;
    new_job->time_remain=runtime;
    new_job->time_run=0;
    new_job->end_walltime=0;
    new_job->cpu_dispatch=0;
    new_job->io_block=0;
    new_job->TimeInIO=0;
    new_job->initial_inCPU=true;
    new_job->initial_inIO=true;
    new_job->just_run=false;
    new_job->should_block=false;
    new_job->quantum=5;

    return new_job;
}

//linked list
typedef struct  list_node{
    job* current_job; // current_job->name
    struct list_node *next; //
} LinkedList;

// create a node
LinkedList* create_node(job *j)
{
    LinkedList* node=(LinkedList*)malloc(sizeof(LinkedList));
    node->current_job =j;
    node->next = NULL;
    return node;
}

//create job queue
LinkedList* creat_CPU_queue(int n, job** jobs){
    LinkedList *head, *node, *end;
    head = (LinkedList*)malloc(sizeof(LinkedList));
    end = head;         //set as empty
    for (int i = 0; i < n; i++) {
        node = (LinkedList*)malloc(sizeof(LinkedList));//set node(jobs) as empty
        end->current_job=jobs[i];
        end->next = node;
        if(i != n-1){
            end->next = node;
            end = end->next;
        }
    }
    //free(end);
    //end->current_job=NULL;
    end->next=NULL;//finish the end
    return head;
}


//add job node to the end
void append(LinkedList*** head, LinkedList** node)
{
    if ((**head)==NULL) // if the queue is empty
    {
        (**head)=(*node);
        (*node)->next = NULL;
        (**head)->next= NULL;
        return;
    }

    LinkedList* tmp =(**head); //remember the head pointer
    while ((**head)->next !=NULL) { //go through the list till the last element
        (**head) = (**head)->next;
        //strcpy((*head)->current_job->name, "change"); test for passing by reference
    }
    (**head)->next= (*node);
    (*node)->next = NULL;

    (**head) = tmp; //restore the head

}
//delete job node from the head
// I interpret it as completely delete the node (free)

void delete_head(LinkedList*** head)
{
    if ((**head)==NULL) // if the queue is empty
    {
        printf ("the queue is empty so nothing can be deleted");
        return; // return immediately
    }
    LinkedList *tmp;
    tmp=(**head);//this is the need-to-delete job
    (**head)=(tmp)->next;
    free(tmp);
}

//free malloc linked list(not tested)
void Delete_Queue(LinkedList** head)
{
    if ((*head)==NULL)// if the queue is empty
    {
        return; // done
    }
    LinkedList *tmp;
    tmp = (*head);
    while (*head!=NULL)
    {
        (*head)=(tmp)->next;
        free(tmp);
        tmp=(*head);
    }
    *head=NULL;
}
LinkedList* creat_EMPTY_queue(){
    LinkedList *head, *end;
    head = (LinkedList*)malloc(sizeof(LinkedList));
    head=NULL;
    end = head;         //set as empty
    return head;
}

void display_queue(LinkedList** queue){
    int i=1;
    if((*queue)!=NULL) {
        printf("node%d: \n", i);
        printf("\tname %s,", (*queue)->current_job->name);
        printf("\ttime remain %d, ", (*queue)->current_job->time_remain);
        printf("\truntime %d \n", (*queue)->current_job->runtime);
        LinkedList *tmp = (*queue)->next;
        i++;
        while (tmp!=NULL) {
            printf("node%d: \n", i);
            printf("\tname %s,", tmp->current_job->name);
            printf("\ttime remain %d, ", tmp->current_job->time_remain);
            printf("\truntime %d \n", tmp->current_job->runtime);
            tmp = tmp->next;
            i++;
        }
    }else{
        printf("NULL\n");
    }
}

void swap(job** a, job** b){
  job** c = *a;
  *a = *b;
  *b = c;
}
//display result
void display_result(job** jobs, cpu my_cpu, IO my_io, int *wallclock, int jobs_count){
    printf("Processes:\n\n");
    printf("   name     CPU time  when done  cpu disp  i/o disp  i/o time\n");
    //print out process
    int min=0;
    for(int i=0; i<jobs_count-1;i++){
      min=i;
      for(int j=i+1;j<jobs_count;j++){
        if((jobs)[i]->end_walltime>(jobs)[j]->end_walltime){
         min=j;
        }
        //swap(&jobs[min],&jobs[i]);
      }
      swap(&jobs[min],&jobs[i]);
    }
  for(int i=0; i<jobs_count;i++) {
    printf("%-10s %6d     %6d    %6d    %6d    %6d\n",
           jobs[i]->name,
           jobs[i]->runtime,
           jobs[i]->end_walltime,
           jobs[i]->cpu_dispatch,
           jobs[i]->io_block,
           jobs[i]->TimeInIO);
  }

    float walltime = (*wallclock);
    printf("\nSystem:\n");
    printf("The wall clock time at which the simulation finished: %d\n", *wallclock);
    //print out CPU
    printf("\nCPU:\n");
    printf("Total time spent busy: %d\n", my_cpu.time_busy);
    printf("Total time spent idle: %d\n", my_cpu.time_idle);
    printf("CPU utilization: %.2f\n", (my_cpu.time_busy / walltime)*1.00);
    printf("Number of dispatches: %d\n", my_cpu.num_dispatches);
    printf("Overall throughput: %.2f\n", (jobs_count / walltime));

    //print out I/O device
    printf("\nI/O device:\n");
    printf("Total time spent busy: %d\n", my_io.time_busy);
    printf("Total time spent idle: %d\n", my_io.time_idle);
    printf("I/O utilization: %.2f\n", (my_io.time_busy / walltime));
    printf("Number of dispatches: %d\n", my_io.num_io_starts);
    printf("Overall throughput: %.2f\n", (jobs_count / walltime));
}

//generate if need to be block
void actual_block(bool* current_job_initial_inCPU, float* current_job_prob_blocking, int time_remain, int* run_till_block, float* actual_block_prob ){
    // determine if block or not; generate a number to compare with block prob
    *actual_block_prob = (float) random() / (float) RAND_MAX; //between 0 - 1
    if (*actual_block_prob < *current_job_prob_blocking){ //will block
        //create an time run_till_block (between 1 and its remaining run time inclusive)
        *run_till_block = (int) random() % (time_remain - 1 + 1) + 1;
    }
    *current_job_initial_inCPU = false;
}

//SIMULATION Tests
void fcfs_simulation_test_for_allblock2(LinkedList** CPU_queue, LinkedList** IO_queue, int* tick, int* walltime, int* io_time, job** jobs, int jobs_count){
    //Initalize my cpu and io-device
    IO my_io; my_io = (IO) {.busy=false, .time_busy=0, .time_idle=0, .device_utilization=0, .num_io_starts=0, .total_time=0};
    cpu my_cpu; my_cpu = (cpu) {.busy=false, .time_to_stop=0, .time_busy=0, .time_idle=0, .cpu_utilization=0, .num_dispatches=0, .total_time=0};
    float actual_block_prob;
    int run_till_block; // a number btw 1 and its remaining run time inclusive.
    int stay_on_IO;
    bool just_deleted=false;

    while ((*CPU_queue) != NULL || (*IO_queue) != NULL) { //still have jobs to finish
      (*walltime)++; //for each time unit
        //PART1 Handle CPU and CPU_queue

        // if the time_remain of job is zero, kick out directly.
        if ((*CPU_queue) != NULL) {
            if((*CPU_queue) != NULL && (*CPU_queue)->current_job->time_remain == 0){
                (*CPU_queue)->current_job->end_walltime = (*walltime);
                //printf("***** %s ends on %d",(*CPU_queue)->current_job->name,(*walltime));
                (*CPU_queue)->current_job->initial_inCPU = true;
                if((*CPU_queue)->current_job->should_block){
                  my_cpu.num_dispatches++;
                  (*CPU_queue)->current_job->cpu_dispatch++;
                }
                delete_head(&CPU_queue); // delete the head
                my_cpu.busy=false;
                //my_cpu.num_dispatches++;
                //(*CPU_queue)->current_job->cpu_dispatch++;

                just_deleted = true;
            }
        }
        if ((*CPU_queue) != NULL && !just_deleted){
            my_cpu.busy = true;
            my_cpu.total_time++;
            (*tick)++;
            if ((*CPU_queue)->current_job->initial_inCPU && (*CPU_queue)->current_job->time_remain!=0) { //first second by CPU; need to check if block
                //set up variable 1.actual_block_prob; 2.run_till_block; 3.set back current_job_initial_inCPU to false until next round
                if((*CPU_queue)->current_job->time_remain<2){ //time remain <2, do not block
                  actual_block_prob = 1.00;
                  (*CPU_queue)->current_job->should_block=false;
                  (*CPU_queue)->current_job->initial_inCPU = false;
                  my_cpu.num_dispatches++;
                  (*CPU_queue)->current_job->cpu_dispatch++;
                } else {
                  //actual_block(&(*CPU_queue)->current_job->initial_inCPU,&(*CPU_queue)->current_job->prob_blocking, (*CPU_queue)->current_job->time_remain, &run_till_block, &actual_block_prob);
                  //printf("initial: actual_block_prob = %f\n", actual_block_prob);
                  //printf ("the job name: %s\n", (*CPU_queue)->current_job->name);
                  actual_block_prob = (float) random() / (float) RAND_MAX; //between 0 - 1
                  //actual_block_prob=0;
                  //printf ("the actual block prob is: %f\n", actual_block_prob);
                  if (actual_block_prob < (*CPU_queue)->current_job->prob_blocking){ //will block
                    //create an time run_till_block (between 1 and its remaining run time inclusive)
                    //printf("initial: runtill blok = %d\n", run_till_block);
                    (*CPU_queue)->current_job->should_block=true;
                    run_till_block = (int) random() % ((*CPU_queue)->current_job->time_remain) + 1;
                    //run_till_block=2;
                    //printf ("need to block, the run_till_block is: %d\n",run_till_block);
                    //printf("set: runtill blok = %d\n", run_till_block);
                  }else{
                    (*CPU_queue)->current_job->should_block=false;
                  }
                  //for test
                  //actual_block_prob = 0;
                  //run_till_block = 2;
                  (*CPU_queue)->current_job->initial_inCPU = false;
                  my_cpu.num_dispatches++;
                  (*CPU_queue)->current_job->cpu_dispatch++;
                }
            }

            if (actual_block_prob < (*CPU_queue)->current_job->prob_blocking) {
                //block
                  //current job is not finished, run one second
                    (*CPU_queue)->current_job->time_run++;
                    (*CPU_queue)->current_job->time_remain--;
                    run_till_block--;
                    my_cpu.time_busy++;
                    (*CPU_queue)->current_job->just_run = true;
                    if (run_till_block == 0) //after run, send to IO
                    {
                        (*CPU_queue)->current_job->initial_inCPU = true;
                        //LinkedList *tmp = CPU_queue;
                        LinkedList *tmp = create_node((*CPU_queue)->current_job);
                        //printf ("%s is sent to IO\n",(*CPU_queue)->current_job->name);
                        append(&IO_queue, &tmp); // append the current job to IO queue
                        delete_head(&CPU_queue); // delete the head
                        my_cpu.busy=false;
                    }

            } else {
                // not block
                // run the current job for one time unit
                (*CPU_queue)->current_job->time_run++;
                (*CPU_queue)->current_job->time_remain--;
                my_cpu.time_busy++;
                /*
                if ((*CPU_queue)->current_job->time_remain == 0) {
                  (*CPU_queue)->current_job->end_walltime = (*walltime);
                  //printf("***** %s ends on %d",(*CPU_queue)->current_job->name,(*walltime));
                  (*CPU_queue)->current_job->initial_inCPU = true;
                  delete_head(&CPU_queue);
                  my_cpu.busy=false;// delete the head
                }*/
            }
        }
        else // CPU_queue is idle
        {
            my_cpu.busy = false;
            my_cpu.time_idle++;
            my_cpu.total_time++;
            just_deleted = false;
        }

        //PART2 Handle IO and IO_queue
        if (((*IO_queue) != NULL) &&(!(*IO_queue)->current_job->just_run) )
        {
            my_io.busy = true;
            //my_io.time_busy++;
            my_io.total_time++;
            (*io_time)++;
            LinkedList* tmp = (*IO_queue)->next;
            while(tmp != NULL){
                tmp->current_job->just_run=false;
                tmp = tmp->next;
            }
            if ((*IO_queue)->current_job->initial_inIO) {//first second by IO
                //set up HOW LONG current job will wait in IO queue
                my_io.num_io_starts++;
                (*IO_queue)->current_job->io_block++;
                if((*IO_queue)->current_job->time_remain==0) {
                  stay_on_IO=1;
                }else{
                  stay_on_IO = (int) random() % 30 + 1; // generate a number btw 1--30
                }

                //stay_on_IO = (int) random() % 30 + 1; // generate a number btw 1--30
                //stay_on_IO=1;

                //printf ("%s is at IO, the block time is set to %d\n",(*IO_queue)->current_job->name,stay_on_IO);
                //stay_on_IO = 1;
                (*IO_queue)->current_job->initial_inIO = false;
            }
            stay_on_IO--;
            (*IO_queue)->current_job->TimeInIO++;
            my_io.time_busy++;
            if (stay_on_IO == 0) { // finish running on IO
                (*IO_queue)->current_job->initial_inIO = true; // prepare for next time
                LinkedList *tmp = create_node((*IO_queue)->current_job);
                append(&CPU_queue, &tmp);//send back to CPU queue
                delete_head(&IO_queue);
            }
        }
        else if (((*IO_queue) != NULL) &&((*IO_queue)->current_job->just_run) ) {
            (*IO_queue)->current_job->just_run = false;
            my_io.busy = false;
            my_io.time_idle++;
            my_io.total_time++;
        }
        else{ // IO_queue is idel
            my_io.busy = false;
            my_io.time_idle++;
            my_io.total_time++;
        }

        //PART3 check the status of two queues
        if ((*CPU_queue) == NULL) {
            my_cpu.busy = false;
        }
        if ((*IO_queue) == NULL) {
            my_io.busy = false;
        }

        //printf("\nAt walltime %d:\n", (*walltime));
        //printf("CPU QUEUE:\n");
        //display_queue(CPU_queue);
        //printf("IO QUEUE:\n");
        //display_queue(IO_queue);

    }
    display_result(jobs,my_cpu,my_io,walltime,jobs_count);
}
int min (int a, int b)
{
  if (a<=b)
    return a;
  else
    return b;
}

void rr_simulation(LinkedList** CPU_queue, LinkedList** IO_queue, int* tick, int* walltime, int* io_time, job** jobs, int jobs_count)
{
    IO my_io; my_io = (IO) {.busy=false, .time_busy=0, .time_idle=0, .device_utilization=0, .num_io_starts=0, .total_time=0};
    cpu my_cpu; my_cpu = (cpu) {.busy=false, .time_to_stop=0, .time_busy=0, .time_idle=0, .cpu_utilization=0, .num_dispatches=0, .total_time=0};
    float actual_block_prob;
    int run_till_block; // a number btw 1 and its remaining run time inclusive.
    int stay_on_IO;
    bool just_deleted=false;
    //RR
    //int job_quantum;
    while ((*CPU_queue) != NULL || (*IO_queue) != NULL){
        (*walltime)++; //for each time unit
        // CPU with RR
        // if the time_remain of job is zero, kick out directly.

        if((*CPU_queue) != NULL && (*CPU_queue)->current_job->time_remain == 0){
          (*CPU_queue)->current_job->end_walltime = (*walltime);
          //printf("***** %s ends on %d",(*CPU_queue)->current_job->name,(*walltime));
          (*CPU_queue)->current_job->initial_inCPU = true;
          if( (*CPU_queue)->current_job->should_block){
            my_cpu.num_dispatches++;
            (*CPU_queue)->current_job->cpu_dispatch++;
          }
          delete_head(&CPU_queue); // delete the head
          my_cpu.busy=false;
          just_deleted = true;
        }else if((*CPU_queue) != NULL && (*CPU_queue)->current_job->quantum == 0){
          LinkedList* tmp= create_node((*CPU_queue)->current_job); //send to CPU tail
          (*CPU_queue)->current_job->initial_inCPU = true;
          delete_head(&CPU_queue);
          append(&CPU_queue,&tmp);
        }

        if ((*CPU_queue) != NULL && !just_deleted){
            my_cpu.busy=true;
            my_cpu.total_time ++;
            (*tick)++;
            if ((*CPU_queue)->current_job->initial_inCPU && (*CPU_queue)->current_job->time_remain !=0 ) { //first second by CPU; need to check if block
                (*CPU_queue)->current_job->quantum = 5;
                if((*CPU_queue)->current_job->time_remain<2){ //time remain <2, do not block
                  actual_block_prob = 1.00;
                  (*CPU_queue)->current_job->should_block=false;
                  (*CPU_queue)->current_job->initial_inCPU = false;
                  my_cpu.num_dispatches++;
                  (*CPU_queue)->current_job->cpu_dispatch++;
                }else{
                  //printf ("the job name: %s\n", (*CPU_queue)->current_job->name);
                  actual_block_prob=(float) random() / (float) RAND_MAX; //between 0 - 1
                  //printf ("the actual block prob is: %.2f\n", (*CPU_queue)->current_job->prob_blocking);
                  //printf("set: actual_block_prob = %.2f\n", actual_block_prob);
                  if (actual_block_prob < (*CPU_queue)->current_job->prob_blocking){ //will block
                    //create an time run_till_block (between 1 and its remaining run time inclusive)
                    //printf("initial: runtill blok = %d\n", run_till_block);
                    //run_till_block = (int) random() % (5)+1;
                      int tmp =min(5,(*CPU_queue)->current_job->time_remain);
                                          run_till_block = (int) random() % (tmp)+1;
                    //run_till_block=2;
                    //printf ("need to block, the run_till_block is: %d\n",run_till_block);
                    (*CPU_queue)->current_job->should_block=true;
                    //printf("set: runtill blok = %d\n", run_till_block);
                  }else{
                    (*CPU_queue)->current_job->should_block=false;
                  }
                }
                (*CPU_queue)->current_job->initial_inCPU = false;
                my_cpu.num_dispatches++;
                (*CPU_queue)->current_job->cpu_dispatch++;
            }

            if (actual_block_prob < (*CPU_queue)->current_job->prob_blocking){//block
                (*CPU_queue)->current_job->time_run++;
                (*CPU_queue)->current_job->time_remain--;
                run_till_block--;
                my_cpu.time_busy++;
                (*CPU_queue)->current_job->just_run = true;
                (*CPU_queue)->current_job->quantum--;
                if (run_till_block == 0) //after run, send to IO
                {
                    (*CPU_queue)->current_job->initial_inCPU = true;
                    LinkedList *tmp = create_node((*CPU_queue)->current_job);
                    append(&IO_queue, &tmp); // append the current job to IO queue
                    delete_head(&CPU_queue); // delete the head
                    my_cpu.busy=false;
                }

            }
            else{// not block
              (*CPU_queue)->current_job->time_run++;
              (*CPU_queue)->current_job->time_remain--;
              my_cpu.time_busy++;
              (*CPU_queue)->current_job->quantum--;
            }
        }
        else // CPU_queue is idel
        {
            my_cpu.busy = false;
            my_cpu.time_idle++;
            my_cpu.total_time++;
            just_deleted = false;
        }

        //Part2 IO
        if (((*IO_queue) != NULL) &&(!(*IO_queue)->current_job->just_run) ){
          my_io.busy = true;
          my_io.total_time++;
          (*io_time)++;
          LinkedList* tmp = (*IO_queue)->next;
          while(tmp != NULL){
            tmp->current_job->just_run=false;
            tmp = tmp->next;
          }

          if ((*IO_queue)->current_job->initial_inIO) {//first second by IO
            //set up HOW LONG current job will wait in IO queue
            my_io.num_io_starts++;
            (*IO_queue)->current_job->io_block++;
            //printf ("%s is at IO, the block time is %d\n",(*IO_queue)->current_job->name,stay_on_IO);
            //stay_on_IO = 1;
            if((*IO_queue)->current_job->time_remain==0) {
              stay_on_IO=1;
            }else{
              stay_on_IO = (int) random() % 30 + 1; // generate a number btw 1--30
            }
            (*IO_queue)->current_job->initial_inIO = false;
          }

          stay_on_IO--;
          (*IO_queue)->current_job->TimeInIO++;
          my_io.time_busy++;
          if (stay_on_IO == 0) { // finish running on IO
            (*IO_queue)->current_job->initial_inIO = true; // prepare for next time
            LinkedList *tmp = create_node((*IO_queue)->current_job);
            append(&CPU_queue, &tmp);//send back to CPU queue
            delete_head(&IO_queue);
          }
        }
        else if (((*IO_queue) != NULL) &&((*IO_queue)->current_job->just_run) ) {
            (*IO_queue)->current_job->just_run = false;
            my_io.busy = false;
            my_io.time_idle++;
            my_io.total_time++;
        }
        else{ // IO_queue is idel
            my_io.busy = false;
            my_io.time_idle++;
            my_io.total_time++;
        }

    //PART3 check the status of two queues
    if ((*CPU_queue) == NULL) {
      my_cpu.busy = false;
    }
    if ((*IO_queue) == NULL) {
      my_io.busy = false;
    }

      //printf("\nAt walltime %d:\n", (*walltime));
      //printf("CPU QUEUE:\n");
      //display_queue(CPU_queue);
      //printf("IO QUEUE:\n");
      //display_queue(IO_queue);
    }

    display_result(jobs,my_cpu,my_io,walltime,jobs_count);
}

int main(int argc, char* argv[]) {
    //initialize
    bool fcfs = false;
    if (argc != 3){perror("Invalid input");}
    if(strcmp(argv[1], "-f")==0){//fcfs mode
        fcfs = true;
    } else if(strcmp(argv[1], "-r")==0){
        fcfs = false;
    } else{
        fprintf(stderr, "Usage: %s [-r | -f] file\n", argv[0]);
        exit(1);
    }

    srandom(12345);
    int tick=0, io_count=0, walltime=0; //tick->countring cpu time; io_count -> counting io time; walltime->count full program time
    int jobs_count=0;
//jobs read in
    job* jobs[3]={NULL};
    //open files
    char line[100], *delim = "\t\t";
    int i = 0;
    FILE *fptr;
    fptr = fopen(argv[2],"r");
    if(fptr == NULL){
        errno = ENOENT;
        perror("filename");
        exit(1);
    }
    //get file content into the jobs array
    while (fgets(line, 100, fptr) != NULL) {
        jobs_count++;
        char * p;
        //printf("line: %s\n",line);
        p = strtok (line,delim);
        char name[11]={NULL};
        strcpy(name, p); //process name is to long (over 10 characters)
        //printf("name:%s\t",name);

        char* temp;
        p = strtok(NULL,delim);
        int runtime; runtime = atoi(p); //check if p has int; otherwise stderr, strtol
        int x= strtol(p,&temp,10);
        if(*temp != '\0')
        {
          fprintf(stderr, "Malformed line %s(%d)\n", argv[2], jobs_count);
          exit(1);
        }

        if(runtime <=0){
          fprintf(stderr, "runtime is not positive integer %s(%d)\n", argv[2], 100);
          exit(1);
        } // runtime is 0 or less
        //printf("run: %d\t",runtime);
        p = strtok(NULL,delim);
        float prob_blocking; prob_blocking= atof(p);
        //printf("prob:%.2f",prob_blocking);
        if(prob_blocking<0 || prob_blocking>1){ // probability is not between 0 and 1
            fprintf(stderr, "probability < 0 or > 1 %s(%d)\n", argv[2], 100);
            exit(1);
        }
        jobs[i]=init_job_struct(name, runtime,prob_blocking);
        //display(jobs[i]);
        i++;
    }
    fclose(fptr);
    //jobs holds all jobs from read file with struct

//Handle jobs
    //created two queue
    LinkedList* CPU_queue=creat_CPU_queue(jobs_count,jobs);
    LinkedList* IO_queue=creat_EMPTY_queue();

    if(fcfs){
        fcfs_simulation_test_for_allblock2(&CPU_queue, &IO_queue, &tick, &walltime, &io_count, jobs, jobs_count);
    } else {
        rr_simulation(&CPU_queue, &IO_queue, &tick, &walltime, &io_count, jobs, jobs_count);
    }

    // free everything // though do not need to
//    Delete_Queue(&CPU_queue);
//    Delete_Queue(&IO_queue);
    //test for linked list
    /*
  LinkedList* CPU_queue=creat_CPU_queue(3,jobs);
  job job_a,job_b;
  job_a = (job) {.name="selftest1", .runtime=4, .prob_blocking=0.87, .time_run=0, .time_remain=0};
  job_b = (job) {.name="selftest2", .runtime=4, .prob_blocking=0.87, .time_run=0, .time_remain=0};
  LinkedList* node1 = create_node(&job_a);
  LinkedList* node2 = create_node(&job_b);

  append(&CPU_queue,&node1);
  append(&CPU_queue,&node2);
  delete_head(&CPU_queue);
  Delete_Queue(&CPU_queue);
*/
    /*
  LinkedList* empty_queue=creat_EMPTY_queue();
  job job_a,job_b;
  job_a = (job) {.name="selftest1", .runtime=4, .prob_blocking=0.87, .time_run=0, .time_remain=0};
  job_b = (job) {.name="selftest2", .runtime=4, .prob_blocking=0.87, .time_run=0, .time_remain=0};
  LinkedList* node1 = create_node(&job_a);
  LinkedList* node2 = create_node(&job_b);
  //append(&empty_queue,&node1);
  //append(&empty_queue,&node2); 连续append 有问题
  //delete_head(&empty_queue);
  //Delete_Queue(&empty_queue);
*/

    // for(int i = 0; i <10; i++){
    //int x = (int) random() % (i+1) + 1;
    //printf("i+1:%d   x: %d \n", i+1, x);
    //}


    //display_queue(&CPU_queue);
    if (CPU_queue!=NULL)
        perror("CPU queue is not empty");
    if (IO_queue!=NULL)
        perror("IO queue is not empty");

    return 0;
}
